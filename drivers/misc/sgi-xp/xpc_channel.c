/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2004-2008 Silicon Graphics, Inc.  All Rights Reserved.
 */

/*
 * Cross Partition Communication (XPC) channel support.
 *
 *	This is the part of XPC that manages the channels and
 *	sends/receives messages across them to/from other partitions.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/cache.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <asm/sn/sn_sal.h>
#include "xpc.h"

/*
 * Guarantee that the kzalloc'd memory is cacheline aligned.
 */
void *
xpc_kzalloc_cacheline_aligned(size_t size, gfp_t flags, void **base)
{
	/* see if kzalloc will give us cachline aligned memory by default */
	*base = kzalloc(size, flags);
	if (*base == NULL)
		return NULL;

	if ((u64)*base == L1_CACHE_ALIGN((u64)*base))
		return *base;

	kfree(*base);

	/* nope, we'll have to do it ourselves */
	*base = kzalloc(size + L1_CACHE_BYTES, flags);
	if (*base == NULL)
		return NULL;

	return (void *)L1_CACHE_ALIGN((u64)*base);
}

/*
 * Allocate the local message queue and the notify queue.
 */
static enum xp_retval
xpc_allocate_local_msgqueue(struct xpc_channel *ch)
{
	unsigned long irq_flags;
	int nentries;
	size_t nbytes;

	for (nentries = ch->local_nentries; nentries > 0; nentries--) {

		nbytes = nentries * ch->msg_size;
		ch->local_msgqueue = xpc_kzalloc_cacheline_aligned(nbytes,
								   GFP_KERNEL,
						      &ch->local_msgqueue_base);
		if (ch->local_msgqueue == NULL)
			continue;

		nbytes = nentries * sizeof(struct xpc_notify);
		ch->notify_queue = kzalloc(nbytes, GFP_KERNEL);
		if (ch->notify_queue == NULL) {
			kfree(ch->local_msgqueue_base);
			ch->local_msgqueue = NULL;
			continue;
		}

		spin_lock_irqsave(&ch->lock, irq_flags);
		if (nentries < ch->local_nentries) {
			dev_dbg(xpc_chan, "nentries=%d local_nentries=%d, "
				"partid=%d, channel=%d\n", nentries,
				ch->local_nentries, ch->partid, ch->number);

			ch->local_nentries = nentries;
		}
		spin_unlock_irqrestore(&ch->lock, irq_flags);
		return xpSuccess;
	}

	dev_dbg(xpc_chan, "can't get memory for local message queue and notify "
		"queue, partid=%d, channel=%d\n", ch->partid, ch->number);
	return xpNoMemory;
}

/*
 * Allocate the cached remote message queue.
 */
static enum xp_retval
xpc_allocate_remote_msgqueue(struct xpc_channel *ch)
{
	unsigned long irq_flags;
	int nentries;
	size_t nbytes;

	DBUG_ON(ch->remote_nentries <= 0);

	for (nentries = ch->remote_nentries; nentries > 0; nentries--) {

		nbytes = nentries * ch->msg_size;
		ch->remote_msgqueue = xpc_kzalloc_cacheline_aligned(nbytes,
								    GFP_KERNEL,
						     &ch->remote_msgqueue_base);
		if (ch->remote_msgqueue == NULL)
			continue;

		spin_lock_irqsave(&ch->lock, irq_flags);
		if (nentries < ch->remote_nentries) {
			dev_dbg(xpc_chan, "nentries=%d remote_nentries=%d, "
				"partid=%d, channel=%d\n", nentries,
				ch->remote_nentries, ch->partid, ch->number);

			ch->remote_nentries = nentries;
		}
		spin_unlock_irqrestore(&ch->lock, irq_flags);
		return xpSuccess;
	}

	dev_dbg(xpc_chan, "can't get memory for cached remote message queue, "
		"partid=%d, channel=%d\n", ch->partid, ch->number);
	return xpNoMemory;
}

/*
 * Allocate message queues and other stuff associated with a channel.
 *
 * Note: Assumes all of the channel sizes are filled in.
 */
static enum xp_retval
xpc_allocate_msgqueues(struct xpc_channel *ch)
{
	unsigned long irq_flags;
	enum xp_retval ret;

	DBUG_ON(ch->flags & XPC_C_SETUP);

	ret = xpc_allocate_local_msgqueue(ch);
	if (ret != xpSuccess)
		return ret;

	ret = xpc_allocate_remote_msgqueue(ch);
	if (ret != xpSuccess) {
		kfree(ch->local_msgqueue_base);
		ch->local_msgqueue = NULL;
		kfree(ch->notify_queue);
		ch->notify_queue = NULL;
		return ret;
	}

	spin_lock_irqsave(&ch->lock, irq_flags);
	ch->flags |= XPC_C_SETUP;
	spin_unlock_irqrestore(&ch->lock, irq_flags);

	return xpSuccess;
}

/*
 * Process a connect message from a remote partition.
 *
 * Note: xpc_process_connect() is expecting to be called with the
 * spin_lock_irqsave held and will leave it locked upon return.
 */
static void
xpc_process_connect(struct xpc_channel *ch, unsigned long *irq_flags)
{
	enum xp_retval ret;

	DBUG_ON(!spin_is_locked(&ch->lock));

	if (!(ch->flags & XPC_C_OPENREQUEST) ||
	    !(ch->flags & XPC_C_ROPENREQUEST)) {
		/* nothing more to do for now */
		return;
	}
	DBUG_ON(!(ch->flags & XPC_C_CONNECTING));

	if (!(ch->flags & XPC_C_SETUP)) {
		spin_unlock_irqrestore(&ch->lock, *irq_flags);
		ret = xpc_allocate_msgqueues(ch);
		spin_lock_irqsave(&ch->lock, *irq_flags);

		if (ret != xpSuccess)
			XPC_DISCONNECT_CHANNEL(ch, ret, irq_flags);

		if (ch->flags & (XPC_C_CONNECTED | XPC_C_DISCONNECTING))
			return;

		DBUG_ON(!(ch->flags & XPC_C_SETUP));
		DBUG_ON(ch->local_msgqueue == NULL);
		DBUG_ON(ch->remote_msgqueue == NULL);
	}

	if (!(ch->flags & XPC_C_OPENREPLY)) {
		ch->flags |= XPC_C_OPENREPLY;
		xpc_IPI_send_openreply(ch, irq_flags);
	}

	if (!(ch->flags & XPC_C_ROPENREPLY))
		return;

	DBUG_ON(ch->remote_msgqueue_pa == 0);

	ch->flags = (XPC_C_CONNECTED | XPC_C_SETUP);	/* clear all else */

	dev_info(xpc_chan, "channel %d to partition %d connected\n",
		 ch->number, ch->partid);

	spin_unlock_irqrestore(&ch->lock, *irq_flags);
	xpc_create_kthreads(ch, 1, 0);
	spin_lock_irqsave(&ch->lock, *irq_flags);
}

/*
 * Notify those who wanted to be notified upon delivery of their message.
 */
static void
xpc_notify_senders(struct xpc_channel *ch, enum xp_retval reason, s64 put)
{
	struct xpc_notify *notify;
	u8 notify_type;
	s64 get = ch->w_remote_GP.get - 1;

	while (++get < put && atomic_read(&ch->n_to_notify) > 0) {

		notify = &ch->notify_queue[get % ch->local_nentries];

		/*
		 * See if the notify entry indicates it was associated with
		 * a message who's sender wants to be notified. It is possible
		 * that it is, but someone else is doing or has done the
		 * notification.
		 */
		notify_type = notify->type;
		if (notify_type == 0 ||
		    cmpxchg(&notify->type, notify_type, 0) != notify_type) {
			continue;
		}

		DBUG_ON(notify_type != XPC_N_CALL);

		atomic_dec(&ch->n_to_notify);

		if (notify->func != NULL) {
			dev_dbg(xpc_chan, "notify->func() called, notify=0x%p, "
				"msg_number=%ld, partid=%d, channel=%d\n",
				(void *)notify, get, ch->partid, ch->number);

			notify->func(reason, ch->partid, ch->number,
				     notify->key);

			dev_dbg(xpc_chan, "notify->func() returned, "
				"notify=0x%p, msg_number=%ld, partid=%d, "
				"channel=%d\n", (void *)notify, get,
				ch->partid, ch->number);
		}
	}
}

/*
 * Free up message queues and other stuff that were allocated for the specified
 * channel.
 *
 * Note: ch->reason and ch->reason_line are left set for debugging purposes,
 * they're cleared when XPC_C_DISCONNECTED is cleared.
 */
static void
xpc_free_msgqueues(struct xpc_channel *ch)
{
	DBUG_ON(!spin_is_locked(&ch->lock));
	DBUG_ON(atomic_read(&ch->n_to_notify) != 0);

	ch->remote_msgqueue_pa = 0;
	ch->func = NULL;
	ch->key = NULL;
	ch->msg_size = 0;
	ch->local_nentries = 0;
	ch->remote_nentries = 0;
	ch->kthreads_assigned_limit = 0;
	ch->kthreads_idle_limit = 0;

	ch->local_GP->get = 0;
	ch->local_GP->put = 0;
	ch->remote_GP.get = 0;
	ch->remote_GP.put = 0;
	ch->w_local_GP.get = 0;
	ch->w_local_GP.put = 0;
	ch->w_remote_GP.get = 0;
	ch->w_remote_GP.put = 0;
	ch->next_msg_to_pull = 0;

	if (ch->flags & XPC_C_SETUP) {
		ch->flags &= ~XPC_C_SETUP;

		dev_dbg(xpc_chan, "ch->flags=0x%x, partid=%d, channel=%d\n",
			ch->flags, ch->partid, ch->number);

		kfree(ch->local_msgqueue_base);
		ch->local_msgqueue = NULL;
		kfree(ch->remote_msgqueue_base);
		ch->remote_msgqueue = NULL;
		kfree(ch->notify_queue);
		ch->notify_queue = NULL;
	}
}

/*
 * spin_lock_irqsave() is expected to be held on entry.
 */
static void
xpc_process_disconnect(struct xpc_channel *ch, unsigned long *irq_flags)
{
	struct xpc_partition *part = &xpc_partitions[ch->partid];
	u32 channel_was_connected = (ch->flags & XPC_C_WASCONNECTED);

	DBUG_ON(!spin_is_locked(&ch->lock));

	if (!(ch->flags & XPC_C_DISCONNECTING))
		return;

	DBUG_ON(!(ch->flags & XPC_C_CLOSEREQUEST));

	/* make sure all activity has settled down first */

	if (atomic_read(&ch->kthreads_assigned) > 0 ||
	    atomic_read(&ch->references) > 0) {
		return;
	}
	DBUG_ON((ch->flags & XPC_C_CONNECTEDCALLOUT_MADE) &&
		!(ch->flags & XPC_C_DISCONNECTINGCALLOUT_MADE));

	if (part->act_state == XPC_P_DEACTIVATING) {
		/* can't proceed until the other side disengages from us */
		if (xpc_partition_engaged(1UL << ch->partid))
			return;

	} else {

		/* as long as the other side is up do the full protocol */

		if (!(ch->flags & XPC_C_RCLOSEREQUEST))
			return;

		if (!(ch->flags & XPC_C_CLOSEREPLY)) {
			ch->flags |= XPC_C_CLOSEREPLY;
			xpc_IPI_send_closereply(ch, irq_flags);
		}

		if (!(ch->flags & XPC_C_RCLOSEREPLY))
			return;
	}

	/* wake those waiting for notify completion */
	if (atomic_read(&ch->n_to_notify) > 0) {
		/* >>> we do callout while holding ch->lock */
		xpc_notify_senders(ch, ch->reason, ch->w_local_GP.put);
	}

	/* both sides are disconnected now */

	if (ch->flags & XPC_C_DISCONNECTINGCALLOUT_MADE) {
		spin_unlock_irqrestore(&ch->lock, *irq_flags);
		xpc_disconnect_callout(ch, xpDisconnected);
		spin_lock_irqsave(&ch->lock, *irq_flags);
	}

	/* it's now safe to free the channel's message queues */
	xpc_free_msgqueues(ch);

	/* mark disconnected, clear all other flags except XPC_C_WDISCONNECT */
	ch->flags = (XPC_C_DISCONNECTED | (ch->flags & XPC_C_WDISCONNECT));

	atomic_dec(&part->nchannels_active);

	if (channel_was_connected) {
		dev_info(xpc_chan, "channel %d to partition %d disconnected, "
			 "reason=%d\n", ch->number, ch->partid, ch->reason);
	}

	if (ch->flags & XPC_C_WDISCONNECT) {
		/* we won't lose the CPU since we're holding ch->lock */
		complete(&ch->wdisconnect_wait);
	} else if (ch->delayed_IPI_flags) {
		if (part->act_state != XPC_P_DEACTIVATING) {
			/* time to take action on any delayed IPI flags */
			spin_lock(&part->IPI_lock);
			XPC_SET_IPI_FLAGS(part->local_IPI_amo, ch->number,
					  ch->delayed_IPI_flags);
			spin_unlock(&part->IPI_lock);
		}
		ch->delayed_IPI_flags = 0;
	}
}

/*
 * Process a change in the channel's remote connection state.
 */
static void
xpc_process_openclose_IPI(struct xpc_partition *part, int ch_number,
			  u8 IPI_flags)
{
	unsigned long irq_flags;
	struct xpc_openclose_args *args =
	    &part->remote_openclose_args[ch_number];
	struct xpc_channel *ch = &part->channels[ch_number];
	enum xp_retval reason;

	spin_lock_irqsave(&ch->lock, irq_flags);

again:

	if ((ch->flags & XPC_C_DISCONNECTED) &&
	    (ch->flags & XPC_C_WDISCONNECT)) {
		/*
		 * Delay processing IPI flags until thread waiting disconnect
		 * has had a chance to see that the channel is disconnected.
		 */
		ch->delayed_IPI_flags |= IPI_flags;
		spin_unlock_irqrestore(&ch->lock, irq_flags);
		return;
	}

	if (IPI_flags & XPC_IPI_CLOSEREQUEST) {

		dev_dbg(xpc_chan, "XPC_IPI_CLOSEREQUEST (reason=%d) received "
			"from partid=%d, channel=%d\n", args->reason,
			ch->partid, ch->number);

		/*
		 * If RCLOSEREQUEST is set, we're probably waiting for
		 * RCLOSEREPLY. We should find it and a ROPENREQUEST packed
		 * with this RCLOSEREQUEST in the IPI_flags.
		 */

		if (ch->flags & XPC_C_RCLOSEREQUEST) {
			DBUG_ON(!(ch->flags & XPC_C_DISCONNECTING));
			DBUG_ON(!(ch->flags & XPC_C_CLOSEREQUEST));
			DBUG_ON(!(ch->flags & XPC_C_CLOSEREPLY));
			DBUG_ON(ch->flags & XPC_C_RCLOSEREPLY);

			DBUG_ON(!(IPI_flags & XPC_IPI_CLOSEREPLY));
			IPI_flags &= ~XPC_IPI_CLOSEREPLY;
			ch->flags |= XPC_C_RCLOSEREPLY;

			/* both sides have finished disconnecting */
			xpc_process_disconnect(ch, &irq_flags);
			DBUG_ON(!(ch->flags & XPC_C_DISCONNECTED));
			goto again;
		}

		if (ch->flags & XPC_C_DISCONNECTED) {
			if (!(IPI_flags & XPC_IPI_OPENREQUEST)) {
				if ((XPC_GET_IPI_FLAGS(part->local_IPI_amo,
						       ch_number) &
				     XPC_IPI_OPENREQUEST)) {

					DBUG_ON(ch->delayed_IPI_flags != 0);
					spin_lock(&part->IPI_lock);
					XPC_SET_IPI_FLAGS(part->local_IPI_amo,
							  ch_number,
							  XPC_IPI_CLOSEREQUEST);
					spin_unlock(&part->IPI_lock);
				}
				spin_unlock_irqrestore(&ch->lock, irq_flags);
				return;
			}

			XPC_SET_REASON(ch, 0, 0);
			ch->flags &= ~XPC_C_DISCONNECTED;

			atomic_inc(&part->nchannels_active);
			ch->flags |= (XPC_C_CONNECTING | XPC_C_ROPENREQUEST);
		}

		IPI_flags &= ~(XPC_IPI_OPENREQUEST | XPC_IPI_OPENREPLY);

		/*
		 * The meaningful CLOSEREQUEST connection state fields are:
		 *      reason = reason connection is to be closed
		 */

		ch->flags |= XPC_C_RCLOSEREQUEST;

		if (!(ch->flags & XPC_C_DISCONNECTING)) {
			reason = args->reason;
			if (reason <= xpSuccess || reason > xpUnknownReason)
				reason = xpUnknownReason;
			else if (reason == xpUnregistering)
				reason = xpOtherUnregistering;

			XPC_DISCONNECT_CHANNEL(ch, reason, &irq_flags);

			DBUG_ON(IPI_flags & XPC_IPI_CLOSEREPLY);
			spin_unlock_irqrestore(&ch->lock, irq_flags);
			return;
		}

		xpc_process_disconnect(ch, &irq_flags);
	}

	if (IPI_flags & XPC_IPI_CLOSEREPLY) {

		dev_dbg(xpc_chan, "XPC_IPI_CLOSEREPLY received from partid=%d,"
			" channel=%d\n", ch->partid, ch->number);

		if (ch->flags & XPC_C_DISCONNECTED) {
			DBUG_ON(part->act_state != XPC_P_DEACTIVATING);
			spin_unlock_irqrestore(&ch->lock, irq_flags);
			return;
		}

		DBUG_ON(!(ch->flags & XPC_C_CLOSEREQUEST));

		if (!(ch->flags & XPC_C_RCLOSEREQUEST)) {
			if ((XPC_GET_IPI_FLAGS(part->local_IPI_amo, ch_number)
			     & XPC_IPI_CLOSEREQUEST)) {

				DBUG_ON(ch->delayed_IPI_flags != 0);
				spin_lock(&part->IPI_lock);
				XPC_SET_IPI_FLAGS(part->local_IPI_amo,
						  ch_number,
						  XPC_IPI_CLOSEREPLY);
				spin_unlock(&part->IPI_lock);
			}
			spin_unlock_irqrestore(&ch->lock, irq_flags);
			return;
		}

		ch->flags |= XPC_C_RCLOSEREPLY;

		if (ch->flags & XPC_C_CLOSEREPLY) {
			/* both sides have finished disconnecting */
			xpc_process_disconnect(ch, &irq_flags);
		}
	}

	if (IPI_flags & XPC_IPI_OPENREQUEST) {

		dev_dbg(xpc_chan, "XPC_IPI_OPENREQUEST (msg_size=%d, "
			"local_nentries=%d) received from partid=%d, "
			"channel=%d\n", args->msg_size, args->local_nentries,
			ch->partid, ch->number);

		if (part->act_state == XPC_P_DEACTIVATING ||
		    (ch->flags & XPC_C_ROPENREQUEST)) {
			spin_unlock_irqrestore(&ch->lock, irq_flags);
			return;
		}

		if (ch->flags & (XPC_C_DISCONNECTING | XPC_C_WDISCONNECT)) {
			ch->delayed_IPI_flags |= XPC_IPI_OPENREQUEST;
			spin_unlock_irqrestore(&ch->lock, irq_flags);
			return;
		}
		DBUG_ON(!(ch->flags & (XPC_C_DISCONNECTED |
				       XPC_C_OPENREQUEST)));
		DBUG_ON(ch->flags & (XPC_C_ROPENREQUEST | XPC_C_ROPENREPLY |
				     XPC_C_OPENREPLY | XPC_C_CONNECTED));

		/*
		 * The meaningful OPENREQUEST connection state fields are:
		 *      msg_size = size of channel's messages in bytes
		 *      local_nentries = remote partition's local_nentries
		 */
		if (args->msg_size == 0 || args->local_nentries == 0) {
			/* assume OPENREQUEST was delayed by mistake */
			spin_unlock_irqrestore(&ch->lock, irq_flags);
			return;
		}

		ch->flags |= (XPC_C_ROPENREQUEST | XPC_C_CONNECTING);
		ch->remote_nentries = args->local_nentries;

		if (ch->flags & XPC_C_OPENREQUEST) {
			if (args->msg_size != ch->msg_size) {
				XPC_DISCONNECT_CHANNEL(ch, xpUnequalMsgSizes,
						       &irq_flags);
				spin_unlock_irqrestore(&ch->lock, irq_flags);
				return;
			}
		} else {
			ch->msg_size = args->msg_size;

			XPC_SET_REASON(ch, 0, 0);
			ch->flags &= ~XPC_C_DISCONNECTED;

			atomic_inc(&part->nchannels_active);
		}

		xpc_process_connect(ch, &irq_flags);
	}

	if (IPI_flags & XPC_IPI_OPENREPLY) {

		dev_dbg(xpc_chan, "XPC_IPI_OPENREPLY (local_msgqueue_pa=0x%lx, "
			"local_nentries=%d, remote_nentries=%d) received from "
			"partid=%d, channel=%d\n", args->local_msgqueue_pa,
			args->local_nentries, args->remote_nentries,
			ch->partid, ch->number);

		if (ch->flags & (XPC_C_DISCONNECTING | XPC_C_DISCONNECTED)) {
			spin_unlock_irqrestore(&ch->lock, irq_flags);
			return;
		}
		if (!(ch->flags & XPC_C_OPENREQUEST)) {
			XPC_DISCONNECT_CHANNEL(ch, xpOpenCloseError,
					       &irq_flags);
			spin_unlock_irqrestore(&ch->lock, irq_flags);
			return;
		}

		DBUG_ON(!(ch->flags & XPC_C_ROPENREQUEST));
		DBUG_ON(ch->flags & XPC_C_CONNECTED);

		/*
		 * The meaningful OPENREPLY connection state fields are:
		 *      local_msgqueue_pa = physical address of remote
		 *                          partition's local_msgqueue
		 *      local_nentries = remote partition's local_nentries
		 *      remote_nentries = remote partition's remote_nentries
		 */
		DBUG_ON(args->local_msgqueue_pa == 0);
		DBUG_ON(args->local_nentries == 0);
		DBUG_ON(args->remote_nentries == 0);

		ch->flags |= XPC_C_ROPENREPLY;
		ch->remote_msgqueue_pa = args->local_msgqueue_pa;

		if (args->local_nentries < ch->remote_nentries) {
			dev_dbg(xpc_chan, "XPC_IPI_OPENREPLY: new "
				"remote_nentries=%d, old remote_nentries=%d, "
				"partid=%d, channel=%d\n",
				args->local_nentries, ch->remote_nentries,
				ch->partid, ch->number);

			ch->remote_nentries = args->local_nentries;
		}
		if (args->remote_nentries < ch->local_nentries) {
			dev_dbg(xpc_chan, "XPC_IPI_OPENREPLY: new "
				"local_nentries=%d, old local_nentries=%d, "
				"partid=%d, channel=%d\n",
				args->remote_nentries, ch->local_nentries,
				ch->partid, ch->number);

			ch->local_nentries = args->remote_nentries;
		}

		xpc_process_connect(ch, &irq_flags);
	}

	spin_unlock_irqrestore(&ch->lock, irq_flags);
}

/*
 * Attempt to establish a channel connection to a remote partition.
 */
static enum xp_retval
xpc_connect_channel(struct xpc_channel *ch)
{
	unsigned long irq_flags;
	struct xpc_registration *registration = &xpc_registrations[ch->number];

	if (mutex_trylock(&registration->mutex) == 0)
		return xpRetry;

	if (!XPC_CHANNEL_REGISTERED(ch->number)) {
		mutex_unlock(&registration->mutex);
		return xpUnregistered;
	}

	spin_lock_irqsave(&ch->lock, irq_flags);

	DBUG_ON(ch->flags & XPC_C_CONNECTED);
	DBUG_ON(ch->flags & XPC_C_OPENREQUEST);

	if (ch->flags & XPC_C_DISCONNECTING) {
		spin_unlock_irqrestore(&ch->lock, irq_flags);
		mutex_unlock(&registration->mutex);
		return ch->reason;
	}

	/* add info from the channel connect registration to the channel */

	ch->kthreads_assigned_limit = registration->assigned_limit;
	ch->kthreads_idle_limit = registration->idle_limit;
	DBUG_ON(atomic_read(&ch->kthreads_assigned) != 0);
	DBUG_ON(atomic_read(&ch->kthreads_idle) != 0);
	DBUG_ON(atomic_read(&ch->kthreads_active) != 0);

	ch->func = registration->func;
	DBUG_ON(registration->func == NULL);
	ch->key = registration->key;

	ch->local_nentries = registration->nentries;

	if (ch->flags & XPC_C_ROPENREQUEST) {
		if (registration->msg_size != ch->msg_size) {
			/* the local and remote sides aren't the same */

			/*
			 * Because XPC_DISCONNECT_CHANNEL() can block we're
			 * forced to up the registration sema before we unlock
			 * the channel lock. But that's okay here because we're
			 * done with the part that required the registration
			 * sema. XPC_DISCONNECT_CHANNEL() requires that the
			 * channel lock be locked and will unlock and relock
			 * the channel lock as needed.
			 */
			mutex_unlock(&registration->mutex);
			XPC_DISCONNECT_CHANNEL(ch, xpUnequalMsgSizes,
					       &irq_flags);
			spin_unlock_irqrestore(&ch->lock, irq_flags);
			return xpUnequalMsgSizes;
		}
	} else {
		ch->msg_size = registration->msg_size;

		XPC_SET_REASON(ch, 0, 0);
		ch->flags &= ~XPC_C_DISCONNECTED;

		atomic_inc(&xpc_partitions[ch->partid].nchannels_active);
	}

	mutex_unlock(&registration->mutex);

	/* initiate the connection */

	ch->flags |= (XPC_C_OPENREQUEST | XPC_C_CONNECTING);
	xpc_IPI_send_openrequest(ch, &irq_flags);

	xpc_process_connect(ch, &irq_flags);

	spin_unlock_irqrestore(&ch->lock, irq_flags);

	return xpSuccess;
}

/*
 * Clear some of the msg flags in the local message queue.
 */
static inline void
xpc_clear_local_msgqueue_flags(struct xpc_channel *ch)
{
	struct xpc_msg *msg;
	s64 get;

	get = ch->w_remote_GP.get;
	do {
		msg = (struct xpc_msg *)((u64)ch->local_msgqueue +
					 (get % ch->local_nentries) *
					 ch->msg_size);
		msg->flags = 0;
	} while (++get < ch->remote_GP.get);
}

/*
 * Clear some of the msg flags in the remote message queue.
 */
static inline void
xpc_clear_remote_msgqueue_flags(struct xpc_channel *ch)
{
	struct xpc_msg *msg;
	s64 put;

	put = ch->w_remote_GP.put;
	do {
		msg = (struct xpc_msg *)((u64)ch->remote_msgqueue +
					 (put % ch->remote_nentries) *
					 ch->msg_size);
		msg->flags = 0;
	} while (++put < ch->remote_GP.put);
}

static void
xpc_process_msg_IPI(struct xpc_partition *part, int ch_number)
{
	struct xpc_channel *ch = &part->channels[ch_number];
	int nmsgs_sent;

	ch->remote_GP = part->remote_GPs[ch_number];

	/* See what, if anything, has changed for each connected channel */

	xpc_msgqueue_ref(ch);

	if (ch->w_remote_GP.get == ch->remote_GP.get &&
	    ch->w_remote_GP.put == ch->remote_GP.put) {
		/* nothing changed since GPs were last pulled */
		xpc_msgqueue_deref(ch);
		return;
	}

	if (!(ch->flags & XPC_C_CONNECTED)) {
		xpc_msgqueue_deref(ch);
		return;
	}

	/*
	 * First check to see if messages recently sent by us have been
	 * received by the other side. (The remote GET value will have
	 * changed since we last looked at it.)
	 */

	if (ch->w_remote_GP.get != ch->remote_GP.get) {

		/*
		 * We need to notify any senders that want to be notified
		 * that their sent messages have been received by their
		 * intended recipients. We need to do this before updating
		 * w_remote_GP.get so that we don't allocate the same message
		 * queue entries prematurely (see xpc_allocate_msg()).
		 */
		if (atomic_read(&ch->n_to_notify) > 0) {
			/*
			 * Notify senders that messages sent have been
			 * received and delivered by the other side.
			 */
			xpc_notify_senders(ch, xpMsgDelivered,
					   ch->remote_GP.get);
		}

		/*
		 * Clear msg->flags in previously sent messages, so that
		 * they're ready for xpc_allocate_msg().
		 */
		xpc_clear_local_msgqueue_flags(ch);

		ch->w_remote_GP.get = ch->remote_GP.get;

		dev_dbg(xpc_chan, "w_remote_GP.get changed to %ld, partid=%d, "
			"channel=%d\n", ch->w_remote_GP.get, ch->partid,
			ch->number);

		/*
		 * If anyone was waiting for message queue entries to become
		 * available, wake them up.
		 */
		if (atomic_read(&ch->n_on_msg_allocate_wq) > 0)
			wake_up(&ch->msg_allocate_wq);
	}

	/*
	 * Now check for newly sent messages by the other side. (The remote
	 * PUT value will have changed since we last looked at it.)
	 */

	if (ch->w_remote_GP.put != ch->remote_GP.put) {
		/*
		 * Clear msg->flags in previously received messages, so that
		 * they're ready for xpc_get_deliverable_msg().
		 */
		xpc_clear_remote_msgqueue_flags(ch);

		ch->w_remote_GP.put = ch->remote_GP.put;

		dev_dbg(xpc_chan, "w_remote_GP.put changed to %ld, partid=%d, "
			"channel=%d\n", ch->w_remote_GP.put, ch->partid,
			ch->number);

		nmsgs_sent = ch->w_remote_GP.put - ch->w_local_GP.get;
		if (nmsgs_sent > 0) {
			dev_dbg(xpc_chan, "msgs waiting to be copied and "
				"delivered=%d, partid=%d, channel=%d\n",
				nmsgs_sent, ch->partid, ch->number);

			if (ch->flags & XPC_C_CONNECTEDCALLOUT_MADE)
				xpc_activate_kthreads(ch, nmsgs_sent);
		}
	}

	xpc_msgqueue_deref(ch);
}

void
xpc_process_channel_activity(struct xpc_partition *part)
{
	unsigned long irq_flags;
	u64 IPI_amo, IPI_flags;
	struct xpc_channel *ch;
	int ch_number;
	u32 ch_flags;

	IPI_amo = xpc_get_IPI_flags(part);

	/*
	 * Initiate channel connections for registered channels.
	 *
	 * For each connected channel that has pending messages activate idle
	 * kthreads and/or create new kthreads as needed.
	 */

	for (ch_number = 0; ch_number < part->nchannels; ch_number++) {
		ch = &part->channels[ch_number];

		/*
		 * Process any open or close related IPI flags, and then deal
		 * with connecting or disconnecting the channel as required.
		 */

		IPI_flags = XPC_GET_IPI_FLAGS(IPI_amo, ch_number);

		if (XPC_ANY_OPENCLOSE_IPI_FLAGS_SET(IPI_flags))
			xpc_process_openclose_IPI(part, ch_number, IPI_flags);

		ch_flags = ch->flags;	/* need an atomic snapshot of flags */

		if (ch_flags & XPC_C_DISCONNECTING) {
			spin_lock_irqsave(&ch->lock, irq_flags);
			xpc_process_disconnect(ch, &irq_flags);
			spin_unlock_irqrestore(&ch->lock, irq_flags);
			continue;
		}

		if (part->act_state == XPC_P_DEACTIVATING)
			continue;

		if (!(ch_flags & XPC_C_CONNECTED)) {
			if (!(ch_flags & XPC_C_OPENREQUEST)) {
				DBUG_ON(ch_flags & XPC_C_SETUP);
				(void)xpc_connect_channel(ch);
			} else {
				spin_lock_irqsave(&ch->lock, irq_flags);
				xpc_process_connect(ch, &irq_flags);
				spin_unlock_irqrestore(&ch->lock, irq_flags);
			}
			continue;
		}

		/*
		 * Process any message related IPI flags, this may involve the
		 * activation of kthreads to deliver any pending messages sent
		 * from the other partition.
		 */

		if (XPC_ANY_MSG_IPI_FLAGS_SET(IPI_flags))
			xpc_process_msg_IPI(part, ch_number);
	}
}

/*
 * XPC's heartbeat code calls this function to inform XPC that a partition is
 * going down.  XPC responds by tearing down the XPartition Communication
 * infrastructure used for the just downed partition.
 *
 * XPC's heartbeat code will never call this function and xpc_partition_up()
 * at the same time. Nor will it ever make multiple calls to either function
 * at the same time.
 */
void
xpc_partition_going_down(struct xpc_partition *part, enum xp_retval reason)
{
	unsigned long irq_flags;
	int ch_number;
	struct xpc_channel *ch;

	dev_dbg(xpc_chan, "deactivating partition %d, reason=%d\n",
		XPC_PARTID(part), reason);

	if (!xpc_part_ref(part)) {
		/* infrastructure for this partition isn't currently set up */
		return;
	}

	/* disconnect channels associated with the partition going down */

	for (ch_number = 0; ch_number < part->nchannels; ch_number++) {
		ch = &part->channels[ch_number];

		xpc_msgqueue_ref(ch);
		spin_lock_irqsave(&ch->lock, irq_flags);

		XPC_DISCONNECT_CHANNEL(ch, reason, &irq_flags);

		spin_unlock_irqrestore(&ch->lock, irq_flags);
		xpc_msgqueue_deref(ch);
	}

	xpc_wakeup_channel_mgr(part);

	xpc_part_deref(part);
}

/*
 * Called by XP at the time of channel connection registration to cause
 * XPC to establish connections to all currently active partitions.
 */
void
xpc_initiate_connect(int ch_number)
{
	short partid;
	struct xpc_partition *part;
	struct xpc_channel *ch;

	DBUG_ON(ch_number < 0 || ch_number >= XPC_MAX_NCHANNELS);

	for (partid = 0; partid < xp_max_npartitions; partid++) {
		part = &xpc_partitions[partid];

		if (xpc_part_ref(part)) {
			ch = &part->channels[ch_number];

			/*
			 * Initiate the establishment of a connection on the
			 * newly registered channel to the remote partition.
			 */
			xpc_wakeup_channel_mgr(part);
			xpc_part_deref(part);
		}
	}
}

void
xpc_connected_callout(struct xpc_channel *ch)
{
	/* let the registerer know that a connection has been established */

	if (ch->func != NULL) {
		dev_dbg(xpc_chan, "ch->func() called, reason=xpConnected, "
			"partid=%d, channel=%d\n", ch->partid, ch->number);

		ch->func(xpConnected, ch->partid, ch->number,
			 (void *)(u64)ch->local_nentries, ch->key);

		dev_dbg(xpc_chan, "ch->func() returned, reason=xpConnected, "
			"partid=%d, channel=%d\n", ch->partid, ch->number);
	}
}

/*
 * Called by XP at the time of channel connection unregistration to cause
 * XPC to teardown all current connections for the specified channel.
 *
 * Before returning xpc_initiate_disconnect() will wait until all connections
 * on the specified channel have been closed/torndown. So the caller can be
 * assured that they will not be receiving any more callouts from XPC to the
 * function they registered via xpc_connect().
 *
 * Arguments:
 *
 *	ch_number - channel # to unregister.
 */
void
xpc_initiate_disconnect(int ch_number)
{
	unsigned long irq_flags;
	short partid;
	struct xpc_partition *part;
	struct xpc_channel *ch;

	DBUG_ON(ch_number < 0 || ch_number >= XPC_MAX_NCHANNELS);

	/* initiate the channel disconnect for every active partition */
	for (partid = 0; partid < xp_max_npartitions; partid++) {
		part = &xpc_partitions[partid];

		if (xpc_part_ref(part)) {
			ch = &part->channels[ch_number];
			xpc_msgqueue_ref(ch);

			spin_lock_irqsave(&ch->lock, irq_flags);

			if (!(ch->flags & XPC_C_DISCONNECTED)) {
				ch->flags |= XPC_C_WDISCONNECT;

				XPC_DISCONNECT_CHANNEL(ch, xpUnregistering,
						       &irq_flags);
			}

			spin_unlock_irqrestore(&ch->lock, irq_flags);

			xpc_msgqueue_deref(ch);
			xpc_part_deref(part);
		}
	}

	xpc_disconnect_wait(ch_number);
}

/*
 * To disconnect a channel, and reflect it back to all who may be waiting.
 *
 * An OPEN is not allowed until XPC_C_DISCONNECTING is cleared by
 * xpc_process_disconnect(), and if set, XPC_C_WDISCONNECT is cleared by
 * xpc_disconnect_wait().
 *
 * THE CHANNEL IS TO BE LOCKED BY THE CALLER AND WILL REMAIN LOCKED UPON RETURN.
 */
void
xpc_disconnect_channel(const int line, struct xpc_channel *ch,
		       enum xp_retval reason, unsigned long *irq_flags)
{
	u32 channel_was_connected = (ch->flags & XPC_C_CONNECTED);

	DBUG_ON(!spin_is_locked(&ch->lock));

	if (ch->flags & (XPC_C_DISCONNECTING | XPC_C_DISCONNECTED))
		return;

	DBUG_ON(!(ch->flags & (XPC_C_CONNECTING | XPC_C_CONNECTED)));

	dev_dbg(xpc_chan, "reason=%d, line=%d, partid=%d, channel=%d\n",
		reason, line, ch->partid, ch->number);

	XPC_SET_REASON(ch, reason, line);

	ch->flags |= (XPC_C_CLOSEREQUEST | XPC_C_DISCONNECTING);
	/* some of these may not have been set */
	ch->flags &= ~(XPC_C_OPENREQUEST | XPC_C_OPENREPLY |
		       XPC_C_ROPENREQUEST | XPC_C_ROPENREPLY |
		       XPC_C_CONNECTING | XPC_C_CONNECTED);

	xpc_IPI_send_closerequest(ch, irq_flags);

	if (channel_was_connected)
		ch->flags |= XPC_C_WASCONNECTED;

	spin_unlock_irqrestore(&ch->lock, *irq_flags);

	/* wake all idle kthreads so they can exit */
	if (atomic_read(&ch->kthreads_idle) > 0) {
		wake_up_all(&ch->idle_wq);

	} else if ((ch->flags & XPC_C_CONNECTEDCALLOUT_MADE) &&
		   !(ch->flags & XPC_C_DISCONNECTINGCALLOUT)) {
		/* start a kthread that will do the xpDisconnecting callout */
		xpc_create_kthreads(ch, 1, 1);
	}

	/* wake those waiting to allocate an entry from the local msg queue */
	if (atomic_read(&ch->n_on_msg_allocate_wq) > 0)
		wake_up(&ch->msg_allocate_wq);

	spin_lock_irqsave(&ch->lock, *irq_flags);
}

void
xpc_disconnect_callout(struct xpc_channel *ch, enum xp_retval reason)
{
	/*
	 * Let the channel's registerer know that the channel is being
	 * disconnected. We don't want to do this if the registerer was never
	 * informed of a connection being made.
	 */

	if (ch->func != NULL) {
		dev_dbg(xpc_chan, "ch->func() called, reason=%d, partid=%d, "
			"channel=%d\n", reason, ch->partid, ch->number);

		ch->func(reason, ch->partid, ch->number, NULL, ch->key);

		dev_dbg(xpc_chan, "ch->func() returned, reason=%d, partid=%d, "
			"channel=%d\n", reason, ch->partid, ch->number);
	}
}

/*
 * Wait for a message entry to become available for the specified channel,
 * but don't wait any longer than 1 jiffy.
 */
enum xp_retval
xpc_allocate_msg_wait(struct xpc_channel *ch)
{
	enum xp_retval ret;

	if (ch->flags & XPC_C_DISCONNECTING) {
		DBUG_ON(ch->reason == xpInterrupted);
		return ch->reason;
	}

	atomic_inc(&ch->n_on_msg_allocate_wq);
	ret = interruptible_sleep_on_timeout(&ch->msg_allocate_wq, 1);
	atomic_dec(&ch->n_on_msg_allocate_wq);

	if (ch->flags & XPC_C_DISCONNECTING) {
		ret = ch->reason;
		DBUG_ON(ch->reason == xpInterrupted);
	} else if (ret == 0) {
		ret = xpTimeout;
	} else {
		ret = xpInterrupted;
	}

	return ret;
}

/*
 * Allocate an entry for a message from the message queue associated with the
 * specified channel. NOTE that this routine can sleep waiting for a message
 * entry to become available. To not sleep, pass in the XPC_NOWAIT flag.
 *
 * Arguments:
 *
 *	partid - ID of partition to which the channel is connected.
 *	ch_number - channel #.
 *	flags - see xpc.h for valid flags.
 *	payload - address of the allocated payload area pointer (filled in on
 * 	          return) in which the user-defined message is constructed.
 */
enum xp_retval
xpc_initiate_allocate(short partid, int ch_number, u32 flags, void **payload)
{
	struct xpc_partition *part = &xpc_partitions[partid];
	enum xp_retval ret = xpUnknownReason;
	struct xpc_msg *msg = NULL;

	DBUG_ON(partid < 0 || partid >= xp_max_npartitions);
	DBUG_ON(ch_number < 0 || ch_number >= part->nchannels);

	*payload = NULL;

	if (xpc_part_ref(part)) {
		ret = xpc_allocate_msg(&part->channels[ch_number], flags, &msg);
		xpc_part_deref(part);

		if (msg != NULL)
			*payload = &msg->payload;
	}

	return ret;
}

/*
 * Send a message previously allocated using xpc_initiate_allocate() on the
 * specified channel connected to the specified partition.
 *
 * This routine will not wait for the message to be received, nor will
 * notification be given when it does happen. Once this routine has returned
 * the message entry allocated via xpc_initiate_allocate() is no longer
 * accessable to the caller.
 *
 * This routine, although called by users, does not call xpc_part_ref() to
 * ensure that the partition infrastructure is in place. It relies on the
 * fact that we called xpc_msgqueue_ref() in xpc_allocate_msg().
 *
 * Arguments:
 *
 *	partid - ID of partition to which the channel is connected.
 *	ch_number - channel # to send message on.
 *	payload - pointer to the payload area allocated via
 *			xpc_initiate_allocate().
 */
enum xp_retval
xpc_initiate_send(short partid, int ch_number, void *payload)
{
	struct xpc_partition *part = &xpc_partitions[partid];
	struct xpc_msg *msg = XPC_MSG_ADDRESS(payload);
	enum xp_retval ret;

	dev_dbg(xpc_chan, "msg=0x%p, partid=%d, channel=%d\n", (void *)msg,
		partid, ch_number);

	DBUG_ON(partid < 0 || partid >= xp_max_npartitions);
	DBUG_ON(ch_number < 0 || ch_number >= part->nchannels);
	DBUG_ON(msg == NULL);

	ret = xpc_send_msg(&part->channels[ch_number], msg, 0, NULL, NULL);

	return ret;
}

/*
 * Send a message previously allocated using xpc_initiate_allocate on the
 * specified channel connected to the specified partition.
 *
 * This routine will not wait for the message to be sent. Once this routine
 * has returned the message entry allocated via xpc_initiate_allocate() is no
 * longer accessable to the caller.
 *
 * Once the remote end of the channel has received the message, the function
 * passed as an argument to xpc_initiate_send_notify() will be called. This
 * allows the sender to free up or re-use any buffers referenced by the
 * message, but does NOT mean the message has been processed at the remote
 * end by a receiver.
 *
 * If this routine returns an error, the caller's function will NOT be called.
 *
 * This routine, although called by users, does not call xpc_part_ref() to
 * ensure that the partition infrastructure is in place. It relies on the
 * fact that we called xpc_msgqueue_ref() in xpc_allocate_msg().
 *
 * Arguments:
 *
 *	partid - ID of partition to which the channel is connected.
 *	ch_number - channel # to send message on.
 *	payload - pointer to the payload area allocated via
 *			xpc_initiate_allocate().
 *	func - function to call with asynchronous notification of message
 *		  receipt. THIS FUNCTION MUST BE NON-BLOCKING.
 *	key - user-defined key to be passed to the function when it's called.
 */
enum xp_retval
xpc_initiate_send_notify(short partid, int ch_number, void *payload,
			 xpc_notify_func func, void *key)
{
	struct xpc_partition *part = &xpc_partitions[partid];
	struct xpc_msg *msg = XPC_MSG_ADDRESS(payload);
	enum xp_retval ret;

	dev_dbg(xpc_chan, "msg=0x%p, partid=%d, channel=%d\n", (void *)msg,
		partid, ch_number);

	DBUG_ON(partid < 0 || partid >= xp_max_npartitions);
	DBUG_ON(ch_number < 0 || ch_number >= part->nchannels);
	DBUG_ON(msg == NULL);
	DBUG_ON(func == NULL);

	ret = xpc_send_msg(&part->channels[ch_number], msg, XPC_N_CALL,
			   func, key);
	return ret;
}

/*
 * Deliver a message to its intended recipient.
 */
void
xpc_deliver_msg(struct xpc_channel *ch)
{
	struct xpc_msg *msg;

	msg = xpc_get_deliverable_msg(ch);
	if (msg != NULL) {

		/*
		 * This ref is taken to protect the payload itself from being
		 * freed before the user is finished with it, which the user
		 * indicates by calling xpc_initiate_received().
		 */
		xpc_msgqueue_ref(ch);

		atomic_inc(&ch->kthreads_active);

		if (ch->func != NULL) {
			dev_dbg(xpc_chan, "ch->func() called, msg=0x%p, "
				"msg_number=%ld, partid=%d, channel=%d\n",
				(void *)msg, msg->number, ch->partid,
				ch->number);

			/* deliver the message to its intended recipient */
			ch->func(xpMsgReceived, ch->partid, ch->number,
				 &msg->payload, ch->key);

			dev_dbg(xpc_chan, "ch->func() returned, msg=0x%p, "
				"msg_number=%ld, partid=%d, channel=%d\n",
				(void *)msg, msg->number, ch->partid,
				ch->number);
		}

		atomic_dec(&ch->kthreads_active);
	}
}

/*
 * Acknowledge receipt of a delivered message.
 *
 * If a message has XPC_M_INTERRUPT set, send an interrupt to the partition
 * that sent the message.
 *
 * This function, although called by users, does not call xpc_part_ref() to
 * ensure that the partition infrastructure is in place. It relies on the
 * fact that we called xpc_msgqueue_ref() in xpc_deliver_msg().
 *
 * Arguments:
 *
 *	partid - ID of partition to which the channel is connected.
 *	ch_number - channel # message received on.
 *	payload - pointer to the payload area allocated via
 *			xpc_initiate_allocate().
 */
void
xpc_initiate_received(short partid, int ch_number, void *payload)
{
	struct xpc_partition *part = &xpc_partitions[partid];
	struct xpc_channel *ch;
	struct xpc_msg *msg = XPC_MSG_ADDRESS(payload);

	DBUG_ON(partid < 0 || partid >= xp_max_npartitions);
	DBUG_ON(ch_number < 0 || ch_number >= part->nchannels);

	ch = &part->channels[ch_number];
	xpc_received_msg(ch, msg);

	/* the call to xpc_msgqueue_ref() was done by xpc_deliver_msg()  */
	xpc_msgqueue_deref(ch);
}
