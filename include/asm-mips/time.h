/*
 * Copyright (C) 2001, 2002, MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 * Copyright (c) 2003  Maciej W. Rozycki
 *
 * include/asm-mips/time.h
 *     header file for the new style time.c file and time services.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * Please refer to Documentation/mips/time.README.
 */
#ifndef _ASM_TIME_H
#define _ASM_TIME_H

#include <linux/interrupt.h>
#include <linux/linkage.h>
#include <linux/ptrace.h>
#include <linux/rtc.h>
#include <linux/spinlock.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>

extern spinlock_t rtc_lock;

/*
 * RTC ops.  By default, they point to weak no-op RTC functions.
 *	rtc_mips_set_time - reverse the above translation and set time to RTC.
 *	rtc_mips_set_mmss - similar to rtc_set_time, but only min and sec need
 *			to be set.  Used by RTC sync-up.
 */
extern int rtc_mips_set_time(unsigned long);
extern int rtc_mips_set_mmss(unsigned long);

/*
 * Timer interrupt functions.
 * mips_timer_state is needed for high precision timer calibration.
 * mips_timer_ack may be NULL if the interrupt is self-recoverable.
 */
extern int (*mips_timer_state)(void);

/*
 * High precision timer clocksource.
 * If .read is NULL, an R4k-compatible timer setup is attempted.
 */
extern struct clocksource clocksource_mips;

/*
 * profiling and process accouting is done separately in local_timer_interrupt
 */
extern void local_timer_interrupt(int irq, void *dev_id);

/*
 * board specific routines required by time_init().
 */
struct irqaction;
extern void plat_time_init(void);

/*
 * mips_hpt_frequency - must be set if you intend to use an R4k-compatible
 * counter as a timer interrupt source; otherwise it can be set up
 * automagically with an aid of mips_timer_state.
 */
extern unsigned int mips_hpt_frequency;

/*
 * The performance counter IRQ on MIPS is a close relative to the timer IRQ
 * so it lives here.
 */
extern int (*perf_irq)(void);

/*
 * Initialize the calling CPU's compare interrupt as clockevent device
 */
#ifdef CONFIG_CEVT_R4K
extern void mips_clockevent_init(void);
extern unsigned int __weak get_c0_compare_int(void);
#else
static inline void mips_clockevent_init(void)
{
}
#endif

extern void clocksource_set_clock(struct clocksource *cs, unsigned int clock);
extern void clockevent_set_clock(struct clock_event_device *cd,
		unsigned int clock);

#endif /* _ASM_TIME_H */
