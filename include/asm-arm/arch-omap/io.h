/*
 * linux/include/asm-arm/arch-omap/io.h
 *
 * IO definitions for TI OMAP processors and boards
 *
 * Copied from linux/include/asm-arm/arch-sa1100/io.h
 * Copyright (C) 1997-1999 Russell King
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Modifications:
 *  06-12-1997	RMK	Created.
 *  07-04-1999	RMK	Major cleanup
 */

#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#include <asm/arch/hardware.h>

#define IO_SPACE_LIMIT 0xffffffff

/*
 * We don't actually have real ISA nor PCI buses, but there is so many
 * drivers out there that might just work if we fake them...
 */
#define __io(a)			((void __iomem *)(PCIO_BASE + (a)))
#define __mem_pci(a)		(a)

/*
 * ----------------------------------------------------------------------------
 * I/O mapping
 * ----------------------------------------------------------------------------
 */

#define PCIO_BASE	0

#if defined(CONFIG_ARCH_OMAP1)

#define IO_PHYS		0xFFFB0000
#define IO_OFFSET	0x01000000	/* Virtual IO = 0xfefb0000 */
#define IO_SIZE		0x40000
#define IO_VIRT		(IO_PHYS - IO_OFFSET)
#define IO_ADDRESS(pa)	((pa) - IO_OFFSET)
#define OMAP1_IO_ADDRESS(pa)	((pa) - IO_OFFSET)
#define io_p2v(pa)	((pa) - IO_OFFSET)
#define io_v2p(va)	((va) + IO_OFFSET)

#elif defined(CONFIG_ARCH_OMAP2)

/* We map both L3 and L4 on OMAP2 */
#define L3_24XX_PHYS	L3_24XX_BASE	/* 0x68000000 */
#define L3_24XX_VIRT	0xf8000000
#define L3_24XX_SIZE	SZ_1M		/* 44kB of 128MB used, want 1MB sect */
#define L4_24XX_PHYS	L4_24XX_BASE	/* 0x48000000 */
#define L4_24XX_VIRT	0xd8000000
#define L4_24XX_SIZE	SZ_1M		/* 1MB of 128MB used, want 1MB sect */

#ifdef CONFIG_ARCH_OMAP2430
#define L4_WK_243X_PHYS		L4_WK_243X_BASE		/* 0x49000000 */
#define L4_WK_243X_VIRT		0xd9000000
#define L4_WK_243X_SIZE		SZ_1M
#define OMAP243X_GPMC_PHYS	OMAP243X_GPMC_BASE	/* 0x49000000 */
#define OMAP243X_GPMC_VIRT	0xFE000000
#define OMAP243X_GPMC_SIZE	SZ_1M
#define OMAP243X_SDRC_PHYS	OMAP243X_SDRC_BASE
#define OMAP243X_SDRC_VIRT	0xFD000000
#define OMAP243X_SDRC_SIZE	SZ_1M
#define OMAP243X_SMS_PHYS	OMAP243X_SMS_BASE
#define OMAP243X_SMS_VIRT	0xFC000000
#define OMAP243X_SMS_SIZE	SZ_1M

#endif

#define IO_OFFSET	0x90000000
#define IO_ADDRESS(pa)	((pa) + IO_OFFSET)	/* Works for L3 and L4 */
#define OMAP2_IO_ADDRESS(pa)	((pa) + IO_OFFSET)	/* Works for L3 and L4 */
#define io_p2v(pa)	((pa) + IO_OFFSET)	/* Works for L3 and L4 */
#define io_v2p(va)	((va) - IO_OFFSET)	/* Works for L3 and L4 */

/* DSP */
#define DSP_MEM_24XX_PHYS	OMAP2420_DSP_MEM_BASE	/* 0x58000000 */
#define DSP_MEM_24XX_VIRT	0xe0000000
#define DSP_MEM_24XX_SIZE	0x28000
#define DSP_IPI_24XX_PHYS	OMAP2420_DSP_IPI_BASE	/* 0x59000000 */
#define DSP_IPI_24XX_VIRT	0xe1000000
#define DSP_IPI_24XX_SIZE	SZ_4K
#define DSP_MMU_24XX_PHYS	OMAP2420_DSP_MMU_BASE	/* 0x5a000000 */
#define DSP_MMU_24XX_VIRT	0xe2000000
#define DSP_MMU_24XX_SIZE	SZ_4K

#elif defined(CONFIG_ARCH_OMAP3)

/* We map both L3 and L4 on OMAP3 */
#define L3_34XX_PHYS		L3_34XX_BASE	/* 0x68000000 */
#define L3_34XX_VIRT		0xf8000000
#define L3_34XX_SIZE		SZ_1M   /* 44kB of 128MB used, want 1MB sect */

#define L4_34XX_PHYS		L4_34XX_BASE	/* 0x48000000 */
#define L4_34XX_VIRT		0xd8000000
#define L4_34XX_SIZE		SZ_4M   /* 1MB of 128MB used, want 1MB sect */

/*
 * Need to look at the Size 4M for L4.
 * VPOM3430 was not working for Int controller
 */

#define L4_WK_34XX_PHYS		L4_WK_34XX_BASE /* 0x48300000 */
#define L4_WK_34XX_VIRT		0xd8300000
#define L4_WK_34XX_SIZE		SZ_1M

#define L4_PER_34XX_PHYS	L4_PER_34XX_BASE /* 0x49000000 */
#define L4_PER_34XX_VIRT	0xd9000000
#define L4_PER_34XX_SIZE	SZ_1M

#define L4_EMU_34XX_PHYS	L4_EMU_34XX_BASE /* 0x54000000 */
#define L4_EMU_34XX_VIRT	0xe4000000
#define L4_EMU_34XX_SIZE	SZ_64M

#define OMAP34XX_GPMC_PHYS	OMAP34XX_GPMC_BASE /* 0x6E000000 */
#define OMAP34XX_GPMC_VIRT	0xFE000000
#define OMAP34XX_GPMC_SIZE	SZ_1M

#define OMAP343X_SMS_PHYS	OMAP343X_SMS_BASE /* 0x6C000000 */
#define OMAP343X_SMS_VIRT	0xFC000000
#define OMAP343X_SMS_SIZE	SZ_1M

#define OMAP343X_SDRC_PHYS	OMAP343X_SDRC_BASE /* 0x6D000000 */
#define OMAP343X_SDRC_VIRT	0xFD000000
#define OMAP343X_SDRC_SIZE	SZ_1M


#define IO_OFFSET		0x90000000
#define IO_ADDRESS(pa)		((pa) + IO_OFFSET)/* Works for L3 and L4 */
#define OMAP2_IO_ADDRESS(pa)	((pa) + IO_OFFSET)/* Works for L3 and L4 */
#define io_p2v(pa)		((pa) + IO_OFFSET)/* Works for L3 and L4 */
#define io_v2p(va)		((va) - IO_OFFSET)/* Works for L3 and L4 */

/* DSP */
#define DSP_MEM_34XX_PHYS	OMAP34XX_DSP_MEM_BASE	/* 0x58000000 */
#define DSP_MEM_34XX_VIRT	0xe0000000
#define DSP_MEM_34XX_SIZE	0x28000
#define DSP_IPI_34XX_PHYS	OMAP34XX_DSP_IPI_BASE	/* 0x59000000 */
#define DSP_IPI_34XX_VIRT	0xe1000000
#define DSP_IPI_34XX_SIZE	SZ_4K
#define DSP_MMU_34XX_PHYS	OMAP34XX_DSP_MMU_BASE	/* 0x5a000000 */
#define DSP_MMU_34XX_VIRT	0xe2000000
#define DSP_MMU_34XX_SIZE	SZ_4K

#endif

#ifndef __ASSEMBLER__

/*
 * Functions to access the OMAP IO region
 *
 * NOTE: - Use omap_read/write[bwl] for physical register addresses
 *	 - Use __raw_read/write[bwl]() for virtual register addresses
 *	 - Use IO_ADDRESS(phys_addr) to convert registers to virtual addresses
 *	 - DO NOT use hardcoded virtual addresses to allow changing the
 *	   IO address space again if needed
 */
#define omap_readb(a)		(*(volatile unsigned char  *)IO_ADDRESS(a))
#define omap_readw(a)		(*(volatile unsigned short *)IO_ADDRESS(a))
#define omap_readl(a)		(*(volatile unsigned int   *)IO_ADDRESS(a))

#define omap_writeb(v,a)	(*(volatile unsigned char  *)IO_ADDRESS(a) = (v))
#define omap_writew(v,a)	(*(volatile unsigned short *)IO_ADDRESS(a) = (v))
#define omap_writel(v,a)	(*(volatile unsigned int   *)IO_ADDRESS(a) = (v))

extern void omap1_map_common_io(void);
extern void omap1_init_common_hw(void);

extern void omap2_map_common_io(void);
extern void omap2_init_common_hw(void);

#endif

#endif
