/*
 *  tifm_core.c - TI FlashMedia driver
 *
 *  Copyright (C) 2006 Alex Dubov <oakad@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/tifm.h>
#include <linux/init.h>
#include <linux/idr.h>

#define DRIVER_NAME "tifm_core"
#define DRIVER_VERSION "0.8"

static struct workqueue_struct *workqueue;
static DEFINE_IDR(tifm_adapter_idr);
static DEFINE_SPINLOCK(tifm_adapter_lock);

static const char *tifm_media_type_name(unsigned char type, unsigned char nt)
{
	const char *card_type_name[3][3] = {
		{ "SmartMedia/xD", "MemoryStick", "MMC/SD" },
		{ "XD", "MS", "SD"},
		{ "xd", "ms", "sd"}
	};

	if (nt > 2 || type < 1 || type > 3)
		return NULL;
	return card_type_name[nt][type - 1];
}

static int tifm_dev_match(struct tifm_dev *sock, struct tifm_device_id *id)
{
	if (sock->type == id->type)
		return 1;
	return 0;
}

static int tifm_bus_match(struct device *dev, struct device_driver *drv)
{
	struct tifm_dev *sock = container_of(dev, struct tifm_dev, dev);
	struct tifm_driver *fm_drv = container_of(drv, struct tifm_driver,
						  driver);
	struct tifm_device_id *ids = fm_drv->id_table;

	if (ids) {
		while (ids->type) {
			if (tifm_dev_match(sock, ids))
				return 1;
			++ids;
		}
	}
	return 0;
}

static int tifm_uevent(struct device *dev, char **envp, int num_envp,
		       char *buffer, int buffer_size)
{
	struct tifm_dev *sock = container_of(dev, struct tifm_dev, dev);
	int i = 0;
	int length = 0;

	if (add_uevent_var(envp, num_envp, &i, buffer, buffer_size, &length,
			   "TIFM_CARD_TYPE=%s",
			   tifm_media_type_name(sock->type, 1)))
		return -ENOMEM;

	return 0;
}

static int tifm_device_probe(struct device *dev)
{
	struct tifm_dev *sock = container_of(dev, struct tifm_dev, dev);
	struct tifm_driver *drv = container_of(dev->driver, struct tifm_driver,
					       driver);
	int rc = -ENODEV;

	get_device(dev);
	if (dev->driver && drv->probe) {
		rc = drv->probe(sock);
		if (!rc)
			return 0;
	}
	put_device(dev);
	return rc;
}

static void tifm_dummy_event(struct tifm_dev *sock)
{
	return;
}

static int tifm_device_remove(struct device *dev)
{
	struct tifm_dev *sock = container_of(dev, struct tifm_dev, dev);
	struct tifm_driver *drv = container_of(dev->driver, struct tifm_driver,
					       driver);

	if (dev->driver && drv->remove) {
		sock->card_event = tifm_dummy_event;
		sock->data_event = tifm_dummy_event;
		drv->remove(sock);
		sock->dev.driver = NULL;
	}

	put_device(dev);
	return 0;
}

#ifdef CONFIG_PM

static int tifm_device_suspend(struct device *dev, pm_message_t state)
{
	struct tifm_dev *fm_dev = container_of(dev, struct tifm_dev, dev);
	struct tifm_driver *drv = container_of(dev->driver, struct tifm_driver,
					       driver);

	if (dev->driver && drv->suspend)
		return drv->suspend(fm_dev, state);
	return 0;
}

static int tifm_device_resume(struct device *dev)
{
	struct tifm_dev *fm_dev = container_of(dev, struct tifm_dev, dev);
	struct tifm_driver *drv = container_of(dev->driver, struct tifm_driver,
					       driver);

	if (dev->driver && drv->resume)
		return drv->resume(fm_dev);
	return 0;
}

#else

#define tifm_device_suspend NULL
#define tifm_device_resume NULL

#endif /* CONFIG_PM */

static struct bus_type tifm_bus_type = {
	.name    = "tifm",
	.match   = tifm_bus_match,
	.uevent  = tifm_uevent,
	.probe   = tifm_device_probe,
	.remove  = tifm_device_remove,
	.suspend = tifm_device_suspend,
	.resume  = tifm_device_resume
};

static void tifm_free(struct class_device *cdev)
{
	struct tifm_adapter *fm = container_of(cdev, struct tifm_adapter, cdev);

	kfree(fm);
}

static struct class tifm_adapter_class = {
	.name    = "tifm_adapter",
	.release = tifm_free
};

struct tifm_adapter *tifm_alloc_adapter(unsigned int num_sockets,
					struct device *dev)
{
	struct tifm_adapter *fm;

	fm = kzalloc(sizeof(struct tifm_adapter)
		     + sizeof(struct tifm_dev*) * num_sockets, GFP_KERNEL);
	if (fm) {
		fm->cdev.class = &tifm_adapter_class;
		fm->cdev.dev = dev;
		class_device_initialize(&fm->cdev);
		spin_lock_init(&fm->lock);
		fm->num_sockets = num_sockets;
	}
	return fm;
}
EXPORT_SYMBOL(tifm_alloc_adapter);

int tifm_add_adapter(struct tifm_adapter *fm)
{
	int rc;

	if (!idr_pre_get(&tifm_adapter_idr, GFP_KERNEL))
		return -ENOMEM;

	spin_lock(&tifm_adapter_lock);
	rc = idr_get_new(&tifm_adapter_idr, fm, &fm->id);
	spin_unlock(&tifm_adapter_lock);
	if (rc)
		return rc;

	snprintf(fm->cdev.class_id, BUS_ID_SIZE, "tifm%u", fm->id);
	rc = class_device_add(&fm->cdev);
	if (rc) {
		spin_lock(&tifm_adapter_lock);
		idr_remove(&tifm_adapter_idr, fm->id);
		spin_unlock(&tifm_adapter_lock);
	}

	return rc;
}
EXPORT_SYMBOL(tifm_add_adapter);

void tifm_remove_adapter(struct tifm_adapter *fm)
{
	unsigned int cnt;

	flush_workqueue(workqueue);
	for (cnt = 0; cnt < fm->num_sockets; ++cnt) {
		if (fm->sockets[cnt])
			device_unregister(&fm->sockets[cnt]->dev);
	}

	spin_lock(&tifm_adapter_lock);
	idr_remove(&tifm_adapter_idr, fm->id);
	spin_unlock(&tifm_adapter_lock);
	class_device_del(&fm->cdev);
}
EXPORT_SYMBOL(tifm_remove_adapter);

void tifm_free_adapter(struct tifm_adapter *fm)
{
	class_device_put(&fm->cdev);
}
EXPORT_SYMBOL(tifm_free_adapter);

void tifm_free_device(struct device *dev)
{
	struct tifm_dev *sock = container_of(dev, struct tifm_dev, dev);
	kfree(sock);
}
EXPORT_SYMBOL(tifm_free_device);

struct tifm_dev *tifm_alloc_device(struct tifm_adapter *fm, unsigned int id,
				   unsigned char type)
{
	struct tifm_dev *sock = NULL;

	if (!tifm_media_type_name(type, 0))
		return sock;

	sock = kzalloc(sizeof(struct tifm_dev), GFP_KERNEL);
	if (sock) {
		spin_lock_init(&sock->lock);
		sock->type = type;
		sock->socket_id = id;
		sock->card_event = tifm_dummy_event;
		sock->data_event = tifm_dummy_event;

		sock->dev.parent = fm->cdev.dev;
		sock->dev.bus = &tifm_bus_type;
		sock->dev.dma_mask = fm->cdev.dev->dma_mask;
		sock->dev.release = tifm_free_device;

		snprintf(sock->dev.bus_id, BUS_ID_SIZE,
			 "tifm_%s%u:%u", tifm_media_type_name(type, 2),
			 fm->id, id);
		printk(KERN_INFO DRIVER_NAME
		       ": %s card detected in socket %u:%u\n",
		       tifm_media_type_name(type, 0), fm->id, id);
	}
	return sock;
}
EXPORT_SYMBOL(tifm_alloc_device);

void tifm_eject(struct tifm_dev *sock)
{
	struct tifm_adapter *fm = dev_get_drvdata(sock->dev.parent);
	fm->eject(fm, sock);
}
EXPORT_SYMBOL(tifm_eject);

int tifm_map_sg(struct tifm_dev *sock, struct scatterlist *sg, int nents,
		int direction)
{
	return pci_map_sg(to_pci_dev(sock->dev.parent), sg, nents, direction);
}
EXPORT_SYMBOL(tifm_map_sg);

void tifm_unmap_sg(struct tifm_dev *sock, struct scatterlist *sg, int nents,
		   int direction)
{
	pci_unmap_sg(to_pci_dev(sock->dev.parent), sg, nents, direction);
}
EXPORT_SYMBOL(tifm_unmap_sg);

void tifm_queue_work(struct work_struct *work)
{
	queue_work(workqueue, work);
}
EXPORT_SYMBOL(tifm_queue_work);

int tifm_register_driver(struct tifm_driver *drv)
{
	drv->driver.bus = &tifm_bus_type;

	return driver_register(&drv->driver);
}
EXPORT_SYMBOL(tifm_register_driver);

void tifm_unregister_driver(struct tifm_driver *drv)
{
	driver_unregister(&drv->driver);
}
EXPORT_SYMBOL(tifm_unregister_driver);

static int __init tifm_init(void)
{
	int rc;

	workqueue = create_freezeable_workqueue("tifm");
	if (!workqueue)
		return -ENOMEM;

	rc = bus_register(&tifm_bus_type);

	if (rc)
		goto err_out_wq;

	rc = class_register(&tifm_adapter_class);
	if (!rc)
		return 0;

	bus_unregister(&tifm_bus_type);

err_out_wq:
	destroy_workqueue(workqueue);

	return rc;
}

static void __exit tifm_exit(void)
{
	class_unregister(&tifm_adapter_class);
	bus_unregister(&tifm_bus_type);
	destroy_workqueue(workqueue);
}

subsys_initcall(tifm_init);
module_exit(tifm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex Dubov");
MODULE_DESCRIPTION("TI FlashMedia core driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
