/*
 * Copyright (C) 2000, 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/percpu.h>

#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/time.h>

#include <asm/sibyte/sb1250.h>
#include <asm/sibyte/sb1250_regs.h>
#include <asm/sibyte/sb1250_int.h>
#include <asm/sibyte/sb1250_scd.h>

#define IMR_IP2_VAL	K_INT_MAP_I0
#define IMR_IP3_VAL	K_INT_MAP_I1
#define IMR_IP4_VAL	K_INT_MAP_I2

#define SB1250_HPT_NUM		3
#define SB1250_HPT_VALUE	M_SCD_TIMER_CNT /* max value */

/*
 * The general purpose timer ticks at 1MHz independent if
 * the rest of the system
 */
static void sibyte_set_mode(enum clock_event_mode mode,
                           struct clock_event_device *evt)
{
	unsigned int cpu = smp_processor_id();
	void __iomem *cfg, *init;

	cfg = IOADDR(A_SCD_TIMER_REGISTER(cpu, R_SCD_TIMER_CFG));
	init = IOADDR(A_SCD_TIMER_REGISTER(cpu, R_SCD_TIMER_INIT));

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		__raw_writeq(0, cfg);
		__raw_writeq((V_SCD_TIMER_FREQ / HZ) - 1, init);
		__raw_writeq(M_SCD_TIMER_ENABLE | M_SCD_TIMER_MODE_CONTINUOUS,
			     cfg);
		break;

	case CLOCK_EVT_MODE_ONESHOT:
		/* Stop the timer until we actually program a shot */
	case CLOCK_EVT_MODE_SHUTDOWN:
		__raw_writeq(0, cfg);
		break;

	case CLOCK_EVT_MODE_UNUSED:	/* shuddup gcc */
	case CLOCK_EVT_MODE_RESUME:
		;
	}
}

static int sibyte_next_event(unsigned long delta, struct clock_event_device *cd)
{
	unsigned int cpu = smp_processor_id();
	void __iomem *cfg, *init;

	cfg = IOADDR(A_SCD_TIMER_REGISTER(cpu, R_SCD_TIMER_CFG));
	init = IOADDR(A_SCD_TIMER_REGISTER(cpu, R_SCD_TIMER_INIT));

	__raw_writeq(delta - 1, init);
	__raw_writeq(M_SCD_TIMER_ENABLE, cfg);

	return 0;
}

static irqreturn_t sibyte_counter_handler(int irq, void *dev_id)
{
	unsigned int cpu = smp_processor_id();
	struct clock_event_device *cd = dev_id;
	void __iomem *cfg;
	unsigned long tmode;

	if (cd->mode == CLOCK_EVT_MODE_PERIODIC)
		tmode = M_SCD_TIMER_ENABLE | M_SCD_TIMER_MODE_CONTINUOUS;
	else
		tmode = 0;

	/* ACK interrupt */
	cfg = IOADDR(A_SCD_TIMER_REGISTER(cpu, R_SCD_TIMER_CFG));
	____raw_writeq(tmode, cfg);

	cd->event_handler(cd);

	return IRQ_HANDLED;
}

static DEFINE_PER_CPU(struct clock_event_device, sibyte_hpt_clockevent);
static DEFINE_PER_CPU(struct irqaction, sibyte_hpt_irqaction);
static DEFINE_PER_CPU(char [18], sibyte_hpt_name);

void __cpuinit sb1250_clockevent_init(void)
{
	unsigned int cpu = smp_processor_id();
	unsigned int irq = K_INT_TIMER_0 + cpu;
	struct irqaction *action = &per_cpu(sibyte_hpt_irqaction, cpu);
	struct clock_event_device *cd = &per_cpu(sibyte_hpt_clockevent, cpu);
	unsigned char *name = per_cpu(sibyte_hpt_name, cpu);

	/* Only have 4 general purpose timers, and we use last one as hpt */
	BUG_ON(cpu > 2);

	sprintf(name, "sb1250-counter-%d", cpu);
	cd->name		= name;
	cd->features		= CLOCK_EVT_FEAT_PERIODIC |
				  CLOCK_EVT_FEAT_ONESHOT;
	clockevent_set_clock(cd, V_SCD_TIMER_FREQ);
	cd->max_delta_ns	= clockevent_delta2ns(0x7fffff, cd);
	cd->min_delta_ns	= clockevent_delta2ns(1, cd);
	cd->rating		= 200;
	cd->irq			= irq;
	cd->cpumask		= cpumask_of_cpu(cpu);
	cd->set_next_event	= sibyte_next_event;
	cd->set_mode		= sibyte_set_mode;
	clockevents_register_device(cd);

	sb1250_mask_irq(cpu, irq);

	/*
	 * Map the timer interrupt to IP[4] of this cpu
	 */
	__raw_writeq(IMR_IP4_VAL,
		     IOADDR(A_IMR_REGISTER(cpu, R_IMR_INTERRUPT_MAP_BASE) +
			    (irq << 3)));

	sb1250_unmask_irq(cpu, irq);

	action->handler	= sibyte_counter_handler;
	action->flags	= IRQF_DISABLED | IRQF_PERCPU;
	action->name	= name;
	action->dev_id	= cd;
	setup_irq(irq, action);
}

/*
 * The HPT is free running from SB1250_HPT_VALUE down to 0 then starts over
 * again.
 */
static cycle_t sb1250_hpt_read(void)
{
	unsigned int count;

	count = G_SCD_TIMER_CNT(__raw_readq(IOADDR(A_SCD_TIMER_REGISTER(SB1250_HPT_NUM, R_SCD_TIMER_CNT))));

	return SB1250_HPT_VALUE - count;
}

struct clocksource bcm1250_clocksource = {
	.name	= "MIPS",
	.rating	= 200,
	.read	= sb1250_hpt_read,
	.mask	= CLOCKSOURCE_MASK(23),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

void __init sb1250_clocksource_init(void)
{
	struct clocksource *cs = &bcm1250_clocksource;

	/* Setup hpt using timer #3 but do not enable irq for it */
	__raw_writeq(0,
		     IOADDR(A_SCD_TIMER_REGISTER(SB1250_HPT_NUM,
						 R_SCD_TIMER_CFG)));
	__raw_writeq(SB1250_HPT_VALUE,
		     IOADDR(A_SCD_TIMER_REGISTER(SB1250_HPT_NUM,
						 R_SCD_TIMER_INIT)));
	__raw_writeq(M_SCD_TIMER_ENABLE | M_SCD_TIMER_MODE_CONTINUOUS,
		     IOADDR(A_SCD_TIMER_REGISTER(SB1250_HPT_NUM,
						 R_SCD_TIMER_CFG)));

	clocksource_set_clock(cs, V_SCD_TIMER_FREQ);
	clocksource_register(cs);
}

void __init plat_time_init(void)
{
	sb1250_clocksource_init();
	sb1250_clockevent_init();
}
