/*
  handle em28xx IR remotes via linux kernel input layer.

   Copyright (C) 2005 Ludovico Cavedon <cavedon@sssup.it>
		      Markus Rechberger <mrechberger@gmail.com>
		      Mauro Carvalho Chehab <mchehab@infradead.org>
		      Sascha Sommer <saschasommer@freenet.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/usb.h>

#include "em28xx.h"

#define EM28XX_SNAPSHOT_KEY KEY_CAMERA
#define EM28XX_SBUTTON_QUERY_INTERVAL 500
#define EM28XX_R0C_USBSUSP_SNAPSHOT 0x20

static unsigned int ir_debug;
module_param(ir_debug, int, 0644);
MODULE_PARM_DESC(ir_debug, "enable debug messages [IR]");

#define i2cdprintk(fmt, arg...) \
	if (ir_debug) { \
		printk(KERN_DEBUG "%s/ir: " fmt, ir->c.name , ## arg); \
	}

#define dprintk(fmt, arg...) \
	if (ir_debug) { \
		printk(KERN_DEBUG "%s/ir: " fmt, ir->name , ## arg); \
	}

/**********************************************************
 Polling structure used by em28xx IR's
 **********************************************************/

struct em28xx_IR {
	struct em28xx *dev;
	struct input_dev *input;
	struct ir_input_state ir;
	char name[32];
	char phys[32];

	/* poll external decoder */
	int polling;
	struct work_struct work;
	struct timer_list timer;
	u32 last_gpio;
	u32 mask_keycode;
	u32 mask_keydown;
	u32 mask_keyup;

	int  (*get_key)(struct em28xx_IR *);
};

/**********************************************************
 I2C IR based get keycodes - should be used with ir-kbd-i2c
 **********************************************************/

int em28xx_get_key_terratec(struct IR_i2c *ir, u32 *ir_key, u32 *ir_raw)
{
	unsigned char b;

	/* poll IR chip */
	if (1 != i2c_master_recv(&ir->c, &b, 1)) {
		i2cdprintk("read error\n");
		return -EIO;
	}

	/* it seems that 0xFE indicates that a button is still hold
	   down, while 0xff indicates that no button is hold
	   down. 0xfe sequences are sometimes interrupted by 0xFF */

	i2cdprintk("key %02x\n", b);

	if (b == 0xff)
		return 0;

	if (b == 0xfe)
		/* keep old data */
		return 1;

	*ir_key = b;
	*ir_raw = b;
	return 1;
}

int em28xx_get_key_em_haup(struct IR_i2c *ir, u32 *ir_key, u32 *ir_raw)
{
	unsigned char buf[2];
	unsigned char code;

	/* poll IR chip */
	if (2 != i2c_master_recv(&ir->c, buf, 2))
		return -EIO;

	/* Does eliminate repeated parity code */
	if (buf[1] == 0xff)
		return 0;

	ir->old = buf[1];

	/* Rearranges bits to the right order */
	code =   ((buf[0]&0x01)<<5) | /* 0010 0000 */
		 ((buf[0]&0x02)<<3) | /* 0001 0000 */
		 ((buf[0]&0x04)<<1) | /* 0000 1000 */
		 ((buf[0]&0x08)>>1) | /* 0000 0100 */
		 ((buf[0]&0x10)>>3) | /* 0000 0010 */
		 ((buf[0]&0x20)>>5);  /* 0000 0001 */

	i2cdprintk("ir hauppauge (em2840): code=0x%02x (rcv=0x%02x)\n",
			code, buf[0]);

	/* return key */
	*ir_key = code;
	*ir_raw = code;
	return 1;
}

int em28xx_get_key_pinnacle_usb_grey(struct IR_i2c *ir, u32 *ir_key,
				     u32 *ir_raw)
{
	unsigned char buf[3];

	/* poll IR chip */

	if (3 != i2c_master_recv(&ir->c, buf, 3)) {
		i2cdprintk("read error\n");
		return -EIO;
	}

	i2cdprintk("key %02x\n", buf[2]&0x3f);
	if (buf[0] != 0x00)
		return 0;

	*ir_key = buf[2]&0x3f;
	*ir_raw = buf[2]&0x3f;

	return 1;
}

/**********************************************************
 Poll based get keycode functions
 **********************************************************/

static int default_polling_getkey(struct em28xx_IR *ir)
{
	struct em28xx *dev = ir->dev;
	int rc;
	u32 msg;

	/* Read key toggle, brand, and key code */
	rc = dev->em28xx_read_reg_req_len(dev, 0, EM28XX_R45_IR,
					  (u8 *)&msg, sizeof(msg));
	if (rc < 0)
		return rc;

	return (int)(msg & 0x7fffffffl);
}

/**********************************************************
 Polling code for em28xx
 **********************************************************/

static void em28xx_ir_handle_key(struct em28xx_IR *ir)
{
	int    gpio;
	u32    data;

	/* read gpio value */
	gpio = ir->get_key(ir);
	if (gpio < 0)
		return;

	if (gpio == ir->last_gpio)
		return;
	ir->last_gpio = gpio;

	/* extract data */
	data = ir_extract_bits(gpio, ir->mask_keycode);
	dprintk("irq gpio=0x%x code=%d | poll%s%s\n",
		   gpio, data,
		   (gpio & ir->mask_keydown) ? " down" : "",
		   (gpio & ir->mask_keyup) ? " up" : "");

	/* Generate keyup/keydown events */
	if (ir->mask_keydown) {
		/* bit set on keydown */
		if (gpio & ir->mask_keydown)
			ir_input_keydown(ir->input, &ir->ir, data, data);
		else
			ir_input_nokey(ir->input, &ir->ir);
	} else if (ir->mask_keyup) {
		/* bit cleared on keydown */
		if (!(gpio & ir->mask_keyup))
			ir_input_keydown(ir->input, &ir->ir, data, data);
		else
			ir_input_nokey(ir->input, &ir->ir);
	} else {
		/* can't distinguish keydown/up :-/ */
		ir_input_keydown(ir->input, &ir->ir, data, data);
		ir_input_nokey(ir->input, &ir->ir);
	}
}

static void ir_timer(unsigned long data)
{
	struct em28xx_IR *ir = (struct em28xx_IR *)data;

	schedule_work(&ir->work);
}

static void em28xx_ir_work(struct work_struct *work)
{
	struct em28xx_IR *ir = container_of(work, struct em28xx_IR, work);

	em28xx_ir_handle_key(ir);
	mod_timer(&ir->timer, jiffies + msecs_to_jiffies(ir->polling));
}

void em28xx_ir_start(struct em28xx_IR *ir)
{
	setup_timer(&ir->timer, ir_timer, (unsigned long)ir);
	INIT_WORK(&ir->work, em28xx_ir_work);
	schedule_work(&ir->work);
}

static void em28xx_ir_stop(struct em28xx_IR *ir)
{
	del_timer_sync(&ir->timer);
	flush_scheduled_work();
}

int em28xx_ir_init(struct em28xx *dev)
{
	struct em28xx_IR *ir;
	struct input_dev *input_dev;
	IR_KEYTAB_TYPE *ir_codes = NULL;
	int ir_type = IR_TYPE_OTHER;
	int err = -ENOMEM;

	ir = kzalloc(sizeof(*ir), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!ir || !input_dev)
		goto err_out_free;

	ir->input = input_dev;

	/* */
	ir->get_key = default_polling_getkey;
	ir->polling = 50; /* ms */

	/* detect & configure */
	switch (dev->model) {
	}

	if (NULL == ir_codes) {
		err = -ENODEV;
		goto err_out_free;
	}

	/* Get the current key status, to avoid adding an
	   unexistent key code */
	ir->last_gpio    = ir->get_key(ir);

	/* init input device */
	snprintf(ir->name, sizeof(ir->name), "em28xx IR (%s)",
						dev->name);

	usb_make_path(dev->udev, ir->phys, sizeof(ir->phys));
	strlcat(ir->phys, "/input0", sizeof(ir->phys));

	ir_input_init(input_dev, &ir->ir, ir_type, ir_codes);
	input_dev->name = ir->name;
	input_dev->phys = ir->phys;
	input_dev->id.bustype = BUS_USB;
	input_dev->id.version = 1;
	input_dev->id.vendor = le16_to_cpu(dev->udev->descriptor.idVendor);
	input_dev->id.product = le16_to_cpu(dev->udev->descriptor.idProduct);

	input_dev->dev.parent = &dev->udev->dev;
	/* record handles to ourself */
	ir->dev = dev;
	dev->ir = ir;

	em28xx_ir_start(ir);

	/* all done */
	err = input_register_device(ir->input);
	if (err)
		goto err_out_stop;

	return 0;
 err_out_stop:
	em28xx_ir_stop(ir);
	dev->ir = NULL;
 err_out_free:
	input_free_device(input_dev);
	kfree(ir);
	return err;
}

int em28xx_ir_fini(struct em28xx *dev)
{
	struct em28xx_IR *ir = dev->ir;

	/* skip detach on non attached boards */
	if (!ir)
		return 0;

	em28xx_ir_stop(ir);
	input_unregister_device(ir->input);
	kfree(ir);

	/* done */
	dev->ir = NULL;
	return 0;
}

/**********************************************************
 Handle Webcam snapshot button
 **********************************************************/

static void em28xx_query_sbutton(struct work_struct *work)
{
	/* Poll the register and see if the button is depressed */
	struct em28xx *dev =
		container_of(work, struct em28xx, sbutton_query_work.work);
	int ret;

	ret = em28xx_read_reg(dev, EM28XX_R0C_USBSUSP);

	if (ret & EM28XX_R0C_USBSUSP_SNAPSHOT) {
		u8 cleared;
		/* Button is depressed, clear the register */
		cleared = ((u8) ret) & ~EM28XX_R0C_USBSUSP_SNAPSHOT;
		em28xx_write_regs(dev, EM28XX_R0C_USBSUSP, &cleared, 1);

		/* Not emulate the keypress */
		input_report_key(dev->sbutton_input_dev, EM28XX_SNAPSHOT_KEY,
				 1);
		/* Now unpress the key */
		input_report_key(dev->sbutton_input_dev, EM28XX_SNAPSHOT_KEY,
				 0);
	}

	/* Schedule next poll */
	schedule_delayed_work(&dev->sbutton_query_work,
			      msecs_to_jiffies(EM28XX_SBUTTON_QUERY_INTERVAL));
}

void em28xx_register_snapshot_button(struct em28xx *dev)
{
	struct input_dev *input_dev;
	int err;

	em28xx_info("Registering snapshot button...\n");
	input_dev = input_allocate_device();
	if (!input_dev) {
		em28xx_errdev("input_allocate_device failed\n");
		return;
	}

	usb_make_path(dev->udev, dev->snapshot_button_path,
		      sizeof(dev->snapshot_button_path));
	strlcat(dev->snapshot_button_path, "/sbutton",
		sizeof(dev->snapshot_button_path));
	INIT_DELAYED_WORK(&dev->sbutton_query_work, em28xx_query_sbutton);

	input_dev->name = "em28xx snapshot button";
	input_dev->phys = dev->snapshot_button_path;
	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	set_bit(EM28XX_SNAPSHOT_KEY, input_dev->keybit);
	input_dev->keycodesize = 0;
	input_dev->keycodemax = 0;
	input_dev->id.bustype = BUS_USB;
	input_dev->id.vendor = le16_to_cpu(dev->udev->descriptor.idVendor);
	input_dev->id.product = le16_to_cpu(dev->udev->descriptor.idProduct);
	input_dev->id.version = 1;
	input_dev->dev.parent = &dev->udev->dev;

	err = input_register_device(input_dev);
	if (err) {
		em28xx_errdev("input_register_device failed\n");
		input_free_device(input_dev);
		return;
	}

	dev->sbutton_input_dev = input_dev;
	schedule_delayed_work(&dev->sbutton_query_work,
			      msecs_to_jiffies(EM28XX_SBUTTON_QUERY_INTERVAL));
	return;

}

void em28xx_deregister_snapshot_button(struct em28xx *dev)
{
	if (dev->sbutton_input_dev != NULL) {
		em28xx_info("Deregistering snapshot button\n");
		cancel_rearming_delayed_work(&dev->sbutton_query_work);
		input_unregister_device(dev->sbutton_input_dev);
		dev->sbutton_input_dev = NULL;
	}
	return;
}
