/*
 * Header for code common to all DaVinci machines.
 *
 * Author: Kevin Hilman, MontaVista Software, Inc. <source@mvista.com>
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __ARCH_ARM_MACH_DAVINCI_COMMON_H
#define __ARCH_ARM_MACH_DAVINCI_COMMON_H

struct sys_timer;

extern struct sys_timer davinci_timer;

extern void davinci_irq_init(void);
extern void __iomem *davinci_intc_base;

/* parameters describe VBUS sourcing for host mode */
extern void setup_usb(unsigned mA, unsigned potpgt_msec);

/* parameters describe VBUS sourcing for host mode */
extern void setup_usb(unsigned mA, unsigned potpgt_msec);

struct davinci_timer_instance {
	void __iomem	*base;
	u32		bottom_irq;
	u32		top_irq;
};

struct davinci_timer_info {
	struct davinci_timer_instance	*timers;
	unsigned int			clockevent_id;
	unsigned int			clocksource_id;
};

/* SoC specific init support */
struct davinci_soc_info {
	struct map_desc			*io_desc;
	unsigned long			io_desc_num;
	u32				cpu_id;
	u32				jtag_id;
	void __iomem			*jtag_id_base;
	struct davinci_id		*ids;
	unsigned long			ids_num;
	struct davinci_clk		*cpu_clks;
	void __iomem			**psc_bases;
	unsigned long			psc_bases_num;
	void __iomem			*pinmux_base;
	const struct mux_config		*pinmux_pins;
	unsigned long			pinmux_pins_num;
	void __iomem			*intc_base;
	int				intc_type;
	u8				*intc_irq_prios;
	unsigned long			intc_irq_num;
	struct davinci_timer_info	*timer_info;
	void __iomem			*wdt_base;
	void __iomem			*gpio_base;
	unsigned			gpio_num;
	unsigned			gpio_irq;
};

extern struct davinci_soc_info davinci_soc_info;

extern void davinci_common_init(struct davinci_soc_info *soc_info);

#endif /* __ARCH_ARM_MACH_DAVINCI_COMMON_H */
