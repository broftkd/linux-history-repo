/*
 *  arch/arm/plat-omap/include/mach/clock.h
 *
 *  Copyright (C) 2004 - 2005 Nokia corporation
 *  Written by Tuukka Tikkanen <tuukka.tikkanen@elektrobit.com>
 *  Based on clocks.h by Tony Lindgren, Gordon McNutt and RidgeRun, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ARCH_ARM_OMAP_CLOCK_H
#define __ARCH_ARM_OMAP_CLOCK_H

struct module;
struct clk;
struct clockdomain;

struct clkops {
	int			(*enable)(struct clk *);
	void			(*disable)(struct clk *);
};

#if defined(CONFIG_ARCH_OMAP2) || defined(CONFIG_ARCH_OMAP3)

struct clksel_rate {
	u32			val;
	u8			div;
	u8			flags;
};

struct clksel {
	struct clk		 *parent;
	const struct clksel_rate *rates;
};

struct dpll_data {
	void __iomem		*mult_div1_reg;
	u32			mult_mask;
	u32			div1_mask;
	unsigned int		rate_tolerance;
	unsigned long		last_rounded_rate;
	u16			last_rounded_m;
	u8			last_rounded_n;
	u8			max_divider;
	u32			max_tolerance;
	u16			max_multiplier;
#  if defined(CONFIG_ARCH_OMAP3)
	u8			modes;
	void __iomem		*control_reg;
	void __iomem		*autoidle_reg;
	void __iomem		*idlest_reg;
	u32			enable_mask;
	u32			autoidle_mask;
	u32			freqsel_mask;
	u32			idlest_mask;
	u8			auto_recal_bit;
	u8			recal_en_bit;
	u8			recal_st_bit;
#  endif
};

#endif

struct clk {
	struct list_head	node;
	const struct clkops	*ops;
	const char		*name;
	int			id;
	struct clk		*parent;
	unsigned long		rate;
	__u32			flags;
	void __iomem		*enable_reg;
	void			(*recalc)(struct clk *);
	int			(*set_rate)(struct clk *, unsigned long);
	long			(*round_rate)(struct clk *, unsigned long);
	void			(*init)(struct clk *);
	__u8			enable_bit;
	__s8			usecount;
#if defined(CONFIG_ARCH_OMAP2) || defined(CONFIG_ARCH_OMAP3)
	u8			fixed_div;
	void __iomem		*clksel_reg;
	u32			clksel_mask;
	const struct clksel	*clksel;
	struct dpll_data	*dpll_data;
	const char		*clkdm_name;
	struct clockdomain	*clkdm;
#else
	__u8			rate_offset;
	__u8			src_offset;
#endif
#if defined(CONFIG_PM_DEBUG) && defined(CONFIG_DEBUG_FS)
	struct dentry		*dent;	/* For visible tree hierarchy */
#endif
};

struct cpufreq_frequency_table;

struct clk_functions {
	int		(*clk_enable)(struct clk *clk);
	void		(*clk_disable)(struct clk *clk);
	long		(*clk_round_rate)(struct clk *clk, unsigned long rate);
	int		(*clk_set_rate)(struct clk *clk, unsigned long rate);
	int		(*clk_set_parent)(struct clk *clk, struct clk *parent);
	void		(*clk_allow_idle)(struct clk *clk);
	void		(*clk_deny_idle)(struct clk *clk);
	void		(*clk_disable_unused)(struct clk *clk);
#ifdef CONFIG_CPU_FREQ
	void		(*clk_init_cpufreq_table)(struct cpufreq_frequency_table **);
#endif
};

extern unsigned int mpurate;

extern int clk_init(struct clk_functions *custom_clocks);
extern int clk_register(struct clk *clk);
extern void clk_unregister(struct clk *clk);
extern void propagate_rate(struct clk *clk);
extern void recalculate_root_clocks(void);
extern void followparent_recalc(struct clk *clk);
extern int clk_get_usecount(struct clk *clk);
extern void clk_enable_init_clocks(void);
#ifdef CONFIG_CPU_FREQ
extern void clk_init_cpufreq_table(struct cpufreq_frequency_table **table);
#endif

extern const struct clkops clkops_null;

/* Clock flags */
/* bit 0 is free */
#define RATE_FIXED		(1 << 1)	/* Fixed clock rate */
#define RATE_PROPAGATES		(1 << 2)	/* Program children too */
/* bits 3-4 are free */
#define ENABLE_REG_32BIT	(1 << 5)	/* Use 32-bit access */
#define VIRTUAL_IO_ADDRESS	(1 << 6)	/* Clock in virtual address */
#define CLOCK_IDLE_CONTROL	(1 << 7)
#define CLOCK_NO_IDLE_PARENT	(1 << 8)
#define DELAYED_APP		(1 << 9)	/* Delay application of clock */
#define CONFIG_PARTICIPANT	(1 << 10)	/* Fundamental clock */
#define ENABLE_ON_INIT		(1 << 11)	/* Enable upon framework init */
#define INVERT_ENABLE           (1 << 12)       /* 0 enables, 1 disables */
/* bits 13-31 are currently free */

/* Clksel_rate flags */
#define DEFAULT_RATE		(1 << 0)
#define RATE_IN_242X		(1 << 1)
#define RATE_IN_243X		(1 << 2)
#define RATE_IN_343X		(1 << 3)	/* rates common to all 343X */
#define RATE_IN_3430ES2		(1 << 4)	/* 3430ES2 rates only */

#define RATE_IN_24XX		(RATE_IN_242X | RATE_IN_243X)


/* CM_CLKSEL2_PLL.CORE_CLK_SRC options (24XX) */
#define CORE_CLK_SRC_32K		0
#define CORE_CLK_SRC_DPLL		1
#define CORE_CLK_SRC_DPLL_X2		2

#endif
