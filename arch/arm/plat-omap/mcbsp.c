/*
 * linux/arch/arm/plat-omap/mcbsp.c
 *
 * Copyright (C) 2004 Nokia Corporation
 * Author: Samuel Ortiz <samuel.ortiz@nokia.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Multichannel mode not supported.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <mach/dma.h>
#include <mach/mcbsp.h>

static struct omap_mcbsp mcbsp[OMAP_MAX_MCBSP_COUNT];

#define omap_mcbsp_check_valid_id(id)	(mcbsp[id].pdata && \
					mcbsp[id].pdata->ops && \
					mcbsp[id].pdata->ops->check && \
					(mcbsp[id].pdata->ops->check(id) == 0))

static void omap_mcbsp_dump_reg(u8 id)
{
	dev_dbg(mcbsp[id].dev, "**** McBSP%d regs ****\n", mcbsp[id].id);
	dev_dbg(mcbsp[id].dev, "DRR2:  0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, DRR2));
	dev_dbg(mcbsp[id].dev, "DRR1:  0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, DRR1));
	dev_dbg(mcbsp[id].dev, "DXR2:  0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, DXR2));
	dev_dbg(mcbsp[id].dev, "DXR1:  0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, DXR1));
	dev_dbg(mcbsp[id].dev, "SPCR2: 0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, SPCR2));
	dev_dbg(mcbsp[id].dev, "SPCR1: 0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, SPCR1));
	dev_dbg(mcbsp[id].dev, "RCR2:  0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, RCR2));
	dev_dbg(mcbsp[id].dev, "RCR1:  0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, RCR1));
	dev_dbg(mcbsp[id].dev, "XCR2:  0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, XCR2));
	dev_dbg(mcbsp[id].dev, "XCR1:  0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, XCR1));
	dev_dbg(mcbsp[id].dev, "SRGR2: 0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, SRGR2));
	dev_dbg(mcbsp[id].dev, "SRGR1: 0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, SRGR1));
	dev_dbg(mcbsp[id].dev, "PCR0:  0x%04x\n",
			OMAP_MCBSP_READ(mcbsp[id].io_base, PCR0));
	dev_dbg(mcbsp[id].dev, "***********************\n");
}

static irqreturn_t omap_mcbsp_tx_irq_handler(int irq, void *dev_id)
{
	struct omap_mcbsp *mcbsp_tx = dev_id;

	dev_dbg(mcbsp_tx->dev, "TX IRQ callback : 0x%x\n",
		OMAP_MCBSP_READ(mcbsp_tx->io_base, SPCR2));

	complete(&mcbsp_tx->tx_irq_completion);

	return IRQ_HANDLED;
}

static irqreturn_t omap_mcbsp_rx_irq_handler(int irq, void *dev_id)
{
	struct omap_mcbsp *mcbsp_rx = dev_id;

	dev_dbg(mcbsp_rx->dev, "RX IRQ callback : 0x%x\n",
		OMAP_MCBSP_READ(mcbsp_rx->io_base, SPCR2));

	complete(&mcbsp_rx->rx_irq_completion);

	return IRQ_HANDLED;
}

static void omap_mcbsp_tx_dma_callback(int lch, u16 ch_status, void *data)
{
	struct omap_mcbsp *mcbsp_dma_tx = data;

	dev_dbg(mcbsp_dma_tx->dev, "TX DMA callback : 0x%x\n",
		OMAP_MCBSP_READ(mcbsp_dma_tx->io_base, SPCR2));

	/* We can free the channels */
	omap_free_dma(mcbsp_dma_tx->dma_tx_lch);
	mcbsp_dma_tx->dma_tx_lch = -1;

	complete(&mcbsp_dma_tx->tx_dma_completion);
}

static void omap_mcbsp_rx_dma_callback(int lch, u16 ch_status, void *data)
{
	struct omap_mcbsp *mcbsp_dma_rx = data;

	dev_dbg(mcbsp_dma_rx->dev, "RX DMA callback : 0x%x\n",
		OMAP_MCBSP_READ(mcbsp_dma_rx->io_base, SPCR2));

	/* We can free the channels */
	omap_free_dma(mcbsp_dma_rx->dma_rx_lch);
	mcbsp_dma_rx->dma_rx_lch = -1;

	complete(&mcbsp_dma_rx->rx_dma_completion);
}

/*
 * omap_mcbsp_config simply write a config to the
 * appropriate McBSP.
 * You either call this function or set the McBSP registers
 * by yourself before calling omap_mcbsp_start().
 */
void omap_mcbsp_config(unsigned int id, const struct omap_mcbsp_reg_cfg *config)
{
	u32 io_base;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return;
	}

	io_base = mcbsp[id].io_base;
	dev_dbg(mcbsp[id].dev, "Configuring McBSP%d  io_base: 0x%8x\n",
			mcbsp[id].id, io_base);

	/* We write the given config */
	OMAP_MCBSP_WRITE(io_base, SPCR2, config->spcr2);
	OMAP_MCBSP_WRITE(io_base, SPCR1, config->spcr1);
	OMAP_MCBSP_WRITE(io_base, RCR2, config->rcr2);
	OMAP_MCBSP_WRITE(io_base, RCR1, config->rcr1);
	OMAP_MCBSP_WRITE(io_base, XCR2, config->xcr2);
	OMAP_MCBSP_WRITE(io_base, XCR1, config->xcr1);
	OMAP_MCBSP_WRITE(io_base, SRGR2, config->srgr2);
	OMAP_MCBSP_WRITE(io_base, SRGR1, config->srgr1);
	OMAP_MCBSP_WRITE(io_base, MCR2, config->mcr2);
	OMAP_MCBSP_WRITE(io_base, MCR1, config->mcr1);
	OMAP_MCBSP_WRITE(io_base, PCR0, config->pcr0);
}
EXPORT_SYMBOL(omap_mcbsp_config);

/*
 * We can choose between IRQ based or polled IO.
 * This needs to be called before omap_mcbsp_request().
 */
int omap_mcbsp_set_io_type(unsigned int id, omap_mcbsp_io_type_t io_type)
{
	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return -ENODEV;
	}

	spin_lock(&mcbsp[id].lock);

	if (!mcbsp[id].free) {
		dev_err(mcbsp[id].dev, "McBSP%d is currently in use\n",
			mcbsp[id].id);
		spin_unlock(&mcbsp[id].lock);
		return -EINVAL;
	}

	mcbsp[id].io_type = io_type;

	spin_unlock(&mcbsp[id].lock);

	return 0;
}
EXPORT_SYMBOL(omap_mcbsp_set_io_type);

int omap_mcbsp_request(unsigned int id)
{
	int err;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return -ENODEV;
	}

	if (mcbsp[id].pdata->ops->request)
		mcbsp[id].pdata->ops->request(id);

	clk_enable(mcbsp[id].clk);

	spin_lock(&mcbsp[id].lock);
	if (!mcbsp[id].free) {
		dev_err(mcbsp[id].dev, "McBSP%d is currently in use\n",
			mcbsp[id].id);
		spin_unlock(&mcbsp[id].lock);
		return -1;
	}

	mcbsp[id].free = 0;
	spin_unlock(&mcbsp[id].lock);

	if (mcbsp[id].io_type == OMAP_MCBSP_IRQ_IO) {
		/* We need to get IRQs here */
		err = request_irq(mcbsp[id].tx_irq, omap_mcbsp_tx_irq_handler,
					0, "McBSP", (void *) (&mcbsp[id]));
		if (err != 0) {
			dev_err(mcbsp[id].dev, "Unable to request TX IRQ %d "
					"for McBSP%d\n", mcbsp[id].tx_irq,
					mcbsp[id].id);
			return err;
		}

		init_completion(&(mcbsp[id].tx_irq_completion));

		err = request_irq(mcbsp[id].rx_irq, omap_mcbsp_rx_irq_handler,
					0, "McBSP", (void *) (&mcbsp[id]));
		if (err != 0) {
			dev_err(mcbsp[id].dev, "Unable to request RX IRQ %d "
					"for McBSP%d\n", mcbsp[id].rx_irq,
					mcbsp[id].id);
			free_irq(mcbsp[id].tx_irq, (void *) (&mcbsp[id]));
			return err;
		}

		init_completion(&(mcbsp[id].rx_irq_completion));
	}

	return 0;
}
EXPORT_SYMBOL(omap_mcbsp_request);

void omap_mcbsp_free(unsigned int id)
{
	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return;
	}

	if (mcbsp[id].pdata->ops->free)
		mcbsp[id].pdata->ops->free(id);

	clk_disable(mcbsp[id].clk);

	spin_lock(&mcbsp[id].lock);
	if (mcbsp[id].free) {
		dev_err(mcbsp[id].dev, "McBSP%d was not reserved\n",
			mcbsp[id].id);
		spin_unlock(&mcbsp[id].lock);
		return;
	}

	mcbsp[id].free = 1;
	spin_unlock(&mcbsp[id].lock);

	if (mcbsp[id].io_type == OMAP_MCBSP_IRQ_IO) {
		/* Free IRQs */
		free_irq(mcbsp[id].rx_irq, (void *) (&mcbsp[id]));
		free_irq(mcbsp[id].tx_irq, (void *) (&mcbsp[id]));
	}
}
EXPORT_SYMBOL(omap_mcbsp_free);

/*
 * Here we start the McBSP, by enabling the sample
 * generator, both transmitter and receivers,
 * and the frame sync.
 */
void omap_mcbsp_start(unsigned int id)
{
	u32 io_base;
	u16 w;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return;
	}

	io_base = mcbsp[id].io_base;

	mcbsp[id].rx_word_length = (OMAP_MCBSP_READ(io_base, RCR1) >> 5) & 0x7;
	mcbsp[id].tx_word_length = (OMAP_MCBSP_READ(io_base, XCR1) >> 5) & 0x7;

	/* Start the sample generator */
	w = OMAP_MCBSP_READ(io_base, SPCR2);
	OMAP_MCBSP_WRITE(io_base, SPCR2, w | (1 << 6));

	/* Enable transmitter and receiver */
	w = OMAP_MCBSP_READ(io_base, SPCR2);
	OMAP_MCBSP_WRITE(io_base, SPCR2, w | 1);

	w = OMAP_MCBSP_READ(io_base, SPCR1);
	OMAP_MCBSP_WRITE(io_base, SPCR1, w | 1);

	udelay(100);

	/* Start frame sync */
	w = OMAP_MCBSP_READ(io_base, SPCR2);
	OMAP_MCBSP_WRITE(io_base, SPCR2, w | (1 << 7));

	/* Dump McBSP Regs */
	omap_mcbsp_dump_reg(id);
}
EXPORT_SYMBOL(omap_mcbsp_start);

void omap_mcbsp_stop(unsigned int id)
{
	u32 io_base;
	u16 w;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return;
	}

	io_base = mcbsp[id].io_base;

	/* Reset transmitter */
	w = OMAP_MCBSP_READ(io_base, SPCR2);
	OMAP_MCBSP_WRITE(io_base, SPCR2, w & ~(1));

	/* Reset receiver */
	w = OMAP_MCBSP_READ(io_base, SPCR1);
	OMAP_MCBSP_WRITE(io_base, SPCR1, w & ~(1));

	/* Reset the sample rate generator */
	w = OMAP_MCBSP_READ(io_base, SPCR2);
	OMAP_MCBSP_WRITE(io_base, SPCR2, w & ~(1 << 6));
}
EXPORT_SYMBOL(omap_mcbsp_stop);

/* polled mcbsp i/o operations */
int omap_mcbsp_pollwrite(unsigned int id, u16 buf)
{
	u32 base;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return -ENODEV;
	}

	base = mcbsp[id].io_base;
	writew(buf, base + OMAP_MCBSP_REG_DXR1);
	/* if frame sync error - clear the error */
	if (readw(base + OMAP_MCBSP_REG_SPCR2) & XSYNC_ERR) {
		/* clear error */
		writew(readw(base + OMAP_MCBSP_REG_SPCR2) & (~XSYNC_ERR),
		       base + OMAP_MCBSP_REG_SPCR2);
		/* resend */
		return -1;
	} else {
		/* wait for transmit confirmation */
		int attemps = 0;
		while (!(readw(base + OMAP_MCBSP_REG_SPCR2) & XRDY)) {
			if (attemps++ > 1000) {
				writew(readw(base + OMAP_MCBSP_REG_SPCR2) &
				       (~XRST),
				       base + OMAP_MCBSP_REG_SPCR2);
				udelay(10);
				writew(readw(base + OMAP_MCBSP_REG_SPCR2) |
				       (XRST),
				       base + OMAP_MCBSP_REG_SPCR2);
				udelay(10);
				dev_err(mcbsp[id].dev, "Could not write to"
					" McBSP%d Register\n", mcbsp[id].id);
				return -2;
			}
		}
	}

	return 0;
}
EXPORT_SYMBOL(omap_mcbsp_pollwrite);

int omap_mcbsp_pollread(unsigned int id, u16 *buf)
{
	u32 base;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return -ENODEV;
	}

	base = mcbsp[id].io_base;
	/* if frame sync error - clear the error */
	if (readw(base + OMAP_MCBSP_REG_SPCR1) & RSYNC_ERR) {
		/* clear error */
		writew(readw(base + OMAP_MCBSP_REG_SPCR1) & (~RSYNC_ERR),
		       base + OMAP_MCBSP_REG_SPCR1);
		/* resend */
		return -1;
	} else {
		/* wait for recieve confirmation */
		int attemps = 0;
		while (!(readw(base + OMAP_MCBSP_REG_SPCR1) & RRDY)) {
			if (attemps++ > 1000) {
				writew(readw(base + OMAP_MCBSP_REG_SPCR1) &
				       (~RRST),
				       base + OMAP_MCBSP_REG_SPCR1);
				udelay(10);
				writew(readw(base + OMAP_MCBSP_REG_SPCR1) |
				       (RRST),
				       base + OMAP_MCBSP_REG_SPCR1);
				udelay(10);
				dev_err(mcbsp[id].dev, "Could not read from"
					" McBSP%d Register\n", mcbsp[id].id);
				return -2;
			}
		}
	}
	*buf = readw(base + OMAP_MCBSP_REG_DRR1);

	return 0;
}
EXPORT_SYMBOL(omap_mcbsp_pollread);

/*
 * IRQ based word transmission.
 */
void omap_mcbsp_xmit_word(unsigned int id, u32 word)
{
	u32 io_base;
	omap_mcbsp_word_length word_length;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return;
	}

	io_base = mcbsp[id].io_base;
	word_length = mcbsp[id].tx_word_length;

	wait_for_completion(&(mcbsp[id].tx_irq_completion));

	if (word_length > OMAP_MCBSP_WORD_16)
		OMAP_MCBSP_WRITE(io_base, DXR2, word >> 16);
	OMAP_MCBSP_WRITE(io_base, DXR1, word & 0xffff);
}
EXPORT_SYMBOL(omap_mcbsp_xmit_word);

u32 omap_mcbsp_recv_word(unsigned int id)
{
	u32 io_base;
	u16 word_lsb, word_msb = 0;
	omap_mcbsp_word_length word_length;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return -ENODEV;
	}

	word_length = mcbsp[id].rx_word_length;
	io_base = mcbsp[id].io_base;

	wait_for_completion(&(mcbsp[id].rx_irq_completion));

	if (word_length > OMAP_MCBSP_WORD_16)
		word_msb = OMAP_MCBSP_READ(io_base, DRR2);
	word_lsb = OMAP_MCBSP_READ(io_base, DRR1);

	return (word_lsb | (word_msb << 16));
}
EXPORT_SYMBOL(omap_mcbsp_recv_word);

int omap_mcbsp_spi_master_xmit_word_poll(unsigned int id, u32 word)
{
	u32 io_base;
	omap_mcbsp_word_length tx_word_length;
	omap_mcbsp_word_length rx_word_length;
	u16 spcr2, spcr1, attempts = 0, word_lsb, word_msb = 0;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return -ENODEV;
	}

	io_base = mcbsp[id].io_base;
	tx_word_length = mcbsp[id].tx_word_length;
	rx_word_length = mcbsp[id].rx_word_length;

	if (tx_word_length != rx_word_length)
		return -EINVAL;

	/* First we wait for the transmitter to be ready */
	spcr2 = OMAP_MCBSP_READ(io_base, SPCR2);
	while (!(spcr2 & XRDY)) {
		spcr2 = OMAP_MCBSP_READ(io_base, SPCR2);
		if (attempts++ > 1000) {
			/* We must reset the transmitter */
			OMAP_MCBSP_WRITE(io_base, SPCR2, spcr2 & (~XRST));
			udelay(10);
			OMAP_MCBSP_WRITE(io_base, SPCR2, spcr2 | XRST);
			udelay(10);
			dev_err(mcbsp[id].dev, "McBSP%d transmitter not "
				"ready\n", mcbsp[id].id);
			return -EAGAIN;
		}
	}

	/* Now we can push the data */
	if (tx_word_length > OMAP_MCBSP_WORD_16)
		OMAP_MCBSP_WRITE(io_base, DXR2, word >> 16);
	OMAP_MCBSP_WRITE(io_base, DXR1, word & 0xffff);

	/* We wait for the receiver to be ready */
	spcr1 = OMAP_MCBSP_READ(io_base, SPCR1);
	while (!(spcr1 & RRDY)) {
		spcr1 = OMAP_MCBSP_READ(io_base, SPCR1);
		if (attempts++ > 1000) {
			/* We must reset the receiver */
			OMAP_MCBSP_WRITE(io_base, SPCR1, spcr1 & (~RRST));
			udelay(10);
			OMAP_MCBSP_WRITE(io_base, SPCR1, spcr1 | RRST);
			udelay(10);
			dev_err(mcbsp[id].dev, "McBSP%d receiver not "
				"ready\n", mcbsp[id].id);
			return -EAGAIN;
		}
	}

	/* Receiver is ready, let's read the dummy data */
	if (rx_word_length > OMAP_MCBSP_WORD_16)
		word_msb = OMAP_MCBSP_READ(io_base, DRR2);
	word_lsb = OMAP_MCBSP_READ(io_base, DRR1);

	return 0;
}
EXPORT_SYMBOL(omap_mcbsp_spi_master_xmit_word_poll);

int omap_mcbsp_spi_master_recv_word_poll(unsigned int id, u32 *word)
{
	u32 io_base, clock_word = 0;
	omap_mcbsp_word_length tx_word_length;
	omap_mcbsp_word_length rx_word_length;
	u16 spcr2, spcr1, attempts = 0, word_lsb, word_msb = 0;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return -ENODEV;
	}

	io_base = mcbsp[id].io_base;
	tx_word_length = mcbsp[id].tx_word_length;
	rx_word_length = mcbsp[id].rx_word_length;

	if (tx_word_length != rx_word_length)
		return -EINVAL;

	/* First we wait for the transmitter to be ready */
	spcr2 = OMAP_MCBSP_READ(io_base, SPCR2);
	while (!(spcr2 & XRDY)) {
		spcr2 = OMAP_MCBSP_READ(io_base, SPCR2);
		if (attempts++ > 1000) {
			/* We must reset the transmitter */
			OMAP_MCBSP_WRITE(io_base, SPCR2, spcr2 & (~XRST));
			udelay(10);
			OMAP_MCBSP_WRITE(io_base, SPCR2, spcr2 | XRST);
			udelay(10);
			dev_err(mcbsp[id].dev, "McBSP%d transmitter not "
				"ready\n", mcbsp[id].id);
			return -EAGAIN;
		}
	}

	/* We first need to enable the bus clock */
	if (tx_word_length > OMAP_MCBSP_WORD_16)
		OMAP_MCBSP_WRITE(io_base, DXR2, clock_word >> 16);
	OMAP_MCBSP_WRITE(io_base, DXR1, clock_word & 0xffff);

	/* We wait for the receiver to be ready */
	spcr1 = OMAP_MCBSP_READ(io_base, SPCR1);
	while (!(spcr1 & RRDY)) {
		spcr1 = OMAP_MCBSP_READ(io_base, SPCR1);
		if (attempts++ > 1000) {
			/* We must reset the receiver */
			OMAP_MCBSP_WRITE(io_base, SPCR1, spcr1 & (~RRST));
			udelay(10);
			OMAP_MCBSP_WRITE(io_base, SPCR1, spcr1 | RRST);
			udelay(10);
			dev_err(mcbsp[id].dev, "McBSP%d receiver not "
				"ready\n", mcbsp[id].id);
			return -EAGAIN;
		}
	}

	/* Receiver is ready, there is something for us */
	if (rx_word_length > OMAP_MCBSP_WORD_16)
		word_msb = OMAP_MCBSP_READ(io_base, DRR2);
	word_lsb = OMAP_MCBSP_READ(io_base, DRR1);

	word[0] = (word_lsb | (word_msb << 16));

	return 0;
}
EXPORT_SYMBOL(omap_mcbsp_spi_master_recv_word_poll);

/*
 * Simple DMA based buffer rx/tx routines.
 * Nothing fancy, just a single buffer tx/rx through DMA.
 * The DMA resources are released once the transfer is done.
 * For anything fancier, you should use your own customized DMA
 * routines and callbacks.
 */
int omap_mcbsp_xmit_buffer(unsigned int id, dma_addr_t buffer,
				unsigned int length)
{
	int dma_tx_ch;
	int src_port = 0;
	int dest_port = 0;
	int sync_dev = 0;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return -ENODEV;
	}

	if (omap_request_dma(mcbsp[id].dma_tx_sync, "McBSP TX",
				omap_mcbsp_tx_dma_callback,
				&mcbsp[id],
				&dma_tx_ch)) {
		dev_err(mcbsp[id].dev, " Unable to request DMA channel for "
				"McBSP%d TX. Trying IRQ based TX\n",
				mcbsp[id].id);
		return -EAGAIN;
	}
	mcbsp[id].dma_tx_lch = dma_tx_ch;

	dev_err(mcbsp[id].dev, "McBSP%d TX DMA on channel %d\n", mcbsp[id].id,
		dma_tx_ch);

	init_completion(&(mcbsp[id].tx_dma_completion));

	if (cpu_class_is_omap1()) {
		src_port = OMAP_DMA_PORT_TIPB;
		dest_port = OMAP_DMA_PORT_EMIFF;
	}
	if (cpu_class_is_omap2())
		sync_dev = mcbsp[id].dma_tx_sync;

	omap_set_dma_transfer_params(mcbsp[id].dma_tx_lch,
				     OMAP_DMA_DATA_TYPE_S16,
				     length >> 1, 1,
				     OMAP_DMA_SYNC_ELEMENT,
	 sync_dev, 0);

	omap_set_dma_dest_params(mcbsp[id].dma_tx_lch,
				 src_port,
				 OMAP_DMA_AMODE_CONSTANT,
				 mcbsp[id].io_base + OMAP_MCBSP_REG_DXR1,
				 0, 0);

	omap_set_dma_src_params(mcbsp[id].dma_tx_lch,
				dest_port,
				OMAP_DMA_AMODE_POST_INC,
				buffer,
				0, 0);

	omap_start_dma(mcbsp[id].dma_tx_lch);
	wait_for_completion(&(mcbsp[id].tx_dma_completion));

	return 0;
}
EXPORT_SYMBOL(omap_mcbsp_xmit_buffer);

int omap_mcbsp_recv_buffer(unsigned int id, dma_addr_t buffer,
				unsigned int length)
{
	int dma_rx_ch;
	int src_port = 0;
	int dest_port = 0;
	int sync_dev = 0;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return -ENODEV;
	}

	if (omap_request_dma(mcbsp[id].dma_rx_sync, "McBSP RX",
				omap_mcbsp_rx_dma_callback,
				&mcbsp[id],
				&dma_rx_ch)) {
		dev_err(mcbsp[id].dev, "Unable to request DMA channel for "
				"McBSP%d RX. Trying IRQ based RX\n",
				mcbsp[id].id);
		return -EAGAIN;
	}
	mcbsp[id].dma_rx_lch = dma_rx_ch;

	dev_err(mcbsp[id].dev, "McBSP%d RX DMA on channel %d\n", mcbsp[id].id,
		dma_rx_ch);

	init_completion(&(mcbsp[id].rx_dma_completion));

	if (cpu_class_is_omap1()) {
		src_port = OMAP_DMA_PORT_TIPB;
		dest_port = OMAP_DMA_PORT_EMIFF;
	}
	if (cpu_class_is_omap2())
		sync_dev = mcbsp[id].dma_rx_sync;

	omap_set_dma_transfer_params(mcbsp[id].dma_rx_lch,
					OMAP_DMA_DATA_TYPE_S16,
					length >> 1, 1,
					OMAP_DMA_SYNC_ELEMENT,
					sync_dev, 0);

	omap_set_dma_src_params(mcbsp[id].dma_rx_lch,
				src_port,
				OMAP_DMA_AMODE_CONSTANT,
				mcbsp[id].io_base + OMAP_MCBSP_REG_DRR1,
				0, 0);

	omap_set_dma_dest_params(mcbsp[id].dma_rx_lch,
					dest_port,
					OMAP_DMA_AMODE_POST_INC,
					buffer,
					0, 0);

	omap_start_dma(mcbsp[id].dma_rx_lch);
	wait_for_completion(&(mcbsp[id].rx_dma_completion));

	return 0;
}
EXPORT_SYMBOL(omap_mcbsp_recv_buffer);

/*
 * SPI wrapper.
 * Since SPI setup is much simpler than the generic McBSP one,
 * this wrapper just need an omap_mcbsp_spi_cfg structure as an input.
 * Once this is done, you can call omap_mcbsp_start().
 */
void omap_mcbsp_set_spi_mode(unsigned int id,
				const struct omap_mcbsp_spi_cfg *spi_cfg)
{
	struct omap_mcbsp_reg_cfg mcbsp_cfg;

	if (!omap_mcbsp_check_valid_id(id)) {
		printk(KERN_ERR "%s: Invalid id (%d)\n", __func__, id + 1);
		return;
	}

	memset(&mcbsp_cfg, 0, sizeof(struct omap_mcbsp_reg_cfg));

	/* SPI has only one frame */
	mcbsp_cfg.rcr1 |= (RWDLEN1(spi_cfg->word_length) | RFRLEN1(0));
	mcbsp_cfg.xcr1 |= (XWDLEN1(spi_cfg->word_length) | XFRLEN1(0));

	/* Clock stop mode */
	if (spi_cfg->clk_stp_mode == OMAP_MCBSP_CLK_STP_MODE_NO_DELAY)
		mcbsp_cfg.spcr1 |= (1 << 12);
	else
		mcbsp_cfg.spcr1 |= (3 << 11);

	/* Set clock parities */
	if (spi_cfg->rx_clock_polarity == OMAP_MCBSP_CLK_RISING)
		mcbsp_cfg.pcr0 |= CLKRP;
	else
		mcbsp_cfg.pcr0 &= ~CLKRP;

	if (spi_cfg->tx_clock_polarity == OMAP_MCBSP_CLK_RISING)
		mcbsp_cfg.pcr0 &= ~CLKXP;
	else
		mcbsp_cfg.pcr0 |= CLKXP;

	/* Set SCLKME to 0 and CLKSM to 1 */
	mcbsp_cfg.pcr0 &= ~SCLKME;
	mcbsp_cfg.srgr2 |= CLKSM;

	/* Set FSXP */
	if (spi_cfg->fsx_polarity == OMAP_MCBSP_FS_ACTIVE_HIGH)
		mcbsp_cfg.pcr0 &= ~FSXP;
	else
		mcbsp_cfg.pcr0 |= FSXP;

	if (spi_cfg->spi_mode == OMAP_MCBSP_SPI_MASTER) {
		mcbsp_cfg.pcr0 |= CLKXM;
		mcbsp_cfg.srgr1 |= CLKGDV(spi_cfg->clk_div - 1);
		mcbsp_cfg.pcr0 |= FSXM;
		mcbsp_cfg.srgr2 &= ~FSGM;
		mcbsp_cfg.xcr2 |= XDATDLY(1);
		mcbsp_cfg.rcr2 |= RDATDLY(1);
	} else {
		mcbsp_cfg.pcr0 &= ~CLKXM;
		mcbsp_cfg.srgr1 |= CLKGDV(1);
		mcbsp_cfg.pcr0 &= ~FSXM;
		mcbsp_cfg.xcr2 &= ~XDATDLY(3);
		mcbsp_cfg.rcr2 &= ~RDATDLY(3);
	}

	mcbsp_cfg.xcr2 &= ~XPHASE;
	mcbsp_cfg.rcr2 &= ~RPHASE;

	omap_mcbsp_config(id, &mcbsp_cfg);
}
EXPORT_SYMBOL(omap_mcbsp_set_spi_mode);

/*
 * McBSP1 and McBSP3 are directly mapped on 1610 and 1510.
 * 730 has only 2 McBSP, and both of them are MPU peripherals.
 */
static int __init omap_mcbsp_probe(struct platform_device *pdev)
{
	struct omap_mcbsp_platform_data *pdata = pdev->dev.platform_data;
	int id = pdev->id - 1;
	int ret = 0;

	if (!pdata) {
		dev_err(&pdev->dev, "McBSP device initialized without"
				"platform data\n");
		ret = -EINVAL;
		goto exit;
	}

	dev_dbg(&pdev->dev, "Initializing OMAP McBSP (%d).\n", pdev->id);

	if (id >= OMAP_MAX_MCBSP_COUNT) {
		dev_err(&pdev->dev, "Invalid McBSP device id (%d)\n", id);
		ret = -EINVAL;
		goto exit;
	}

	spin_lock_init(&mcbsp[id].lock);
	mcbsp[id].id = id + 1;
	mcbsp[id].free = 1;
	mcbsp[id].dma_tx_lch = -1;
	mcbsp[id].dma_rx_lch = -1;

	mcbsp[id].io_base = pdata->virt_base;
	/* Default I/O is IRQ based */
	mcbsp[id].io_type = OMAP_MCBSP_IRQ_IO;
	mcbsp[id].tx_irq = pdata->tx_irq;
	mcbsp[id].rx_irq = pdata->rx_irq;
	mcbsp[id].dma_rx_sync = pdata->dma_rx_sync;
	mcbsp[id].dma_tx_sync = pdata->dma_tx_sync;

	if (pdata->clk_name)
		mcbsp[id].clk = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(mcbsp[id].clk)) {
		mcbsp[id].free = 0;
		dev_err(&pdev->dev,
			"Invalid clock configuration for McBSP%d.\n",
			mcbsp[id].id);
		ret = -EINVAL;
		goto exit;
	}

	mcbsp[id].pdata = pdata;
	mcbsp[id].dev = &pdev->dev;
	platform_set_drvdata(pdev, &mcbsp[id]);

exit:
	return ret;
}

static int omap_mcbsp_remove(struct platform_device *pdev)
{
	struct omap_mcbsp *mcbsp = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	if (mcbsp) {

		if (mcbsp->pdata && mcbsp->pdata->ops &&
				mcbsp->pdata->ops->free)
			mcbsp->pdata->ops->free(mcbsp->id);

		clk_disable(mcbsp->clk);
		clk_put(mcbsp->clk);

		mcbsp->clk = NULL;
		mcbsp->free = 0;
		mcbsp->dev = NULL;
	}

	return 0;
}

static struct platform_driver omap_mcbsp_driver = {
	.probe		= omap_mcbsp_probe,
	.remove		= omap_mcbsp_remove,
	.driver		= {
		.name	= "omap-mcbsp",
	},
};

int __init omap_mcbsp_init(void)
{
	/* Register the McBSP driver */
	return platform_driver_register(&omap_mcbsp_driver);
}

