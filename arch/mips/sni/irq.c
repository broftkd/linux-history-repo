/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1992 Linus Torvalds
 * Copyright (C) 1994 - 2000 Ralf Baechle
 */
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>

#include <asm/i8259.h>
#include <asm/io.h>
#include <asm/sni.h>

static void enable_pciasic_irq(unsigned int irq)
{
	unsigned int mask = 1 << (irq - PCIMT_IRQ_INT2);

	*(volatile u8 *) PCIMT_IRQSEL |= mask;
}

void disable_pciasic_irq(unsigned int irq)
{
	unsigned int mask = ~(1 << (irq - PCIMT_IRQ_INT2));

	*(volatile u8 *) PCIMT_IRQSEL &= mask;
}

static void end_pciasic_irq(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_pciasic_irq(irq);
}

static struct irq_chip pciasic_irq_type = {
	.typename = "ASIC-PCI",
	.ack = disable_pciasic_irq,
	.mask = disable_pciasic_irq,
	.mask_ack = disable_pciasic_irq,
	.unmask = enable_pciasic_irq,
	.end = end_pciasic_irq,
};

/*
 * hwint0 should deal with MP agent, ASIC PCI, EISA NMI and debug
 * button interrupts.  Later ...
 */
static void pciasic_hwint0(void)
{
	panic("Received int0 but no handler yet ...");
}

/* This interrupt was used for the com1 console on the first prototypes.  */
static void pciasic_hwint2(void)
{
	/* I think this shouldn't happen on production machines.  */
	panic("hwint2 and no handler yet");
}

/* hwint5 is the r4k count / compare interrupt  */
static void pciasic_hwint5(void)
{
	panic("hwint5 and no handler yet");
}

static unsigned int ls1bit8(unsigned int x)
{
	int b = 7, s;

	s = 4; if ((x & 0x0f) == 0) s = 0; b -= s; x <<= s;
	s = 2; if ((x & 0x30) == 0) s = 0; b -= s; x <<= s;
	s = 1; if ((x & 0x40) == 0) s = 0; b -= s;

	return b;
}

/*
 * hwint 1 deals with EISA and SCSI interrupts,
 *
 * The EISA_INT bit in CSITPEND is high active, all others are low active.
 */
static void pciasic_hwint1(void)
{
	u8 pend = *(volatile char *)PCIMT_CSITPEND;
	unsigned long flags;

	if (pend & IT_EISA) {
		int irq;
		/*
		 * Note: ASIC PCI's builtin interrupt achknowledge feature is
		 * broken.  Using it may result in loss of some or all i8259
		 * interupts, so don't use PCIMT_INT_ACKNOWLEDGE ...
		 */
		irq = i8259_irq();
		if (unlikely(irq < 0))
			return;

		do_IRQ(irq);
	}

	if (!(pend & IT_SCSI)) {
		flags = read_c0_status();
		clear_c0_status(ST0_IM);
		do_IRQ(PCIMT_IRQ_SCSI);
		write_c0_status(flags);
	}
}

/*
 * hwint 3 should deal with the PCI A - D interrupts,
 */
static void pciasic_hwint3(void)
{
	u8 pend = *(volatile char *)PCIMT_CSITPEND;
	int irq;

	pend &= (IT_INTA | IT_INTB | IT_INTC | IT_INTD);
	clear_c0_status(IE_IRQ3);
	irq = PCIMT_IRQ_INT2 + ls1bit8(pend);
	do_IRQ(irq);
	set_c0_status(IE_IRQ3);
}

/*
 * hwint 4 is used for only the onboard PCnet 32.
 */
static void pciasic_hwint4(void)
{
	clear_c0_status(IE_IRQ4);
	do_IRQ(PCIMT_IRQ_ETHERNET);
	set_c0_status(IE_IRQ4);
}

asmlinkage void plat_irq_dispatch(void)
{
	unsigned int pending = read_c0_status() & read_c0_cause();
	static unsigned char led_cache;

	*(volatile unsigned char *) PCIMT_CSLED = ++led_cache;

	if (pending & 0x0800)
		pciasic_hwint1();
	else if (pending & 0x4000)
		pciasic_hwint4();
	else if (pending & 0x2000)
		pciasic_hwint3();
	else if (pending & 0x1000)
		pciasic_hwint2();
	else if (pending & 0x8000)
		pciasic_hwint5();
	else if (pending & 0x0400)
		pciasic_hwint0();
}

void __init init_pciasic(void)
{
	* (volatile u8 *) PCIMT_IRQSEL =
		IT_EISA | IT_INTA | IT_INTB | IT_INTC | IT_INTD;
}

/*
 * On systems with i8259-style interrupt controllers we assume for
 * driver compatibility reasons interrupts 0 - 15 to be the i8295
 * interrupts even if the hardware uses a different interrupt numbering.
 */
void __init arch_init_irq(void)
{
	int i;

	init_i8259_irqs();			/* Integrated i8259  */
	init_pciasic();

	/* Actually we've got more interrupts to handle ...  */
	for (i = PCIMT_IRQ_INT2; i <= PCIMT_IRQ_ETHERNET; i++)
		set_irq_chip(i, &pciasic_irq_type);

	change_c0_status(ST0_IM, IE_IRQ1|IE_IRQ2|IE_IRQ3|IE_IRQ4);
}
