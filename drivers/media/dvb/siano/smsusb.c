/*
 *  Driver for the Siano SMS10xx USB dongle
 *
 *  author: Anatoly Greenblat
 *
 *  Copyright (c), 2005-2008 Siano Mobile Silicon, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3 as
 *  published by the Free Software Foundation;
 *
 *  Software distributed under the License is distributed on an "AS IS"
 *  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/firmware.h>

#include "smscoreapi.h"
#include "sms-cards.h"

#define USB1_BUFFER_SIZE		0x1000
#define USB2_BUFFER_SIZE		0x4000

#define MAX_BUFFERS		50
#define MAX_URBS		10

struct smsusb_device_t;

struct smsusb_urb_t {
	struct smscore_buffer_t *cb;
	struct smsusb_device_t	*dev;

	struct urb urb;
};

struct smsusb_device_t {
	struct usb_device *udev;
	struct smscore_device_t *coredev;

	struct smsusb_urb_t 	surbs[MAX_URBS];

	int		response_alignment;
	int		buffer_size;
};

static int smsusb_submit_urb(struct smsusb_device_t *dev,
			     struct smsusb_urb_t *surb);

static void smsusb_onresponse(struct urb *urb)
{
	struct smsusb_urb_t *surb = (struct smsusb_urb_t *) urb->context;
	struct smsusb_device_t *dev = surb->dev;

	if (urb->status < 0) {
		sms_err("error, urb status %d, %d bytes",
			urb->status, urb->actual_length);
		return;
	}

	if (urb->actual_length > 0) {
		struct SmsMsgHdr_ST *phdr = (struct SmsMsgHdr_ST *) surb->cb->p;

		if (urb->actual_length >= phdr->msgLength) {
			surb->cb->size = phdr->msgLength;

			if (dev->response_alignment &&
			    (phdr->msgFlags & MSG_HDR_FLAG_SPLIT_MSG)) {

				surb->cb->offset =
					dev->response_alignment +
					((phdr->msgFlags >> 8) & 3);

				/* sanity check */
				if (((int) phdr->msgLength +
				     surb->cb->offset) > urb->actual_length) {
					sms_err("invalid response "
						"msglen %d offset %d "
						"size %d",
						phdr->msgLength,
						surb->cb->offset,
						urb->actual_length);
					goto exit_and_resubmit;
				}

				/* move buffer pointer and
				 * copy header to its new location */
				memcpy((char *) phdr + surb->cb->offset,
				       phdr, sizeof(struct SmsMsgHdr_ST));
			} else
				surb->cb->offset = 0;

			smscore_onresponse(dev->coredev, surb->cb);
			surb->cb = NULL;
		} else {
			sms_err("invalid response "
				"msglen %d actual %d",
				phdr->msgLength, urb->actual_length);
		}
	}

exit_and_resubmit:
	smsusb_submit_urb(dev, surb);
}

static int smsusb_submit_urb(struct smsusb_device_t *dev,
			     struct smsusb_urb_t *surb)
{
	if (!surb->cb) {
		surb->cb = smscore_getbuffer(dev->coredev);
		if (!surb->cb) {
			sms_err("smscore_getbuffer(...) returned NULL");
			return -ENOMEM;
		}
	}

	usb_fill_bulk_urb(
		&surb->urb,
		dev->udev,
		usb_rcvbulkpipe(dev->udev, 0x81),
		surb->cb->p,
		dev->buffer_size,
		smsusb_onresponse,
		surb
	);
	surb->urb.transfer_dma = surb->cb->phys;
	surb->urb.transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	return usb_submit_urb(&surb->urb, GFP_ATOMIC);
}

static void smsusb_stop_streaming(struct smsusb_device_t *dev)
{
	int i;

	for (i = 0; i < MAX_URBS; i++) {
		usb_kill_urb(&dev->surbs[i].urb);

		if (dev->surbs[i].cb) {
			smscore_putbuffer(dev->coredev, dev->surbs[i].cb);
			dev->surbs[i].cb = NULL;
		}
	}
}

static int smsusb_start_streaming(struct smsusb_device_t *dev)
{
	int i, rc;

	for (i = 0; i < MAX_URBS; i++) {
		rc = smsusb_submit_urb(dev, &dev->surbs[i]);
		if (rc < 0) {
			sms_err("smsusb_submit_urb(...) failed");
			smsusb_stop_streaming(dev);
			break;
		}
	}

	return rc;
}

static int smsusb_sendrequest(void *context, void *buffer, size_t size)
{
	struct smsusb_device_t *dev = (struct smsusb_device_t *) context;
	int dummy;

	return usb_bulk_msg(dev->udev, usb_sndbulkpipe(dev->udev, 2),
			    buffer, size, &dummy, 1000);
}

static char *smsusb1_fw_lkup[] = {
	"dvbt_stellar_usb.inp",
	"dvbh_stellar_usb.inp",
	"tdmb_stellar_usb.inp",
	"none",
	"dvbt_bda_stellar_usb.inp",
};

static int smsusb1_load_firmware(struct usb_device *udev, int id)
{
	const struct firmware *fw;
	u8 *fw_buffer;
	int rc, dummy;

	if (id < DEVICE_MODE_DVBT || id > DEVICE_MODE_DVBT_BDA) {
		sms_err("invalid firmware id specified %d", id);
		return -EINVAL;
	}

	rc = request_firmware(&fw, smsusb1_fw_lkup[id], &udev->dev);
	if (rc < 0) {
		sms_err("failed to open \"%s\" mode %d",
			smsusb1_fw_lkup[id], id);
		return rc;
	}

	fw_buffer = kmalloc(fw->size, GFP_KERNEL);
	if (fw_buffer) {
		memcpy(fw_buffer, fw->data, fw->size);

		rc = usb_bulk_msg(udev, usb_sndbulkpipe(udev, 2),
				  fw_buffer, fw->size, &dummy, 1000);

		sms_info("sent %d(%d) bytes, rc %d", fw->size, dummy, rc);

		kfree(fw_buffer);
	} else {
		sms_err("failed to allocate firmware buffer");
		rc = -ENOMEM;
	}

	release_firmware(fw);

	return rc;
}

static void smsusb1_detectmode(void *context, int *mode)
{
	char *product_string =
		((struct smsusb_device_t *) context)->udev->product;

	*mode = DEVICE_MODE_NONE;

	if (!product_string) {
		product_string = "none";
		sms_err("product string not found");
	} else if (strstr(product_string, "DVBH"))
		*mode = 1;
	else if (strstr(product_string, "BDA"))
		*mode = 4;
	else if (strstr(product_string, "DVBT"))
		*mode = 0;
	else if (strstr(product_string, "TDMB"))
		*mode = 2;

	sms_info("%d \"%s\"", *mode, product_string);
}

static int smsusb1_setmode(void *context, int mode)
{
	struct SmsMsgHdr_ST Msg = { MSG_SW_RELOAD_REQ, 0, HIF_TASK,
			     sizeof(struct SmsMsgHdr_ST), 0 };

	if (mode < DEVICE_MODE_DVBT || mode > DEVICE_MODE_DVBT_BDA) {
		sms_err("invalid firmware id specified %d", mode);
		return -EINVAL;
	}

	return smsusb_sendrequest(context, &Msg, sizeof(Msg));
}

static void smsusb_term_device(struct usb_interface *intf)
{
	struct smsusb_device_t *dev =
		(struct smsusb_device_t *) usb_get_intfdata(intf);

	if (dev) {
		smsusb_stop_streaming(dev);

		/* unregister from smscore */
		if (dev->coredev)
			smscore_unregister_device(dev->coredev);

		kfree(dev);

		sms_info("device %p destroyed", dev);
	}

	usb_set_intfdata(intf, NULL);
}

static int smsusb_init_device(struct usb_interface *intf, int board_id)
{
	struct smsdevice_params_t params;
	struct smsusb_device_t *dev;
	struct sms_board *board;
	int i, rc;

	/* create device object */
	dev = kzalloc(sizeof(struct smsusb_device_t), GFP_KERNEL);
	if (!dev) {
		sms_err("kzalloc(sizeof(struct smsusb_device_t) failed");
		return -ENOMEM;
	}

	memset(&params, 0, sizeof(params));
	usb_set_intfdata(intf, dev);
	dev->udev = interface_to_usbdev(intf);

	board = sms_get_board(board_id);

	switch (board->type) {

	case SMS_STELLAR:
		dev->buffer_size = USB1_BUFFER_SIZE;

		params.setmode_handler = smsusb1_setmode;
		params.detectmode_handler = smsusb1_detectmode;
		params.device_type = SMS_STELLAR;
		sms_info("stellar device found");
		break;
	default:
		switch (board->type) {
		case SMS_NOVA_A0:
			params.device_type = SMS_NOVA_A0;
			sms_info("nova A0 found");
			break;
		case SMS_NOVA_B0:
			params.device_type = SMS_NOVA_B0;
			sms_info("nova B0 found");
			break;
		case SMS_VEGA:
			params.device_type = SMS_VEGA;
			sms_info("Vega found");
			break;
		default:
			sms_err("Unspecified sms device type!");
		}

		dev->buffer_size = USB2_BUFFER_SIZE;
		dev->response_alignment =
			dev->udev->ep_in[1]->desc.wMaxPacketSize -
			sizeof(struct SmsMsgHdr_ST);

		params.flags |= SMS_DEVICE_FAMILY2;
		break;
	}

	params.device = &dev->udev->dev;
	params.buffer_size = dev->buffer_size;
	params.num_buffers = MAX_BUFFERS;
	params.sendrequest_handler = smsusb_sendrequest;
	params.context = dev;
	snprintf(params.devpath, sizeof(params.devpath),
		 "usb\\%d-%s", dev->udev->bus->busnum, dev->udev->devpath);

	/* register in smscore */
	rc = smscore_register_device(&params, &dev->coredev);
	if (rc < 0) {
		sms_err("smscore_register_device(...) failed, rc %d", rc);
		smsusb_term_device(intf);
		return rc;
	}

	smscore_set_board_id(dev->coredev, board_id);

	/* initialize urbs */
	for (i = 0; i < MAX_URBS; i++) {
		dev->surbs[i].dev = dev;
		usb_init_urb(&dev->surbs[i].urb);
	}

	sms_info("smsusb_start_streaming(...).");
	rc = smsusb_start_streaming(dev);
	if (rc < 0) {
		sms_err("smsusb_start_streaming(...) failed");
		smsusb_term_device(intf);
		return rc;
	}

	rc = smscore_start_device(dev->coredev);
	if (rc < 0) {
		sms_err("smscore_start_device(...) failed");
		smsusb_term_device(intf);
		return rc;
	}

	sms_info("device %p created", dev);

	return rc;
}

static int smsusb_probe(struct usb_interface *intf,
			const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	char devpath[32];
	int i, rc;

	rc = usb_clear_halt(udev, usb_rcvbulkpipe(udev, 0x81));
	rc = usb_clear_halt(udev, usb_rcvbulkpipe(udev, 0x02));

	if (intf->num_altsetting > 0) {
		rc = usb_set_interface(
			udev, intf->cur_altsetting->desc.bInterfaceNumber, 0);
		if (rc < 0) {
			sms_err("usb_set_interface failed, rc %d", rc);
			return rc;
		}
	}

	sms_info("smsusb_probe %d",
	       intf->cur_altsetting->desc.bInterfaceNumber);
	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++)
		sms_info("endpoint %d %02x %02x %d", i,
		       intf->cur_altsetting->endpoint[i].desc.bEndpointAddress,
		       intf->cur_altsetting->endpoint[i].desc.bmAttributes,
		       intf->cur_altsetting->endpoint[i].desc.wMaxPacketSize);

	if ((udev->actconfig->desc.bNumInterfaces == 2) &&
	    (intf->cur_altsetting->desc.bInterfaceNumber == 0)) {
		sms_err("rom interface 0 is not used");
		return -ENODEV;
	}

	if (intf->cur_altsetting->desc.bInterfaceNumber == 1) {
		snprintf(devpath, sizeof(devpath), "usb\\%d-%s",
			 udev->bus->busnum, udev->devpath);
		sms_info("stellar device was found.");
		return smsusb1_load_firmware(
				udev, smscore_registry_getmode(devpath));
	}

	rc = smsusb_init_device(intf, id->driver_info);
	sms_info("rc %d", rc);
	return rc;
}

static void smsusb_disconnect(struct usb_interface *intf)
{
	smsusb_term_device(intf);
}

static struct usb_driver smsusb_driver = {
	.name			= "smsusb",
	.probe			= smsusb_probe,
	.disconnect		= smsusb_disconnect,
	.id_table		= smsusb_id_table,
};

int smsusb_register(void)
{
	int rc = usb_register(&smsusb_driver);
	if (rc)
		sms_err("usb_register failed. Error number %d", rc);

	sms_debug("");

	return rc;
}

void smsusb_unregister(void)
{
	sms_debug("");
	/* Regular USB Cleanup */
	usb_deregister(&smsusb_driver);
}

