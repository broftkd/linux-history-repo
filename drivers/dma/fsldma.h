/*
 * Copyright (C) 2007 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author:
 *   Zhang Wei <wei.zhang@freescale.com>, Jul 2007
 *   Ebony Zhu <ebony.zhu@freescale.com>, May 2007
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#ifndef __DMA_FSLDMA_H
#define __DMA_FSLDMA_H

#include <linux/device.h>
#include <linux/dmapool.h>
#include <linux/dmaengine.h>

/* Define data structures needed by Freescale
 * MPC8540 and MPC8349 DMA controller.
 */
#define FSL_DMA_MR_CS		0x00000001
#define FSL_DMA_MR_CC		0x00000002
#define FSL_DMA_MR_CA		0x00000008
#define FSL_DMA_MR_EIE		0x00000040
#define FSL_DMA_MR_XFE		0x00000020
#define FSL_DMA_MR_EOLNIE	0x00000100
#define FSL_DMA_MR_EOLSIE	0x00000080
#define FSL_DMA_MR_EOSIE	0x00000200
#define FSL_DMA_MR_CDSM		0x00000010
#define FSL_DMA_MR_CTM		0x00000004
#define FSL_DMA_MR_EMP_EN	0x00200000
#define FSL_DMA_MR_EMS_EN	0x00040000
#define FSL_DMA_MR_DAHE		0x00002000
#define FSL_DMA_MR_SAHE		0x00001000

/* Special MR definition for MPC8349 */
#define FSL_DMA_MR_EOTIE	0x00000080

#define FSL_DMA_SR_CH		0x00000020
#define FSL_DMA_SR_PE		0x00000010
#define FSL_DMA_SR_CB		0x00000004
#define FSL_DMA_SR_TE		0x00000080
#define FSL_DMA_SR_EOSI		0x00000002
#define FSL_DMA_SR_EOLSI	0x00000001
#define FSL_DMA_SR_EOCDI	0x00000001
#define FSL_DMA_SR_EOLNI	0x00000008

#define FSL_DMA_SATR_SBPATMU			0x20000000
#define FSL_DMA_SATR_STRANSINT_RIO		0x00c00000
#define FSL_DMA_SATR_SREADTYPE_SNOOP_READ	0x00050000
#define FSL_DMA_SATR_SREADTYPE_BP_IORH		0x00020000
#define FSL_DMA_SATR_SREADTYPE_BP_NREAD		0x00040000
#define FSL_DMA_SATR_SREADTYPE_BP_MREAD		0x00070000

#define FSL_DMA_DATR_DBPATMU			0x20000000
#define FSL_DMA_DATR_DTRANSINT_RIO		0x00c00000
#define FSL_DMA_DATR_DWRITETYPE_SNOOP_WRITE	0x00050000
#define FSL_DMA_DATR_DWRITETYPE_BP_FLUSH	0x00010000

#define FSL_DMA_EOL		((u64)0x1)
#define FSL_DMA_SNEN		((u64)0x10)
#define FSL_DMA_EOSIE		0x8
#define FSL_DMA_NLDA_MASK	(~(u64)0x1f)

#define FSL_DMA_BCR_MAX_CNT	0x03ffffffu

#define FSL_DMA_DGSR_TE		0x80
#define FSL_DMA_DGSR_CH		0x20
#define FSL_DMA_DGSR_PE		0x10
#define FSL_DMA_DGSR_EOLNI	0x08
#define FSL_DMA_DGSR_CB		0x04
#define FSL_DMA_DGSR_EOSI	0x02
#define FSL_DMA_DGSR_EOLSI	0x01

struct fsl_dma_ld_hw {
	u64 __bitwise	src_addr;
	u64 __bitwise	dst_addr;
	u64 __bitwise	next_ln_addr;
	u32 __bitwise	count;
	u32 __bitwise	reserve;
} __attribute__((aligned(32)));

struct fsl_desc_sw {
	struct fsl_dma_ld_hw hw;
	struct list_head node;
	struct dma_async_tx_descriptor async_tx;
	struct list_head *ld;
	void *priv;
} __attribute__((aligned(32)));

struct fsl_dma_chan_regs {
	u32 __bitwise	mr;	/* 0x00 - Mode Register */
	u32 __bitwise	sr;	/* 0x04 - Status Register */
	u64 __bitwise	cdar;	/* 0x08 - Current descriptor address register */
	u64 __bitwise	sar;	/* 0x10 - Source Address Register */
	u64 __bitwise	dar;	/* 0x18 - Destination Address Register */
	u32 __bitwise	bcr;	/* 0x20 - Byte Count Register */
	u64 __bitwise	ndar;	/* 0x24 - Next Descriptor Address Register */
};

struct fsl_dma_chan;
#define FSL_DMA_MAX_CHANS_PER_DEVICE 4

struct fsl_dma_device {
	void __iomem *reg_base;	/* DGSR register base */
	struct resource reg;	/* Resource for register */
	struct device *dev;
	struct dma_device common;
	struct fsl_dma_chan *chan[FSL_DMA_MAX_CHANS_PER_DEVICE];
	u32 feature;		/* The same as DMA channels */
};

/* Define macros for fsl_dma_chan->feature property */
#define FSL_DMA_LITTLE_ENDIAN	0x00000000
#define FSL_DMA_BIG_ENDIAN	0x00000001

#define FSL_DMA_IP_MASK		0x00000ff0
#define FSL_DMA_IP_85XX		0x00000010
#define FSL_DMA_IP_83XX		0x00000020

#define FSL_DMA_CHAN_PAUSE_EXT	0x00001000
#define FSL_DMA_CHAN_START_EXT	0x00002000

struct fsl_dma_chan {
	struct fsl_dma_chan_regs __iomem *reg_base;
	dma_cookie_t completed_cookie;	/* The maximum cookie completed */
	spinlock_t desc_lock;		/* Descriptor operation lock */
	struct list_head ld_queue;	/* Link descriptors queue */
	struct dma_chan common;		/* DMA common channel */
	struct dma_pool *desc_pool;	/* Descriptors pool */
	struct device *dev;		/* Channel device */
	struct resource reg;		/* Resource for register */
	int irq;			/* Channel IRQ */
	int id;				/* Raw id of this channel */
	struct tasklet_struct tasklet;
	u32 feature;

	void (*toggle_ext_pause)(struct fsl_dma_chan *fsl_chan, int size);
	void (*toggle_ext_start)(struct fsl_dma_chan *fsl_chan, int enable);
	void (*set_src_loop_size)(struct fsl_dma_chan *fsl_chan, int size);
	void (*set_dest_loop_size)(struct fsl_dma_chan *fsl_chan, int size);
};

#define to_fsl_chan(chan) container_of(chan, struct fsl_dma_chan, common)
#define to_fsl_desc(lh) container_of(lh, struct fsl_desc_sw, node)
#define tx_to_fsl_desc(tx) container_of(tx, struct fsl_desc_sw, async_tx)

#ifndef __powerpc64__
static u64 in_be64(const u64 __iomem *addr)
{
	return ((u64)in_be32((u32 *)addr) << 32) | (in_be32((u32 *)addr + 1));
}

static void out_be64(u64 __iomem *addr, u64 val)
{
	out_be32((u32 *)addr, val >> 32);
	out_be32((u32 *)addr + 1, (u32)val);
}

/* There is no asm instructions for 64 bits reverse loads and stores */
static u64 in_le64(const u64 __iomem *addr)
{
	return ((u64)in_le32((u32 *)addr + 1) << 32) | (in_le32((u32 *)addr));
}

static void out_le64(u64 __iomem *addr, u64 val)
{
	out_le32((u32 *)addr + 1, val >> 32);
	out_le32((u32 *)addr, (u32)val);
}
#endif

#define DMA_IN(fsl_chan, addr, width)					\
		(((fsl_chan)->feature & FSL_DMA_BIG_ENDIAN) ?		\
			in_be##width(addr) : in_le##width(addr))
#define DMA_OUT(fsl_chan, addr, val, width)				\
		(((fsl_chan)->feature & FSL_DMA_BIG_ENDIAN) ?		\
			out_be##width(addr, val) : out_le##width(addr, val))

#define DMA_TO_CPU(fsl_chan, d, width)					\
		(((fsl_chan)->feature & FSL_DMA_BIG_ENDIAN) ?		\
			be##width##_to_cpu(d) :	le##width##_to_cpu(d))
#define CPU_TO_DMA(fsl_chan, c, width)					\
		(((fsl_chan)->feature & FSL_DMA_BIG_ENDIAN) ?		\
			cpu_to_be##width(c) : cpu_to_le##width(c))

#endif	/* __DMA_FSLDMA_H */
