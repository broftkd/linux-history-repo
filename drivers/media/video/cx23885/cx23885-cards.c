/*
 *  Driver for the Conexant CX23885 PCIe bridge
 *
 *  Copyright (c) 2006 Steven Toth <stoth@hauppauge.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <media/cx25840.h>

#include "cx23885.h"

/* ------------------------------------------------------------------ */
/* board config info                                                  */

struct cx23885_board cx23885_boards[] = {
	[CX23885_BOARD_UNKNOWN] = {
		.name		= "UNKNOWN/GENERIC",
		/* Ensure safe default for unknown boards */
		.clk_freq       = 0,
		.input          = {{
			.type   = CX23885_VMUX_COMPOSITE1,
			.vmux   = 0,
		},{
			.type   = CX23885_VMUX_COMPOSITE2,
			.vmux   = 1,
		},{
			.type   = CX23885_VMUX_COMPOSITE3,
			.vmux   = 2,
		},{
			.type   = CX23885_VMUX_COMPOSITE4,
			.vmux   = 3,
		}},
	},
	[CX23885_BOARD_HAUPPAUGE_HVR1800lp] = {
		.name		= "Hauppauge WinTV-HVR1800lp",
		.portc		= CX23885_MPEG_DVB,
		.input          = {{
			.type   = CX23885_VMUX_TELEVISION,
			.vmux   = 0,
			.gpio0  = 0xff00,
		},{
			.type   = CX23885_VMUX_DEBUG,
			.vmux   = 0,
			.gpio0  = 0xff01,
		},{
			.type   = CX23885_VMUX_COMPOSITE1,
			.vmux   = 1,
			.gpio0  = 0xff02,
		},{
			.type   = CX23885_VMUX_SVIDEO,
			.vmux   = 2,
			.gpio0  = 0xff02,
		}},
	},
	[CX23885_BOARD_HAUPPAUGE_HVR1800] = {
		.name		= "Hauppauge WinTV-HVR1800",
		.porta		= CX23885_ANALOG_VIDEO,
		.portc		= CX23885_MPEG_DVB,
		.tuner_type	= TUNER_PHILIPS_TDA8290,
		.tuner_addr	= 0x42, /* 0x84 >> 1 */
		.input          = {{
			.type   = CX23885_VMUX_TELEVISION,
			.vmux   =	CX25840_VIN7_CH3 |
					CX25840_VIN5_CH2 |
					CX25840_VIN2_CH1,
			.gpio0  = 0,
		},{
			.type   = CX23885_VMUX_COMPOSITE1,
			.vmux   =	CX25840_VIN7_CH3 |
					CX25840_VIN4_CH2 |
					CX25840_VIN6_CH1,
			.gpio0  = 0,
		},{
			.type   = CX23885_VMUX_SVIDEO,
			.vmux   =	CX25840_VIN7_CH3 |
					CX25840_VIN4_CH2 |
					CX25840_VIN8_CH1 |
					CX25840_SVIDEO_ON,
			.gpio0  = 0,
		}},
	},
	[CX23885_BOARD_HAUPPAUGE_HVR1250] = {
		.name		= "Hauppauge WinTV-HVR1250",
		.portc		= CX23885_MPEG_DVB,
		.input          = {{
			.type   = CX23885_VMUX_TELEVISION,
			.vmux   = 0,
			.gpio0  = 0xff00,
		},{
			.type   = CX23885_VMUX_DEBUG,
			.vmux   = 0,
			.gpio0  = 0xff01,
		},{
			.type   = CX23885_VMUX_COMPOSITE1,
			.vmux   = 1,
			.gpio0  = 0xff02,
		},{
			.type   = CX23885_VMUX_SVIDEO,
			.vmux   = 2,
			.gpio0  = 0xff02,
		}},
	},
	[CX23885_BOARD_DVICO_FUSIONHDTV_5_EXP] = {
		.name		= "DViCO FusionHDTV5 Express",
		.portb		= CX23885_MPEG_DVB,
	},
	[CX23885_BOARD_HAUPPAUGE_HVR1500Q] = {
		.name		= "Hauppauge WinTV-HVR1500Q",
		.portc		= CX23885_MPEG_DVB,
	},
	[CX23885_BOARD_HAUPPAUGE_HVR1500] = {
		.name		= "Hauppauge WinTV-HVR1500",
		.portc		= CX23885_MPEG_DVB,
	},
};
const unsigned int cx23885_bcount = ARRAY_SIZE(cx23885_boards);

/* ------------------------------------------------------------------ */
/* PCI subsystem IDs                                                  */

struct cx23885_subid cx23885_subids[] = {
	{
		.subvendor = 0x0070,
		.subdevice = 0x3400,
		.card      = CX23885_BOARD_UNKNOWN,
	},{
		.subvendor = 0x0070,
		.subdevice = 0x7600,
		.card      = CX23885_BOARD_HAUPPAUGE_HVR1800lp,
	},{
		.subvendor = 0x0070,
		.subdevice = 0x7800,
		.card      = CX23885_BOARD_HAUPPAUGE_HVR1800,
	},{
		.subvendor = 0x0070,
		.subdevice = 0x7801,
		.card      = CX23885_BOARD_HAUPPAUGE_HVR1800,
	},{
		.subvendor = 0x0070,
		.subdevice = 0x7809,
		.card      = CX23885_BOARD_HAUPPAUGE_HVR1800,
	},{
		.subvendor = 0x0070,
		.subdevice = 0x7911,
		.card      = CX23885_BOARD_HAUPPAUGE_HVR1250,
	},{
		.subvendor = 0x18ac,
		.subdevice = 0xd500,
		.card      = CX23885_BOARD_DVICO_FUSIONHDTV_5_EXP,
	},{
		.subvendor = 0x0070,
		.subdevice = 0x7790,
		.card      = CX23885_BOARD_HAUPPAUGE_HVR1500Q,
	},{
		.subvendor = 0x0070,
		.subdevice = 0x7797,
		.card      = CX23885_BOARD_HAUPPAUGE_HVR1500Q,
	},{
		.subvendor = 0x0070,
		.subdevice = 0x7710,
		.card      = CX23885_BOARD_HAUPPAUGE_HVR1500,
	},{
		.subvendor = 0x0070,
		.subdevice = 0x7717,
		.card      = CX23885_BOARD_HAUPPAUGE_HVR1500,
	},
};
const unsigned int cx23885_idcount = ARRAY_SIZE(cx23885_subids);

void cx23885_card_list(struct cx23885_dev *dev)
{
	int i;

	if (0 == dev->pci->subsystem_vendor &&
	    0 == dev->pci->subsystem_device) {
		printk("%s: Your board has no valid PCIe Subsystem ID and thus can't\n"
		       "%s: be autodetected.  Please pass card=<n> insmod option to\n"
		       "%s: workaround that.  Redirect complaints to the vendor of\n"
		       "%s: the TV card.  Best regards,\n"
		       "%s:         -- tux\n",
		       dev->name, dev->name, dev->name, dev->name, dev->name);
	} else {
		printk("%s: Your board isn't known (yet) to the driver.  You can\n"
		       "%s: try to pick one of the existing card configs via\n"
		       "%s: card=<n> insmod option.  Updating to the latest\n"
		       "%s: version might help as well.\n",
		       dev->name, dev->name, dev->name, dev->name);
	}
	printk("%s: Here is a list of valid choices for the card=<n> insmod option:\n",
	       dev->name);
	for (i = 0; i < cx23885_bcount; i++)
		printk("%s:    card=%d -> %s\n",
		       dev->name, i, cx23885_boards[i].name);
}

static void hauppauge_eeprom(struct cx23885_dev *dev, u8 *eeprom_data)
{
	struct tveeprom tv;

	tveeprom_hauppauge_analog(&dev->i2c_bus[0].i2c_client, &tv, eeprom_data);

	/* Make sure we support the board model */
	switch (tv.model)
	{
	case 76601: /* WinTV-HVR1800lp (PCIe, Retail, No IR, Dual channel ATSC and MPEG2 HW Encoder */
	case 77001: /* WinTV-HVR1500 (Express Card, OEM, No IR, ATSC and Basic analog */
	case 77011: /* WinTV-HVR1500 (Express Card, Retail, No IR, ATSC and Basic analog */
	case 77041: /* WinTV-HVR1500Q (Express Card, OEM, No IR, ATSC/QAM and Basic analog */
	case 77051: /* WinTV-HVR1500Q (Express Card, Retail, No IR, ATSC/QAM and Basic analog */
	case 78011: /* WinTV-HVR1800 (PCIe, Retail, 3.5mm in, IR, No FM, Dual channel ATSC and MPEG2 HW Encoder */
	case 78501: /* WinTV-HVR1800 (PCIe, OEM, RCA in, No IR, FM, Dual channel ATSC and MPEG2 HW Encoder */
	case 78521: /* WinTV-HVR1800 (PCIe, OEM, RCA in, No IR, FM, Dual channel ATSC and MPEG2 HW Encoder */
	case 78531: /* WinTV-HVR1800 (PCIe, OEM, RCA in, No IR, No FM, Dual channel ATSC and MPEG2 HW Encoder */
	case 78631: /* WinTV-HVR1800 (PCIe, OEM, No IR, No FM, Dual channel ATSC and MPEG2 HW Encoder */
	case 79001: /* WinTV-HVR1250 (PCIe, Retail, IR, full height, ATSC and Basic analog */
	case 79101: /* WinTV-HVR1250 (PCIe, Retail, IR, half height, ATSC and Basic analog */
	case 79571: /* WinTV-HVR1250 (PCIe, OEM, No IR, full height, ATSC and Basic analog */
	case 79671: /* WinTV-HVR1250 (PCIe, OEM, No IR, half height, ATSC and Basic analog */
		break;
	default:
		printk("%s: warning: unknown hauppauge model #%d\n", dev->name, tv.model);
		break;
	}

	printk(KERN_INFO "%s: hauppauge eeprom: model=%d\n",
			dev->name, tv.model);
}

/* Tuner callback function for cx23885 boards. Currently only needed
 * for HVR1500Q, which has an xc5000 tuner.
 */
int cx23885_tuner_callback(void *priv, int command, int arg)
{
	struct cx23885_i2c *bus = priv;
	struct cx23885_dev *dev = bus->dev;

	switch(dev->board) {
	case CX23885_BOARD_HAUPPAUGE_HVR1500Q:
		if(command == 0) {	/* Tuner Reset Command from xc5000 */
			/* Drive the tuner into reset and out */
			cx_clear(GP0_IO, 0x00000004);
			mdelay(200);
			cx_set(GP0_IO, 0x00000004);
			return 0;
		}
		else {
			printk(KERN_ERR
				"%s(): Unknow command.\n", __FUNCTION__);
			return -EINVAL;
		}
		break;
	}

	return 0; /* Should never be here */
}

void cx23885_gpio_setup(struct cx23885_dev *dev)
{
	switch(dev->board) {
	case CX23885_BOARD_HAUPPAUGE_HVR1250:
		/* GPIO-0 cx24227 demodulator reset */
		cx_set(GP0_IO, 0x00010001); /* Bring the part out of reset */
		break;
	case CX23885_BOARD_HAUPPAUGE_HVR1500:
		/* GPIO-0 cx24227 demodulator */
		/* GPIO-2 xc3028 tuner */

		/* Put the parts into reset */
		cx_set(GP0_IO, 0x00050000);
		cx_clear(GP0_IO, 0x00000005);
		msleep(5);

		/* Bring the parts out of reset */
		cx_set(GP0_IO, 0x00050005);
		break;
	case CX23885_BOARD_HAUPPAUGE_HVR1500Q:
		/* GPIO-0 cx24227 demodulator reset */
		/* GPIO-2 xc5000 tuner reset */
		cx_set(GP0_IO, 0x00050005); /* Bring the part out of reset */
		break;
	case CX23885_BOARD_HAUPPAUGE_HVR1800:
		/* GPIO-0 656_CLK */
		/* GPIO-1 656_D0 */
		/* GPIO-2 8295A Reset */
		/* GPIO-3-10 cx23417 data0-7 */
		/* GPIO-11-14 cx23417 addr0-3 */
		/* GPIO-15-18 cx23417 READY, CS, RD, WR */
		/* GPIO-19 IR_RX */

		/* Force the TDA8295A into reset and back */
		cx_set(GP0_IO, 0x00040004);
		mdelay(20);
		cx_clear(GP0_IO, 0x00000004);
		mdelay(20);
		cx_set(GP0_IO, 0x00040004);
		mdelay(20);
		break;
	}
}

int cx23885_ir_init(struct cx23885_dev *dev)
{
	switch (dev->board) {
	case CX23885_BOARD_HAUPPAUGE_HVR1250:
	case CX23885_BOARD_HAUPPAUGE_HVR1500:
	case CX23885_BOARD_HAUPPAUGE_HVR1500Q:
	case CX23885_BOARD_HAUPPAUGE_HVR1800:
		/* FIXME: Implement me */
		break;
	}

	return 0;
}

void cx23885_card_setup(struct cx23885_dev *dev)
{
	struct cx23885_tsport *ts1 = &dev->ts1;
	struct cx23885_tsport *ts2 = &dev->ts2;

	static u8 eeprom[256];

	if (dev->i2c_bus[0].i2c_rc == 0) {
		dev->i2c_bus[0].i2c_client.addr = 0xa0 >> 1;
		tveeprom_read(&dev->i2c_bus[0].i2c_client,
			      eeprom, sizeof(eeprom));
	}

	switch (dev->board) {
	case CX23885_BOARD_HAUPPAUGE_HVR1250:
	case CX23885_BOARD_HAUPPAUGE_HVR1500:
	case CX23885_BOARD_HAUPPAUGE_HVR1500Q:
	case CX23885_BOARD_HAUPPAUGE_HVR1800:
	case CX23885_BOARD_HAUPPAUGE_HVR1800lp:
		if (dev->i2c_bus[0].i2c_rc == 0)
			hauppauge_eeprom(dev, eeprom+0x80);
		break;
	}

	switch (dev->board) {
	case CX23885_BOARD_DVICO_FUSIONHDTV_5_EXP:
		ts1->gen_ctrl_val  = 0xc; /* Serial bus + punctured clock */
		ts1->ts_clk_en_val = 0x1; /* Enable TS_CLK */
		ts1->src_sel_val   = CX23885_SRC_SEL_PARALLEL_MPEG_VIDEO;
		break;
	case CX23885_BOARD_HAUPPAUGE_HVR1250:
	case CX23885_BOARD_HAUPPAUGE_HVR1500:
	case CX23885_BOARD_HAUPPAUGE_HVR1500Q:
	case CX23885_BOARD_HAUPPAUGE_HVR1800:
	case CX23885_BOARD_HAUPPAUGE_HVR1800lp:
	default:
		ts2->gen_ctrl_val  = 0xc; /* Serial bus + punctured clock */
		ts2->ts_clk_en_val = 0x1; /* Enable TS_CLK */
		ts2->src_sel_val   = CX23885_SRC_SEL_PARALLEL_MPEG_VIDEO;
	}

}

/* ------------------------------------------------------------------ */

/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 * kate: eol "unix"; indent-width 3; remove-trailing-space on; replace-trailing-space-save on; tab-width 8; replace-tabs off; space-indent off; mixed-indent off
 */
