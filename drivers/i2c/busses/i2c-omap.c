/*
 * TI OMAP I2C master mode driver
 *
 * Copyright (C) 2003 MontaVista Software, Inc.
 * Copyright (C) 2004 Texas Instruments.
 *
 * Updated to work with multiple I2C interfaces on 24xx by
 * Tony Lindgren <tony@atomide.com> and Imre Deak <imre.deak@nokia.com>
 * Copyright (C) 2005 Nokia Corporation
 *
 * Cleaned up by Juha Yrjölä <juha.yrjola@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/io.h>

/* timeout waiting for the controller to respond */
#define OMAP_I2C_TIMEOUT (msecs_to_jiffies(1000))

#define OMAP_I2C_REV_REG		0x00
#define OMAP_I2C_IE_REG			0x04
#define OMAP_I2C_STAT_REG		0x08
#define OMAP_I2C_IV_REG			0x0c
#define OMAP_I2C_SYSS_REG		0x10
#define OMAP_I2C_BUF_REG		0x14
#define OMAP_I2C_CNT_REG		0x18
#define OMAP_I2C_DATA_REG		0x1c
#define OMAP_I2C_SYSC_REG		0x20
#define OMAP_I2C_CON_REG		0x24
#define OMAP_I2C_OA_REG			0x28
#define OMAP_I2C_SA_REG			0x2c
#define OMAP_I2C_PSC_REG		0x30
#define OMAP_I2C_SCLL_REG		0x34
#define OMAP_I2C_SCLH_REG		0x38
#define OMAP_I2C_SYSTEST_REG		0x3c
#define OMAP_I2C_BUFSTAT_REG		0x40

/* I2C Interrupt Enable Register (OMAP_I2C_IE): */
#define OMAP_I2C_IE_XDR		(1 << 14)	/* TX Buffer drain int enable */
#define OMAP_I2C_IE_RDR		(1 << 13)	/* RX Buffer drain int enable */
#define OMAP_I2C_IE_XRDY	(1 << 4)	/* TX data ready int enable */
#define OMAP_I2C_IE_RRDY	(1 << 3)	/* RX data ready int enable */
#define OMAP_I2C_IE_ARDY	(1 << 2)	/* Access ready int enable */
#define OMAP_I2C_IE_NACK	(1 << 1)	/* No ack interrupt enable */
#define OMAP_I2C_IE_AL		(1 << 0)	/* Arbitration lost int ena */

/* I2C Status Register (OMAP_I2C_STAT): */
#define OMAP_I2C_STAT_XDR	(1 << 14)	/* TX Buffer draining */
#define OMAP_I2C_STAT_RDR	(1 << 13)	/* RX Buffer draining */
#define OMAP_I2C_STAT_BB	(1 << 12)	/* Bus busy */
#define OMAP_I2C_STAT_ROVR	(1 << 11)	/* Receive overrun */
#define OMAP_I2C_STAT_XUDF	(1 << 10)	/* Transmit underflow */
#define OMAP_I2C_STAT_AAS	(1 << 9)	/* Address as slave */
#define OMAP_I2C_STAT_AD0	(1 << 8)	/* Address zero */
#define OMAP_I2C_STAT_XRDY	(1 << 4)	/* Transmit data ready */
#define OMAP_I2C_STAT_RRDY	(1 << 3)	/* Receive data ready */
#define OMAP_I2C_STAT_ARDY	(1 << 2)	/* Register access ready */
#define OMAP_I2C_STAT_NACK	(1 << 1)	/* No ack interrupt enable */
#define OMAP_I2C_STAT_AL	(1 << 0)	/* Arbitration lost int ena */

/* I2C Buffer Configuration Register (OMAP_I2C_BUF): */
#define OMAP_I2C_BUF_RDMA_EN	(1 << 15)	/* RX DMA channel enable */
#define OMAP_I2C_BUF_RXFIF_CLR	(1 << 14)	/* RX FIFO Clear */
#define OMAP_I2C_BUF_XDMA_EN	(1 << 7)	/* TX DMA channel enable */
#define OMAP_I2C_BUF_TXFIF_CLR	(1 << 6)	/* TX FIFO Clear */

/* I2C Configuration Register (OMAP_I2C_CON): */
#define OMAP_I2C_CON_EN		(1 << 15)	/* I2C module enable */
#define OMAP_I2C_CON_BE		(1 << 14)	/* Big endian mode */
#define OMAP_I2C_CON_OPMODE_HS	(1 << 12)	/* High Speed support */
#define OMAP_I2C_CON_STB	(1 << 11)	/* Start byte mode (master) */
#define OMAP_I2C_CON_MST	(1 << 10)	/* Master/slave mode */
#define OMAP_I2C_CON_TRX	(1 << 9)	/* TX/RX mode (master only) */
#define OMAP_I2C_CON_XA		(1 << 8)	/* Expand address */
#define OMAP_I2C_CON_RM		(1 << 2)	/* Repeat mode (master only) */
#define OMAP_I2C_CON_STP	(1 << 1)	/* Stop cond (master only) */
#define OMAP_I2C_CON_STT	(1 << 0)	/* Start condition (master) */

/* I2C SCL time value when Master */
#define OMAP_I2C_SCLL_HSSCLL	8
#define OMAP_I2C_SCLH_HSSCLH	8

/* I2C System Test Register (OMAP_I2C_SYSTEST): */
#ifdef DEBUG
#define OMAP_I2C_SYSTEST_ST_EN		(1 << 15)	/* System test enable */
#define OMAP_I2C_SYSTEST_FREE		(1 << 14)	/* Free running mode */
#define OMAP_I2C_SYSTEST_TMODE_MASK	(3 << 12)	/* Test mode select */
#define OMAP_I2C_SYSTEST_TMODE_SHIFT	(12)		/* Test mode select */
#define OMAP_I2C_SYSTEST_SCL_I		(1 << 3)	/* SCL line sense in */
#define OMAP_I2C_SYSTEST_SCL_O		(1 << 2)	/* SCL line drive out */
#define OMAP_I2C_SYSTEST_SDA_I		(1 << 1)	/* SDA line sense in */
#define OMAP_I2C_SYSTEST_SDA_O		(1 << 0)	/* SDA line drive out */
#endif

/* I2C System Status register (OMAP_I2C_SYSS): */
#define OMAP_I2C_SYSS_RDONE		(1 << 0)	/* Reset Done */

/* I2C System Configuration Register (OMAP_I2C_SYSC): */
#define OMAP_I2C_SYSC_SRST		(1 << 1)	/* Soft Reset */

struct omap_i2c_dev {
	struct device		*dev;
	void __iomem		*base;		/* virtual */
	int			irq;
	struct clk		*iclk;		/* Interface clock */
	struct clk		*fclk;		/* Functional clock */
	struct completion	cmd_complete;
	struct resource		*ioarea;
	u32			speed;		/* Speed of bus in Khz */
	u16			cmd_err;
	u8			*buf;
	size_t			buf_len;
	struct i2c_adapter	adapter;
	u8			fifo_size;	/* use as flag and value
						 * fifo_size==0 implies no fifo
						 * if set, should be trsh+1
						 */
	unsigned		rev1:1;
	unsigned		b_hw:1;		/* bad h/w fixes */
	unsigned		idle:1;
	u16			iestate;	/* Saved interrupt register */
};

static inline void omap_i2c_write_reg(struct omap_i2c_dev *i2c_dev,
				      int reg, u16 val)
{
	__raw_writew(val, i2c_dev->base + reg);
}

static inline u16 omap_i2c_read_reg(struct omap_i2c_dev *i2c_dev, int reg)
{
	return __raw_readw(i2c_dev->base + reg);
}

static int omap_i2c_get_clocks(struct omap_i2c_dev *dev)
{
	if (cpu_is_omap16xx() || cpu_is_omap24xx()) {
		dev->iclk = clk_get(dev->dev, "i2c_ick");
		if (IS_ERR(dev->iclk)) {
			dev->iclk = NULL;
			return -ENODEV;
		}
	}

	dev->fclk = clk_get(dev->dev, "i2c_fck");
	if (IS_ERR(dev->fclk)) {
		if (dev->iclk != NULL) {
			clk_put(dev->iclk);
			dev->iclk = NULL;
		}
		dev->fclk = NULL;
		return -ENODEV;
	}

	return 0;
}

static void omap_i2c_put_clocks(struct omap_i2c_dev *dev)
{
	clk_put(dev->fclk);
	dev->fclk = NULL;
	if (dev->iclk != NULL) {
		clk_put(dev->iclk);
		dev->iclk = NULL;
	}
}

static void omap_i2c_unidle(struct omap_i2c_dev *dev)
{
	if (dev->iclk != NULL)
		clk_enable(dev->iclk);
	clk_enable(dev->fclk);
	dev->idle = 0;
	if (dev->iestate)
		omap_i2c_write_reg(dev, OMAP_I2C_IE_REG, dev->iestate);
}

static void omap_i2c_idle(struct omap_i2c_dev *dev)
{
	u16 iv;

	dev->iestate = omap_i2c_read_reg(dev, OMAP_I2C_IE_REG);
	omap_i2c_write_reg(dev, OMAP_I2C_IE_REG, 0);
	if (dev->rev1) {
		iv = omap_i2c_read_reg(dev, OMAP_I2C_IV_REG);	/* Read clears */
	} else {
		omap_i2c_write_reg(dev, OMAP_I2C_STAT_REG, dev->iestate);

		/* Flush posted write before the dev->idle store occurs */
		omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG);
	}
	dev->idle = 1;
	clk_disable(dev->fclk);
	if (dev->iclk != NULL)
		clk_disable(dev->iclk);
}

static int omap_i2c_init(struct omap_i2c_dev *dev)
{
	u16 psc = 0, scll = 0, sclh = 0;
	u16 fsscll = 0, fssclh = 0, hsscll = 0, hssclh = 0;
	unsigned long fclk_rate = 12000000;
	unsigned long timeout;
	unsigned long internal_clk = 0;

	if (!dev->rev1) {
		omap_i2c_write_reg(dev, OMAP_I2C_SYSC_REG, OMAP_I2C_SYSC_SRST);
		/* For some reason we need to set the EN bit before the
		 * reset done bit gets set. */
		timeout = jiffies + OMAP_I2C_TIMEOUT;
		omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, OMAP_I2C_CON_EN);
		while (!(omap_i2c_read_reg(dev, OMAP_I2C_SYSS_REG) &
			 OMAP_I2C_SYSS_RDONE)) {
			if (time_after(jiffies, timeout)) {
				dev_warn(dev->dev, "timeout waiting "
						"for controller reset\n");
				return -ETIMEDOUT;
			}
			msleep(1);
		}
	}
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, 0);

	if (cpu_class_is_omap1()) {
		struct clk *armxor_ck;

		armxor_ck = clk_get(NULL, "armxor_ck");
		if (IS_ERR(armxor_ck))
			dev_warn(dev->dev, "Could not get armxor_ck\n");
		else {
			fclk_rate = clk_get_rate(armxor_ck);
			clk_put(armxor_ck);
		}
		/* TRM for 5912 says the I2C clock must be prescaled to be
		 * between 7 - 12 MHz. The XOR input clock is typically
		 * 12, 13 or 19.2 MHz. So we should have code that produces:
		 *
		 * XOR MHz	Divider		Prescaler
		 * 12		1		0
		 * 13		2		1
		 * 19.2		2		1
		 */
		if (fclk_rate > 12000000)
			psc = fclk_rate / 12000000;
	}

	if (cpu_is_omap2430()) {

		/* HSI2C controller internal clk rate should be 19.2 Mhz */
		internal_clk = 19200;
		fclk_rate = clk_get_rate(dev->fclk) / 1000;

		/* Compute prescaler divisor */
		psc = fclk_rate / internal_clk;
		psc = psc - 1;

		/* If configured for High Speed */
		if (dev->speed > 400) {
			/* For first phase of HS mode */
			fsscll = internal_clk / (400 * 2) - 6;
			fssclh = internal_clk / (400 * 2) - 6;

			/* For second phase of HS mode */
			hsscll = fclk_rate / (dev->speed * 2) - 6;
			hssclh = fclk_rate / (dev->speed * 2) - 6;
		} else {
			/* To handle F/S modes */
			fsscll = internal_clk / (dev->speed * 2) - 6;
			fssclh = internal_clk / (dev->speed * 2) - 6;
		}
		scll = (hsscll << OMAP_I2C_SCLL_HSSCLL) | fsscll;
		sclh = (hssclh << OMAP_I2C_SCLH_HSSCLH) | fssclh;
	} else {
		/* Program desired operating rate */
		fclk_rate /= (psc + 1) * 1000;
		if (psc > 2)
			psc = 2;
		scll = fclk_rate / (dev->speed * 2) - 7 + psc;
		sclh = fclk_rate / (dev->speed * 2) - 7 + psc;
	}

	/* Setup clock prescaler to obtain approx 12MHz I2C module clock: */
	omap_i2c_write_reg(dev, OMAP_I2C_PSC_REG, psc);

	/* SCL low and high time values */
	omap_i2c_write_reg(dev, OMAP_I2C_SCLL_REG, scll);
	omap_i2c_write_reg(dev, OMAP_I2C_SCLH_REG, sclh);

	if (dev->fifo_size)
		/* Note: setup required fifo size - 1 */
		omap_i2c_write_reg(dev, OMAP_I2C_BUF_REG,
					(dev->fifo_size - 1) << 8 | /* RTRSH */
					OMAP_I2C_BUF_RXFIF_CLR |
					(dev->fifo_size - 1) | /* XTRSH */
					OMAP_I2C_BUF_TXFIF_CLR);

	/* Take the I2C module out of reset: */
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, OMAP_I2C_CON_EN);

	/* Enable interrupts */
	omap_i2c_write_reg(dev, OMAP_I2C_IE_REG,
			   (OMAP_I2C_IE_XRDY | OMAP_I2C_IE_RRDY |
			    OMAP_I2C_IE_ARDY | OMAP_I2C_IE_NACK |
			    OMAP_I2C_IE_AL)  | ((dev->fifo_size) ?
				(OMAP_I2C_IE_RDR | OMAP_I2C_IE_XDR) : 0));
	return 0;
}

/*
 * Waiting on Bus Busy
 */
static int omap_i2c_wait_for_bb(struct omap_i2c_dev *dev)
{
	unsigned long timeout;

	timeout = jiffies + OMAP_I2C_TIMEOUT;
	while (omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG) & OMAP_I2C_STAT_BB) {
		if (time_after(jiffies, timeout)) {
			dev_warn(dev->dev, "timeout waiting for bus ready\n");
			return -ETIMEDOUT;
		}
		msleep(1);
	}

	return 0;
}

/*
 * Low level master read/write transaction.
 */
static int omap_i2c_xfer_msg(struct i2c_adapter *adap,
			     struct i2c_msg *msg, int stop)
{
	struct omap_i2c_dev *dev = i2c_get_adapdata(adap);
	int r;
	u16 w;

	dev_dbg(dev->dev, "addr: 0x%04x, len: %d, flags: 0x%x, stop: %d\n",
		msg->addr, msg->len, msg->flags, stop);

	if (msg->len == 0)
		return -EINVAL;

	omap_i2c_write_reg(dev, OMAP_I2C_SA_REG, msg->addr);

	/* REVISIT: Could the STB bit of I2C_CON be used with probing? */
	dev->buf = msg->buf;
	dev->buf_len = msg->len;

	omap_i2c_write_reg(dev, OMAP_I2C_CNT_REG, dev->buf_len);

	/* Clear the FIFO Buffers */
	w = omap_i2c_read_reg(dev, OMAP_I2C_BUF_REG);
	w |= OMAP_I2C_BUF_RXFIF_CLR | OMAP_I2C_BUF_TXFIF_CLR;
	omap_i2c_write_reg(dev, OMAP_I2C_BUF_REG, w);

	init_completion(&dev->cmd_complete);
	dev->cmd_err = 0;

	w = OMAP_I2C_CON_EN | OMAP_I2C_CON_MST | OMAP_I2C_CON_STT;

	/* High speed configuration */
	if (dev->speed > 400)
		w |= OMAP_I2C_CON_OPMODE_HS;

	if (msg->flags & I2C_M_TEN)
		w |= OMAP_I2C_CON_XA;
	if (!(msg->flags & I2C_M_RD))
		w |= OMAP_I2C_CON_TRX;
	if (!dev->b_hw && stop)
		w |= OMAP_I2C_CON_STP;
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, w);

	/*
	 * Don't write stt and stp together on some hardware.
	 */
	if (dev->b_hw && stop) {
		unsigned long delay = jiffies + OMAP_I2C_TIMEOUT;
		u16 con = omap_i2c_read_reg(dev, OMAP_I2C_CON_REG);
		while (con & OMAP_I2C_CON_STT) {
			con = omap_i2c_read_reg(dev, OMAP_I2C_CON_REG);

			/* Let the user know if i2c is in a bad state */
			if (time_after(jiffies, delay)) {
				dev_err(dev->dev, "controller timed out "
				"waiting for start condition to finish\n");
				return -ETIMEDOUT;
			}
			cpu_relax();
		}

		w |= OMAP_I2C_CON_STP;
		w &= ~OMAP_I2C_CON_STT;
		omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, w);
	}

	/*
	 * REVISIT: We should abort the transfer on signals, but the bus goes
	 * into arbitration and we're currently unable to recover from it.
	 */
	r = wait_for_completion_timeout(&dev->cmd_complete,
					OMAP_I2C_TIMEOUT);
	dev->buf_len = 0;
	if (r < 0)
		return r;
	if (r == 0) {
		dev_err(dev->dev, "controller timed out\n");
		omap_i2c_init(dev);
		return -ETIMEDOUT;
	}

	if (likely(!dev->cmd_err))
		return 0;

	/* We have an error */
	if (dev->cmd_err & (OMAP_I2C_STAT_AL | OMAP_I2C_STAT_ROVR |
			    OMAP_I2C_STAT_XUDF)) {
		omap_i2c_init(dev);
		return -EIO;
	}

	if (dev->cmd_err & OMAP_I2C_STAT_NACK) {
		if (msg->flags & I2C_M_IGNORE_NAK)
			return 0;
		if (stop) {
			w = omap_i2c_read_reg(dev, OMAP_I2C_CON_REG);
			w |= OMAP_I2C_CON_STP;
			omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, w);
		}
		return -EREMOTEIO;
	}
	return -EIO;
}


/*
 * Prepare controller for a transaction and call omap_i2c_xfer_msg
 * to do the work during IRQ processing.
 */
static int
omap_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct omap_i2c_dev *dev = i2c_get_adapdata(adap);
	int i;
	int r;

	omap_i2c_unidle(dev);

	if ((r = omap_i2c_wait_for_bb(dev)) < 0)
		goto out;

	for (i = 0; i < num; i++) {
		r = omap_i2c_xfer_msg(adap, &msgs[i], (i == (num - 1)));
		if (r != 0)
			break;
	}

	if (r == 0)
		r = num;
out:
	omap_i2c_idle(dev);
	return r;
}

static u32
omap_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK);
}

static inline void
omap_i2c_complete_cmd(struct omap_i2c_dev *dev, u16 err)
{
	dev->cmd_err |= err;
	complete(&dev->cmd_complete);
}

static inline void
omap_i2c_ack_stat(struct omap_i2c_dev *dev, u16 stat)
{
	omap_i2c_write_reg(dev, OMAP_I2C_STAT_REG, stat);
}

static irqreturn_t
omap_i2c_rev1_isr(int this_irq, void *dev_id)
{
	struct omap_i2c_dev *dev = dev_id;
	u16 iv, w;

	if (dev->idle)
		return IRQ_NONE;

	iv = omap_i2c_read_reg(dev, OMAP_I2C_IV_REG);
	switch (iv) {
	case 0x00:	/* None */
		break;
	case 0x01:	/* Arbitration lost */
		dev_err(dev->dev, "Arbitration lost\n");
		omap_i2c_complete_cmd(dev, OMAP_I2C_STAT_AL);
		break;
	case 0x02:	/* No acknowledgement */
		omap_i2c_complete_cmd(dev, OMAP_I2C_STAT_NACK);
		omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, OMAP_I2C_CON_STP);
		break;
	case 0x03:	/* Register access ready */
		omap_i2c_complete_cmd(dev, 0);
		break;
	case 0x04:	/* Receive data ready */
		if (dev->buf_len) {
			w = omap_i2c_read_reg(dev, OMAP_I2C_DATA_REG);
			*dev->buf++ = w;
			dev->buf_len--;
			if (dev->buf_len) {
				*dev->buf++ = w >> 8;
				dev->buf_len--;
			}
		} else
			dev_err(dev->dev, "RRDY IRQ while no data requested\n");
		break;
	case 0x05:	/* Transmit data ready */
		if (dev->buf_len) {
			w = *dev->buf++;
			dev->buf_len--;
			if (dev->buf_len) {
				w |= *dev->buf++ << 8;
				dev->buf_len--;
			}
			omap_i2c_write_reg(dev, OMAP_I2C_DATA_REG, w);
		} else
			dev_err(dev->dev, "XRDY IRQ while no data to send\n");
		break;
	default:
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static irqreturn_t
omap_i2c_isr(int this_irq, void *dev_id)
{
	struct omap_i2c_dev *dev = dev_id;
	u16 bits;
	u16 stat, w;
	int err, count = 0;

	if (dev->idle)
		return IRQ_NONE;

	bits = omap_i2c_read_reg(dev, OMAP_I2C_IE_REG);
	while ((stat = (omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG))) & bits) {
		dev_dbg(dev->dev, "IRQ (ISR = 0x%04x)\n", stat);
		if (count++ == 100) {
			dev_warn(dev->dev, "Too much work in one IRQ\n");
			break;
		}

		omap_i2c_write_reg(dev, OMAP_I2C_STAT_REG, stat);

		err = 0;
		if (stat & OMAP_I2C_STAT_NACK) {
			err |= OMAP_I2C_STAT_NACK;
			omap_i2c_write_reg(dev, OMAP_I2C_CON_REG,
					   OMAP_I2C_CON_STP);
		}
		if (stat & OMAP_I2C_STAT_AL) {
			dev_err(dev->dev, "Arbitration lost\n");
			err |= OMAP_I2C_STAT_AL;
		}
		if (stat & (OMAP_I2C_STAT_ARDY | OMAP_I2C_STAT_NACK |
					OMAP_I2C_STAT_AL))
			omap_i2c_complete_cmd(dev, err);
		if (stat & (OMAP_I2C_STAT_RRDY | OMAP_I2C_STAT_RDR)) {
			u8 num_bytes = 1;
			if (dev->fifo_size) {
				if (stat & OMAP_I2C_STAT_RRDY)
					num_bytes = dev->fifo_size;
				else
					num_bytes = omap_i2c_read_reg(dev,
							OMAP_I2C_BUFSTAT_REG);
			}
			while (num_bytes) {
				num_bytes--;
				w = omap_i2c_read_reg(dev, OMAP_I2C_DATA_REG);
				if (dev->buf_len) {
					*dev->buf++ = w;
					dev->buf_len--;
					/* Data reg from 2430 is 8 bit wide */
					if (!cpu_is_omap2430()) {
						if (dev->buf_len) {
							*dev->buf++ = w >> 8;
							dev->buf_len--;
						}
					}
				} else {
					if (stat & OMAP_I2C_STAT_RRDY)
						dev_err(dev->dev,
							"RRDY IRQ while no data"
								" requested\n");
					if (stat & OMAP_I2C_STAT_RDR)
						dev_err(dev->dev,
							"RDR IRQ while no data"
								" requested\n");
					break;
				}
			}
			omap_i2c_ack_stat(dev,
				stat & (OMAP_I2C_STAT_RRDY | OMAP_I2C_STAT_RDR));
			continue;
		}
		if (stat & (OMAP_I2C_STAT_XRDY | OMAP_I2C_STAT_XDR)) {
			u8 num_bytes = 1;
			if (dev->fifo_size) {
				if (stat & OMAP_I2C_STAT_XRDY)
					num_bytes = dev->fifo_size;
				else
					num_bytes = omap_i2c_read_reg(dev,
							OMAP_I2C_BUFSTAT_REG);
			}
			while (num_bytes) {
				num_bytes--;
				w = 0;
				if (dev->buf_len) {
					w = *dev->buf++;
					dev->buf_len--;
					/* Data reg from  2430 is 8 bit wide */
					if (!cpu_is_omap2430()) {
						if (dev->buf_len) {
							w |= *dev->buf++ << 8;
							dev->buf_len--;
						}
					}
				} else {
					if (stat & OMAP_I2C_STAT_XRDY)
						dev_err(dev->dev,
							"XRDY IRQ while no "
							"data to send\n");
					if (stat & OMAP_I2C_STAT_XDR)
						dev_err(dev->dev,
							"XDR IRQ while no "
							"data to send\n");
					break;
				}
				omap_i2c_write_reg(dev, OMAP_I2C_DATA_REG, w);
			}
			omap_i2c_ack_stat(dev,
				stat & (OMAP_I2C_STAT_XRDY | OMAP_I2C_STAT_XDR));
			continue;
		}
		if (stat & OMAP_I2C_STAT_ROVR) {
			dev_err(dev->dev, "Receive overrun\n");
			dev->cmd_err |= OMAP_I2C_STAT_ROVR;
		}
		if (stat & OMAP_I2C_STAT_XUDF) {
			dev_err(dev->dev, "Transmit underflow\n");
			dev->cmd_err |= OMAP_I2C_STAT_XUDF;
		}
	}

	return count ? IRQ_HANDLED : IRQ_NONE;
}

static const struct i2c_algorithm omap_i2c_algo = {
	.master_xfer	= omap_i2c_xfer,
	.functionality	= omap_i2c_func,
};

static int
omap_i2c_probe(struct platform_device *pdev)
{
	struct omap_i2c_dev	*dev;
	struct i2c_adapter	*adap;
	struct resource		*mem, *irq, *ioarea;
	int r;
	u32 *speed = NULL;

	/* NOTE: driver uses the static register mapping */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}
	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return -ENODEV;
	}

	ioarea = request_mem_region(mem->start, (mem->end - mem->start) + 1,
			pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "I2C region already claimed\n");
		return -EBUSY;
	}

	dev = kzalloc(sizeof(struct omap_i2c_dev), GFP_KERNEL);
	if (!dev) {
		r = -ENOMEM;
		goto err_release_region;
	}

	if (pdev->dev.platform_data != NULL)
		speed = (u32 *) pdev->dev.platform_data;
	else
		*speed = 100; /* Defualt speed */

	dev->speed = *speed;
	dev->dev = &pdev->dev;
	dev->irq = irq->start;
	dev->base = ioremap(mem->start, mem->end - mem->start + 1);
	if (!dev->base) {
		r = -ENOMEM;
		goto err_free_mem;
	}

	platform_set_drvdata(pdev, dev);

	if ((r = omap_i2c_get_clocks(dev)) != 0)
		goto err_iounmap;

	omap_i2c_unidle(dev);

	if (cpu_is_omap15xx())
		dev->rev1 = omap_i2c_read_reg(dev, OMAP_I2C_REV_REG) < 0x20;

	if (cpu_is_omap2430()) {
		u16 s;

		/* Set up the fifo size - Get total size */
		s = (omap_i2c_read_reg(dev, OMAP_I2C_BUFSTAT_REG) >> 14) & 0x3;
		dev->fifo_size = 0x8 << s;

		/*
		 * Set up notification threshold as half the total available
		 * size. This is to ensure that we can handle the status on int
		 * call back latencies.
		 */
		dev->fifo_size = (dev->fifo_size / 2);
		dev->b_hw = 1; /* Enable hardware fixes */
	}

	/* reset ASAP, clearing any IRQs */
	omap_i2c_init(dev);

	r = request_irq(dev->irq, dev->rev1 ? omap_i2c_rev1_isr : omap_i2c_isr,
			0, pdev->name, dev);

	if (r) {
		dev_err(dev->dev, "failure requesting irq %i\n", dev->irq);
		goto err_unuse_clocks;
	}
	r = omap_i2c_read_reg(dev, OMAP_I2C_REV_REG) & 0xff;
	dev_info(dev->dev, "bus %d rev%d.%d at %d kHz\n",
		 pdev->id, r >> 4, r & 0xf, dev->speed);

	adap = &dev->adapter;
	i2c_set_adapdata(adap, dev);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strncpy(adap->name, "OMAP I2C adapter", sizeof(adap->name));
	adap->algo = &omap_i2c_algo;
	adap->dev.parent = &pdev->dev;

	/* i2c device drivers may be active on return from add_adapter() */
	adap->nr = pdev->id;
	r = i2c_add_numbered_adapter(adap);
	if (r) {
		dev_err(dev->dev, "failure adding adapter\n");
		goto err_free_irq;
	}

	omap_i2c_idle(dev);

	return 0;

err_free_irq:
	free_irq(dev->irq, dev);
err_unuse_clocks:
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, 0);
	omap_i2c_idle(dev);
	omap_i2c_put_clocks(dev);
err_iounmap:
	iounmap(dev->base);
err_free_mem:
	platform_set_drvdata(pdev, NULL);
	kfree(dev);
err_release_region:
	release_mem_region(mem->start, (mem->end - mem->start) + 1);

	return r;
}

static int
omap_i2c_remove(struct platform_device *pdev)
{
	struct omap_i2c_dev	*dev = platform_get_drvdata(pdev);
	struct resource		*mem;

	platform_set_drvdata(pdev, NULL);

	free_irq(dev->irq, dev);
	i2c_del_adapter(&dev->adapter);
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, 0);
	omap_i2c_put_clocks(dev);
	iounmap(dev->base);
	kfree(dev);
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(mem->start, (mem->end - mem->start) + 1);
	return 0;
}

static struct platform_driver omap_i2c_driver = {
	.probe		= omap_i2c_probe,
	.remove		= omap_i2c_remove,
	.driver		= {
		.name	= "i2c_omap",
		.owner	= THIS_MODULE,
	},
};

/* I2C may be needed to bring up other drivers */
static int __init
omap_i2c_init_driver(void)
{
	return platform_driver_register(&omap_i2c_driver);
}
subsys_initcall(omap_i2c_init_driver);

static void __exit omap_i2c_exit_driver(void)
{
	platform_driver_unregister(&omap_i2c_driver);
}
module_exit(omap_i2c_exit_driver);

MODULE_AUTHOR("MontaVista Software, Inc. (and others)");
MODULE_DESCRIPTION("TI OMAP I2C bus adapter");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:i2c_omap");
