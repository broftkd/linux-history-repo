/*
 * Copyright(c) 2004 - 2006 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called COPYING.
 */

/*
 * This code implements the DMA subsystem. It provides a HW-neutral interface
 * for other kernel code to use asynchronous memory copy capabilities,
 * if present, and allows different HW DMA drivers to register as providing
 * this capability.
 *
 * Due to the fact we are accelerating what is already a relatively fast
 * operation, the code goes to great lengths to avoid additional overhead,
 * such as locking.
 *
 * LOCKING:
 *
 * The subsystem keeps two global lists, dma_device_list and dma_client_list.
 * Both of these are protected by a mutex, dma_list_mutex.
 *
 * Each device has a channels list, which runs unlocked but is never modified
 * once the device is registered, it's just setup by the driver.
 *
 * Each client is responsible for keeping track of the channels it uses.  See
 * the definition of dma_event_callback in dmaengine.h.
 *
 * Each device has a kref, which is initialized to 1 when the device is
 * registered. A kref_get is done for each device registered.  When the
 * device is released, the corresponding kref_put is done in the release
 * method. Every time one of the device's channels is allocated to a client,
 * a kref_get occurs.  When the channel is freed, the corresponding kref_put
 * happens. The device's release function does a completion, so
 * unregister_device does a remove event, device_unregister, a kref_put
 * for the first reference, then waits on the completion for all other
 * references to finish.
 *
 * Each channel has an open-coded implementation of Rusty Russell's "bigref,"
 * with a kref and a per_cpu local_t.  A dma_chan_get is called when a client
 * signals that it wants to use a channel, and dma_chan_put is called when
 * a channel is removed or a client using it is unregistered.  A client can
 * take extra references per outstanding transaction, as is the case with
 * the NET DMA client.  The release function does a kref_put on the device.
 *	-ChrisL, DanW
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/hardirq.h>
#include <linux/spinlock.h>
#include <linux/percpu.h>
#include <linux/rcupdate.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>

static DEFINE_MUTEX(dma_list_mutex);
static LIST_HEAD(dma_device_list);
static LIST_HEAD(dma_client_list);
static long dmaengine_ref_count;

/* --- sysfs implementation --- */

static ssize_t show_memcpy_count(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct dma_chan *chan = to_dma_chan(dev);
	unsigned long count = 0;
	int i;

	for_each_possible_cpu(i)
		count += per_cpu_ptr(chan->local, i)->memcpy_count;

	return sprintf(buf, "%lu\n", count);
}

static ssize_t show_bytes_transferred(struct device *dev, struct device_attribute *attr,
				      char *buf)
{
	struct dma_chan *chan = to_dma_chan(dev);
	unsigned long count = 0;
	int i;

	for_each_possible_cpu(i)
		count += per_cpu_ptr(chan->local, i)->bytes_transferred;

	return sprintf(buf, "%lu\n", count);
}

static ssize_t show_in_use(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct dma_chan *chan = to_dma_chan(dev);

	return sprintf(buf, "%d\n", chan->client_count);
}

static struct device_attribute dma_attrs[] = {
	__ATTR(memcpy_count, S_IRUGO, show_memcpy_count, NULL),
	__ATTR(bytes_transferred, S_IRUGO, show_bytes_transferred, NULL),
	__ATTR(in_use, S_IRUGO, show_in_use, NULL),
	__ATTR_NULL
};

static void dma_async_device_cleanup(struct kref *kref);

static void dma_dev_release(struct device *dev)
{
	struct dma_chan *chan = to_dma_chan(dev);
	kref_put(&chan->device->refcount, dma_async_device_cleanup);
}

static struct class dma_devclass = {
	.name		= "dma",
	.dev_attrs	= dma_attrs,
	.dev_release	= dma_dev_release,
};

/* --- client and device registration --- */

#define dma_chan_satisfies_mask(chan, mask) \
	__dma_chan_satisfies_mask((chan), &(mask))
static int
__dma_chan_satisfies_mask(struct dma_chan *chan, dma_cap_mask_t *want)
{
	dma_cap_mask_t has;

	bitmap_and(has.bits, want->bits, chan->device->cap_mask.bits,
		DMA_TX_TYPE_END);
	return bitmap_equal(want->bits, has.bits, DMA_TX_TYPE_END);
}

static struct module *dma_chan_to_owner(struct dma_chan *chan)
{
	return chan->device->dev->driver->owner;
}

/**
 * balance_ref_count - catch up the channel reference count
 * @chan - channel to balance ->client_count versus dmaengine_ref_count
 *
 * balance_ref_count must be called under dma_list_mutex
 */
static void balance_ref_count(struct dma_chan *chan)
{
	struct module *owner = dma_chan_to_owner(chan);

	while (chan->client_count < dmaengine_ref_count) {
		__module_get(owner);
		chan->client_count++;
	}
}

/**
 * dma_chan_get - try to grab a dma channel's parent driver module
 * @chan - channel to grab
 *
 * Must be called under dma_list_mutex
 */
static int dma_chan_get(struct dma_chan *chan)
{
	int err = -ENODEV;
	struct module *owner = dma_chan_to_owner(chan);

	if (chan->client_count) {
		__module_get(owner);
		err = 0;
	} else if (try_module_get(owner))
		err = 0;

	if (err == 0)
		chan->client_count++;

	/* allocate upon first client reference */
	if (chan->client_count == 1 && err == 0) {
		int desc_cnt = chan->device->device_alloc_chan_resources(chan, NULL);

		if (desc_cnt < 0) {
			err = desc_cnt;
			chan->client_count = 0;
			module_put(owner);
		} else
			balance_ref_count(chan);
	}

	return err;
}

/**
 * dma_chan_put - drop a reference to a dma channel's parent driver module
 * @chan - channel to release
 *
 * Must be called under dma_list_mutex
 */
static void dma_chan_put(struct dma_chan *chan)
{
	if (!chan->client_count)
		return; /* this channel failed alloc_chan_resources */
	chan->client_count--;
	module_put(dma_chan_to_owner(chan));
	if (chan->client_count == 0)
		chan->device->device_free_chan_resources(chan);
}

/**
 * dma_client_chan_alloc - try to allocate channels to a client
 * @client: &dma_client
 *
 * Called with dma_list_mutex held.
 */
static void dma_client_chan_alloc(struct dma_client *client)
{
	struct dma_device *device;
	struct dma_chan *chan;
	enum dma_state_client ack;

	/* Find a channel */
	list_for_each_entry(device, &dma_device_list, global_node) {
		/* Does the client require a specific DMA controller? */
		if (client->slave && client->slave->dma_dev
				&& client->slave->dma_dev != device->dev)
			continue;

		list_for_each_entry(chan, &device->channels, device_node) {
			if (!dma_chan_satisfies_mask(chan, client->cap_mask))
				continue;
			if (!chan->client_count)
				continue;
			ack = client->event_callback(client, chan,
						     DMA_RESOURCE_AVAILABLE);

			/* we are done once this client rejects
			 * an available resource
			 */
			if (ack == DMA_NAK)
				return;
		}
	}
}

enum dma_status dma_sync_wait(struct dma_chan *chan, dma_cookie_t cookie)
{
	enum dma_status status;
	unsigned long dma_sync_wait_timeout = jiffies + msecs_to_jiffies(5000);

	dma_async_issue_pending(chan);
	do {
		status = dma_async_is_tx_complete(chan, cookie, NULL, NULL);
		if (time_after_eq(jiffies, dma_sync_wait_timeout)) {
			printk(KERN_ERR "dma_sync_wait_timeout!\n");
			return DMA_ERROR;
		}
	} while (status == DMA_IN_PROGRESS);

	return status;
}
EXPORT_SYMBOL(dma_sync_wait);

/**
 * dma_chan_cleanup - release a DMA channel's resources
 * @kref: kernel reference structure that contains the DMA channel device
 */
void dma_chan_cleanup(struct kref *kref)
{
	struct dma_chan *chan = container_of(kref, struct dma_chan, refcount);
	kref_put(&chan->device->refcount, dma_async_device_cleanup);
}
EXPORT_SYMBOL(dma_chan_cleanup);

static void dma_chan_free_rcu(struct rcu_head *rcu)
{
	struct dma_chan *chan = container_of(rcu, struct dma_chan, rcu);

	kref_put(&chan->refcount, dma_chan_cleanup);
}

static void dma_chan_release(struct dma_chan *chan)
{
	call_rcu(&chan->rcu, dma_chan_free_rcu);
}

/**
 * dma_cap_mask_all - enable iteration over all operation types
 */
static dma_cap_mask_t dma_cap_mask_all;

/**
 * dma_chan_tbl_ent - tracks channel allocations per core/operation
 * @chan - associated channel for this entry
 */
struct dma_chan_tbl_ent {
	struct dma_chan *chan;
};

/**
 * channel_table - percpu lookup table for memory-to-memory offload providers
 */
static struct dma_chan_tbl_ent *channel_table[DMA_TX_TYPE_END];

static int __init dma_channel_table_init(void)
{
	enum dma_transaction_type cap;
	int err = 0;

	bitmap_fill(dma_cap_mask_all.bits, DMA_TX_TYPE_END);

	/* 'interrupt' and 'slave' are channel capabilities, but are not
	 * associated with an operation so they do not need an entry in the
	 * channel_table
	 */
	clear_bit(DMA_INTERRUPT, dma_cap_mask_all.bits);
	clear_bit(DMA_SLAVE, dma_cap_mask_all.bits);

	for_each_dma_cap_mask(cap, dma_cap_mask_all) {
		channel_table[cap] = alloc_percpu(struct dma_chan_tbl_ent);
		if (!channel_table[cap]) {
			err = -ENOMEM;
			break;
		}
	}

	if (err) {
		pr_err("dmaengine: initialization failure\n");
		for_each_dma_cap_mask(cap, dma_cap_mask_all)
			if (channel_table[cap])
				free_percpu(channel_table[cap]);
	}

	return err;
}
subsys_initcall(dma_channel_table_init);

/**
 * dma_find_channel - find a channel to carry out the operation
 * @tx_type: transaction type
 */
struct dma_chan *dma_find_channel(enum dma_transaction_type tx_type)
{
	struct dma_chan *chan;
	int cpu;

	WARN_ONCE(dmaengine_ref_count == 0,
		  "client called %s without a reference", __func__);

	cpu = get_cpu();
	chan = per_cpu_ptr(channel_table[tx_type], cpu)->chan;
	put_cpu();

	return chan;
}
EXPORT_SYMBOL(dma_find_channel);

/**
 * nth_chan - returns the nth channel of the given capability
 * @cap: capability to match
 * @n: nth channel desired
 *
 * Defaults to returning the channel with the desired capability and the
 * lowest reference count when 'n' cannot be satisfied.  Must be called
 * under dma_list_mutex.
 */
static struct dma_chan *nth_chan(enum dma_transaction_type cap, int n)
{
	struct dma_device *device;
	struct dma_chan *chan;
	struct dma_chan *ret = NULL;
	struct dma_chan *min = NULL;

	list_for_each_entry(device, &dma_device_list, global_node) {
		if (!dma_has_cap(cap, device->cap_mask))
			continue;
		list_for_each_entry(chan, &device->channels, device_node) {
			if (!chan->client_count)
				continue;
			if (!min)
				min = chan;
			else if (chan->table_count < min->table_count)
				min = chan;

			if (n-- == 0) {
				ret = chan;
				break; /* done */
			}
		}
		if (ret)
			break; /* done */
	}

	if (!ret)
		ret = min;

	if (ret)
		ret->table_count++;

	return ret;
}

/**
 * dma_channel_rebalance - redistribute the available channels
 *
 * Optimize for cpu isolation (each cpu gets a dedicated channel for an
 * operation type) in the SMP case,  and operation isolation (avoid
 * multi-tasking channels) in the non-SMP case.  Must be called under
 * dma_list_mutex.
 */
static void dma_channel_rebalance(void)
{
	struct dma_chan *chan;
	struct dma_device *device;
	int cpu;
	int cap;
	int n;

	/* undo the last distribution */
	for_each_dma_cap_mask(cap, dma_cap_mask_all)
		for_each_possible_cpu(cpu)
			per_cpu_ptr(channel_table[cap], cpu)->chan = NULL;

	list_for_each_entry(device, &dma_device_list, global_node)
		list_for_each_entry(chan, &device->channels, device_node)
			chan->table_count = 0;

	/* don't populate the channel_table if no clients are available */
	if (!dmaengine_ref_count)
		return;

	/* redistribute available channels */
	n = 0;
	for_each_dma_cap_mask(cap, dma_cap_mask_all)
		for_each_online_cpu(cpu) {
			if (num_possible_cpus() > 1)
				chan = nth_chan(cap, n++);
			else
				chan = nth_chan(cap, -1);

			per_cpu_ptr(channel_table[cap], cpu)->chan = chan;
		}
}

/**
 * dma_chans_notify_available - broadcast available channels to the clients
 */
static void dma_clients_notify_available(void)
{
	struct dma_client *client;

	mutex_lock(&dma_list_mutex);

	list_for_each_entry(client, &dma_client_list, global_node)
		dma_client_chan_alloc(client);

	mutex_unlock(&dma_list_mutex);
}

/**
 * dma_async_client_register - register a &dma_client
 * @client: ptr to a client structure with valid 'event_callback' and 'cap_mask'
 */
void dma_async_client_register(struct dma_client *client)
{
	struct dma_device *device, *_d;
	struct dma_chan *chan;
	int err;

	/* validate client data */
	BUG_ON(dma_has_cap(DMA_SLAVE, client->cap_mask) &&
		!client->slave);

	mutex_lock(&dma_list_mutex);
	dmaengine_ref_count++;

	/* try to grab channels */
	list_for_each_entry_safe(device, _d, &dma_device_list, global_node)
		list_for_each_entry(chan, &device->channels, device_node) {
			err = dma_chan_get(chan);
			if (err == -ENODEV) {
				/* module removed before we could use it */
				list_del_init(&device->global_node);
				break;
			} else if (err)
				pr_err("dmaengine: failed to get %s: (%d)\n",
				       dev_name(&chan->dev), err);
		}

	/* if this is the first reference and there were channels
	 * waiting we need to rebalance to get those channels
	 * incorporated into the channel table
	 */
	if (dmaengine_ref_count == 1)
		dma_channel_rebalance();
	list_add_tail(&client->global_node, &dma_client_list);
	mutex_unlock(&dma_list_mutex);
}
EXPORT_SYMBOL(dma_async_client_register);

/**
 * dma_async_client_unregister - unregister a client and free the &dma_client
 * @client: &dma_client to free
 *
 * Force frees any allocated DMA channels, frees the &dma_client memory
 */
void dma_async_client_unregister(struct dma_client *client)
{
	struct dma_device *device;
	struct dma_chan *chan;

	if (!client)
		return;

	mutex_lock(&dma_list_mutex);
	dmaengine_ref_count--;
	BUG_ON(dmaengine_ref_count < 0);
	/* drop channel references */
	list_for_each_entry(device, &dma_device_list, global_node)
		list_for_each_entry(chan, &device->channels, device_node)
			dma_chan_put(chan);

	list_del(&client->global_node);
	mutex_unlock(&dma_list_mutex);
}
EXPORT_SYMBOL(dma_async_client_unregister);

/**
 * dma_async_client_chan_request - send all available channels to the
 * client that satisfy the capability mask
 * @client - requester
 */
void dma_async_client_chan_request(struct dma_client *client)
{
	mutex_lock(&dma_list_mutex);
	dma_client_chan_alloc(client);
	mutex_unlock(&dma_list_mutex);
}
EXPORT_SYMBOL(dma_async_client_chan_request);

/**
 * dma_async_device_register - registers DMA devices found
 * @device: &dma_device
 */
int dma_async_device_register(struct dma_device *device)
{
	static int id;
	int chancnt = 0, rc;
	struct dma_chan* chan;

	if (!device)
		return -ENODEV;

	/* validate device routines */
	BUG_ON(dma_has_cap(DMA_MEMCPY, device->cap_mask) &&
		!device->device_prep_dma_memcpy);
	BUG_ON(dma_has_cap(DMA_XOR, device->cap_mask) &&
		!device->device_prep_dma_xor);
	BUG_ON(dma_has_cap(DMA_ZERO_SUM, device->cap_mask) &&
		!device->device_prep_dma_zero_sum);
	BUG_ON(dma_has_cap(DMA_MEMSET, device->cap_mask) &&
		!device->device_prep_dma_memset);
	BUG_ON(dma_has_cap(DMA_INTERRUPT, device->cap_mask) &&
		!device->device_prep_dma_interrupt);
	BUG_ON(dma_has_cap(DMA_SLAVE, device->cap_mask) &&
		!device->device_prep_slave_sg);
	BUG_ON(dma_has_cap(DMA_SLAVE, device->cap_mask) &&
		!device->device_terminate_all);

	BUG_ON(!device->device_alloc_chan_resources);
	BUG_ON(!device->device_free_chan_resources);
	BUG_ON(!device->device_is_tx_complete);
	BUG_ON(!device->device_issue_pending);
	BUG_ON(!device->dev);

	init_completion(&device->done);
	kref_init(&device->refcount);

	mutex_lock(&dma_list_mutex);
	device->dev_id = id++;
	mutex_unlock(&dma_list_mutex);

	/* represent channels in sysfs. Probably want devs too */
	list_for_each_entry(chan, &device->channels, device_node) {
		chan->local = alloc_percpu(typeof(*chan->local));
		if (chan->local == NULL)
			continue;

		chan->chan_id = chancnt++;
		chan->dev.class = &dma_devclass;
		chan->dev.parent = device->dev;
		dev_set_name(&chan->dev, "dma%dchan%d",
			     device->dev_id, chan->chan_id);

		rc = device_register(&chan->dev);
		if (rc) {
			chancnt--;
			free_percpu(chan->local);
			chan->local = NULL;
			goto err_out;
		}

		/* One for the channel, one of the class device */
		kref_get(&device->refcount);
		kref_get(&device->refcount);
		kref_init(&chan->refcount);
		chan->client_count = 0;
		chan->slow_ref = 0;
		INIT_RCU_HEAD(&chan->rcu);
	}

	mutex_lock(&dma_list_mutex);
	if (dmaengine_ref_count)
		list_for_each_entry(chan, &device->channels, device_node) {
			/* if clients are already waiting for channels we need
			 * to take references on their behalf
			 */
			if (dma_chan_get(chan) == -ENODEV) {
				/* note we can only get here for the first
				 * channel as the remaining channels are
				 * guaranteed to get a reference
				 */
				rc = -ENODEV;
				mutex_unlock(&dma_list_mutex);
				goto err_out;
			}
		}
	list_add_tail(&device->global_node, &dma_device_list);
	dma_channel_rebalance();
	mutex_unlock(&dma_list_mutex);

	dma_clients_notify_available();

	return 0;

err_out:
	list_for_each_entry(chan, &device->channels, device_node) {
		if (chan->local == NULL)
			continue;
		kref_put(&device->refcount, dma_async_device_cleanup);
		device_unregister(&chan->dev);
		chancnt--;
		free_percpu(chan->local);
	}
	return rc;
}
EXPORT_SYMBOL(dma_async_device_register);

/**
 * dma_async_device_cleanup - function called when all references are released
 * @kref: kernel reference object
 */
static void dma_async_device_cleanup(struct kref *kref)
{
	struct dma_device *device;

	device = container_of(kref, struct dma_device, refcount);
	complete(&device->done);
}

/**
 * dma_async_device_unregister - unregister a DMA device
 * @device: &dma_device
 */
void dma_async_device_unregister(struct dma_device *device)
{
	struct dma_chan *chan;

	mutex_lock(&dma_list_mutex);
	list_del(&device->global_node);
	dma_channel_rebalance();
	mutex_unlock(&dma_list_mutex);

	list_for_each_entry(chan, &device->channels, device_node) {
		WARN_ONCE(chan->client_count,
			  "%s called while %d clients hold a reference\n",
			  __func__, chan->client_count);
		device_unregister(&chan->dev);
		dma_chan_release(chan);
	}

	kref_put(&device->refcount, dma_async_device_cleanup);
	wait_for_completion(&device->done);
}
EXPORT_SYMBOL(dma_async_device_unregister);

/**
 * dma_async_memcpy_buf_to_buf - offloaded copy between virtual addresses
 * @chan: DMA channel to offload copy to
 * @dest: destination address (virtual)
 * @src: source address (virtual)
 * @len: length
 *
 * Both @dest and @src must be mappable to a bus address according to the
 * DMA mapping API rules for streaming mappings.
 * Both @dest and @src must stay memory resident (kernel memory or locked
 * user space pages).
 */
dma_cookie_t
dma_async_memcpy_buf_to_buf(struct dma_chan *chan, void *dest,
			void *src, size_t len)
{
	struct dma_device *dev = chan->device;
	struct dma_async_tx_descriptor *tx;
	dma_addr_t dma_dest, dma_src;
	dma_cookie_t cookie;
	int cpu;

	dma_src = dma_map_single(dev->dev, src, len, DMA_TO_DEVICE);
	dma_dest = dma_map_single(dev->dev, dest, len, DMA_FROM_DEVICE);
	tx = dev->device_prep_dma_memcpy(chan, dma_dest, dma_src, len,
					 DMA_CTRL_ACK);

	if (!tx) {
		dma_unmap_single(dev->dev, dma_src, len, DMA_TO_DEVICE);
		dma_unmap_single(dev->dev, dma_dest, len, DMA_FROM_DEVICE);
		return -ENOMEM;
	}

	tx->callback = NULL;
	cookie = tx->tx_submit(tx);

	cpu = get_cpu();
	per_cpu_ptr(chan->local, cpu)->bytes_transferred += len;
	per_cpu_ptr(chan->local, cpu)->memcpy_count++;
	put_cpu();

	return cookie;
}
EXPORT_SYMBOL(dma_async_memcpy_buf_to_buf);

/**
 * dma_async_memcpy_buf_to_pg - offloaded copy from address to page
 * @chan: DMA channel to offload copy to
 * @page: destination page
 * @offset: offset in page to copy to
 * @kdata: source address (virtual)
 * @len: length
 *
 * Both @page/@offset and @kdata must be mappable to a bus address according
 * to the DMA mapping API rules for streaming mappings.
 * Both @page/@offset and @kdata must stay memory resident (kernel memory or
 * locked user space pages)
 */
dma_cookie_t
dma_async_memcpy_buf_to_pg(struct dma_chan *chan, struct page *page,
			unsigned int offset, void *kdata, size_t len)
{
	struct dma_device *dev = chan->device;
	struct dma_async_tx_descriptor *tx;
	dma_addr_t dma_dest, dma_src;
	dma_cookie_t cookie;
	int cpu;

	dma_src = dma_map_single(dev->dev, kdata, len, DMA_TO_DEVICE);
	dma_dest = dma_map_page(dev->dev, page, offset, len, DMA_FROM_DEVICE);
	tx = dev->device_prep_dma_memcpy(chan, dma_dest, dma_src, len,
					 DMA_CTRL_ACK);

	if (!tx) {
		dma_unmap_single(dev->dev, dma_src, len, DMA_TO_DEVICE);
		dma_unmap_page(dev->dev, dma_dest, len, DMA_FROM_DEVICE);
		return -ENOMEM;
	}

	tx->callback = NULL;
	cookie = tx->tx_submit(tx);

	cpu = get_cpu();
	per_cpu_ptr(chan->local, cpu)->bytes_transferred += len;
	per_cpu_ptr(chan->local, cpu)->memcpy_count++;
	put_cpu();

	return cookie;
}
EXPORT_SYMBOL(dma_async_memcpy_buf_to_pg);

/**
 * dma_async_memcpy_pg_to_pg - offloaded copy from page to page
 * @chan: DMA channel to offload copy to
 * @dest_pg: destination page
 * @dest_off: offset in page to copy to
 * @src_pg: source page
 * @src_off: offset in page to copy from
 * @len: length
 *
 * Both @dest_page/@dest_off and @src_page/@src_off must be mappable to a bus
 * address according to the DMA mapping API rules for streaming mappings.
 * Both @dest_page/@dest_off and @src_page/@src_off must stay memory resident
 * (kernel memory or locked user space pages).
 */
dma_cookie_t
dma_async_memcpy_pg_to_pg(struct dma_chan *chan, struct page *dest_pg,
	unsigned int dest_off, struct page *src_pg, unsigned int src_off,
	size_t len)
{
	struct dma_device *dev = chan->device;
	struct dma_async_tx_descriptor *tx;
	dma_addr_t dma_dest, dma_src;
	dma_cookie_t cookie;
	int cpu;

	dma_src = dma_map_page(dev->dev, src_pg, src_off, len, DMA_TO_DEVICE);
	dma_dest = dma_map_page(dev->dev, dest_pg, dest_off, len,
				DMA_FROM_DEVICE);
	tx = dev->device_prep_dma_memcpy(chan, dma_dest, dma_src, len,
					 DMA_CTRL_ACK);

	if (!tx) {
		dma_unmap_page(dev->dev, dma_src, len, DMA_TO_DEVICE);
		dma_unmap_page(dev->dev, dma_dest, len, DMA_FROM_DEVICE);
		return -ENOMEM;
	}

	tx->callback = NULL;
	cookie = tx->tx_submit(tx);

	cpu = get_cpu();
	per_cpu_ptr(chan->local, cpu)->bytes_transferred += len;
	per_cpu_ptr(chan->local, cpu)->memcpy_count++;
	put_cpu();

	return cookie;
}
EXPORT_SYMBOL(dma_async_memcpy_pg_to_pg);

void dma_async_tx_descriptor_init(struct dma_async_tx_descriptor *tx,
	struct dma_chan *chan)
{
	tx->chan = chan;
	spin_lock_init(&tx->lock);
}
EXPORT_SYMBOL(dma_async_tx_descriptor_init);

/* dma_wait_for_async_tx - spin wait for a transaction to complete
 * @tx: in-flight transaction to wait on
 *
 * This routine assumes that tx was obtained from a call to async_memcpy,
 * async_xor, async_memset, etc which ensures that tx is "in-flight" (prepped
 * and submitted).  Walking the parent chain is only meant to cover for DMA
 * drivers that do not implement the DMA_INTERRUPT capability and may race with
 * the driver's descriptor cleanup routine.
 */
enum dma_status
dma_wait_for_async_tx(struct dma_async_tx_descriptor *tx)
{
	enum dma_status status;
	struct dma_async_tx_descriptor *iter;
	struct dma_async_tx_descriptor *parent;

	if (!tx)
		return DMA_SUCCESS;

	WARN_ONCE(tx->parent, "%s: speculatively walking dependency chain for"
		  " %s\n", __func__, dev_name(&tx->chan->dev));

	/* poll through the dependency chain, return when tx is complete */
	do {
		iter = tx;

		/* find the root of the unsubmitted dependency chain */
		do {
			parent = iter->parent;
			if (!parent)
				break;
			else
				iter = parent;
		} while (parent);

		/* there is a small window for ->parent == NULL and
		 * ->cookie == -EBUSY
		 */
		while (iter->cookie == -EBUSY)
			cpu_relax();

		status = dma_sync_wait(iter->chan, iter->cookie);
	} while (status == DMA_IN_PROGRESS || (iter != tx));

	return status;
}
EXPORT_SYMBOL_GPL(dma_wait_for_async_tx);

/* dma_run_dependencies - helper routine for dma drivers to process
 *	(start) dependent operations on their target channel
 * @tx: transaction with dependencies
 */
void dma_run_dependencies(struct dma_async_tx_descriptor *tx)
{
	struct dma_async_tx_descriptor *dep = tx->next;
	struct dma_async_tx_descriptor *dep_next;
	struct dma_chan *chan;

	if (!dep)
		return;

	chan = dep->chan;

	/* keep submitting up until a channel switch is detected
	 * in that case we will be called again as a result of
	 * processing the interrupt from async_tx_channel_switch
	 */
	for (; dep; dep = dep_next) {
		spin_lock_bh(&dep->lock);
		dep->parent = NULL;
		dep_next = dep->next;
		if (dep_next && dep_next->chan == chan)
			dep->next = NULL; /* ->next will be submitted */
		else
			dep_next = NULL; /* submit current dep and terminate */
		spin_unlock_bh(&dep->lock);

		dep->tx_submit(dep);
	}

	chan->device->device_issue_pending(chan);
}
EXPORT_SYMBOL_GPL(dma_run_dependencies);

static int __init dma_bus_init(void)
{
	mutex_init(&dma_list_mutex);
	return class_register(&dma_devclass);
}
subsys_initcall(dma_bus_init);


