/*
 * Basic Virtex platform defines, included by <asm/ibm4xx.h>
 *
 * 2005-2007 (c) Secret Lab Technologies Ltd.
 * 2002-2004 (c) MontaVista Software, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifdef __KERNEL__
#ifndef __ASM_VIRTEX_H__
#define __ASM_VIRTEX_H__

#include <asm/ibm405.h>

/* Ugly, ugly, ugly! BASE_BAUD defined here to keep 8250.c happy. */
#if !defined(BASE_BAUD)
 #define BASE_BAUD		(0) /* dummy value; not used */
#endif

#ifndef __ASSEMBLY__
typedef struct board_info {
	unsigned int	 bi_memsize;		/* DRAM installed, in bytes */
	unsigned char	 bi_enetaddr[6];	/* Local Ethernet MAC address */
	unsigned int	 bi_intfreq;		/* Processor speed, in Hz */
	unsigned int	 bi_busfreq;		/* PLB Bus speed, in Hz */
	unsigned int	 bi_pci_busfreq;	/* PCI Bus speed, in Hz */
} bd_t;

/* Some 4xx parts use a different timebase frequency from the internal clock.
 * the Virtex 405 does not, so just use a macro to make tbfreq match intfreq
*/
#define bi_tbfreq bi_intfreq

extern const char* virtex_machine_name;
#define PPC4xx_MACHINE_NAME (virtex_machine_name)

#endif /* !__ASSEMBLY__ */

/* We don't need anything mapped.  Size of zero will accomplish that. */
#define PPC4xx_ONB_IO_PADDR	0u
#define PPC4xx_ONB_IO_VADDR	0u
#define PPC4xx_ONB_IO_SIZE	0u

#endif				/* __ASM_VIRTEX_H__ */
#endif				/* __KERNEL__ */
