/*
 * arch/powerpc/platforms/83xx/mpc834x_sys.c
 *
 * MPC834x SYS board specific routines
 *
 * Maintainer: Kumar Gala <galak@kernel.crashing.org>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/reboot.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/root_dev.h>

#include <asm/system.h>
#include <asm/atomic.h>
#include <asm/time.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/ipic.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/prom.h>
#include <asm/udbg.h>
#include <sysdev/fsl_soc.h>

#include "mpc83xx.h"

#ifndef CONFIG_PCI
unsigned long isa_io_base = 0;
unsigned long isa_mem_base = 0;
#endif

#define BCSR5_INT_USB		0x02
/* Note: This is only for PB, not for PB+PIB
 * On PB only port0 is connected using ULPI */
static int mpc834x_usb_cfg(void)
{
	unsigned long sccr, sicrl;
	void __iomem *immap;
	void __iomem *bcsr_regs = NULL;
	u8 bcsr5;
	struct device_node *np = NULL;
	int port0_is_dr = 0;

	if ((np = of_find_compatible_node(np, "usb", "fsl-usb2-dr")) != NULL)
		port0_is_dr = 1;
	if ((np = of_find_compatible_node(np, "usb", "fsl-usb2-mph")) != NULL){
		if (port0_is_dr) {
			printk(KERN_WARNING
				"There is only one USB port on PB board! \n");
			return -1;
		} else if (!port0_is_dr)
			/* No usb port enabled */
			return -1;
	}

	immap = ioremap(get_immrbase(), 0x1000);
	if (!immap)
		return -1;

	/* Configure clock */
	sccr = in_be32(immap + MPC83XX_SCCR_OFFS);
	if (port0_is_dr)
		sccr |= MPC83XX_SCCR_USB_DRCM_11;  /* 1:3 */
	else
		sccr |= MPC83XX_SCCR_USB_MPHCM_11; /* 1:3 */
	out_be32(immap + MPC83XX_SCCR_OFFS, sccr);

	/* Configure Pin */
	sicrl = in_be32(immap + MPC83XX_SICRL_OFFS);
	/* set port0 only */
	if (port0_is_dr)
		sicrl |= MPC83XX_SICRL_USB0;
	else
		sicrl &= ~(MPC83XX_SICRL_USB0);
	out_be32(immap + MPC83XX_SICRL_OFFS, sicrl);

	iounmap(immap);

	/* Map BCSR area */
	np = of_find_node_by_name(NULL, "bcsr");
	if (np != 0) {
		struct resource res;

		of_address_to_resource(np, 0, &res);
		bcsr_regs = ioremap(res.start, res.end - res.start + 1);
		of_node_put(np);
	}
	if (!bcsr_regs)
		return -1;

	/*
	 * if SYS board is plug into PIB board,
	 * force to use the PHY on SYS board
	 */
	bcsr5 = in_8(bcsr_regs + 5);
	if (!(bcsr5 & BCSR5_INT_USB))
		out_8(bcsr_regs + 5, (bcsr5 | BCSR5_INT_USB));
	iounmap(bcsr_regs);
	return 0;
}

/* ************************************************************************
 *
 * Setup the architecture
 *
 */
static void __init mpc834x_sys_setup_arch(void)
{
	struct device_node *np;

	if (ppc_md.progress)
		ppc_md.progress("mpc834x_sys_setup_arch()", 0);

	np = of_find_node_by_type(NULL, "cpu");
	if (np != 0) {
		const unsigned int *fp =
			get_property(np, "clock-frequency", NULL);
		if (fp != 0)
			loops_per_jiffy = *fp / HZ;
		else
			loops_per_jiffy = 50000000 / HZ;
		of_node_put(np);
	}

#ifdef CONFIG_PCI
	for (np = NULL; (np = of_find_node_by_type(np, "pci")) != NULL;)
		add_bridge(np);

	ppc_md.pci_exclude_device = mpc83xx_exclude_device;
#endif

	mpc834x_usb_cfg();

#ifdef  CONFIG_ROOT_NFS
	ROOT_DEV = Root_NFS;
#else
	ROOT_DEV = Root_HDA1;
#endif
}

static void __init mpc834x_sys_init_IRQ(void)
{
	struct device_node *np;

	np = of_find_node_by_type(NULL, "ipic");
	if (!np)
		return;

	ipic_init(np, 0);

	/* Initialize the default interrupt mapping priorities,
	 * in case the boot rom changed something on us.
	 */
	ipic_set_default_priority();
}

#if defined(CONFIG_I2C_MPC) && defined(CONFIG_SENSORS_DS1374)
extern ulong ds1374_get_rtc_time(void);
extern int ds1374_set_rtc_time(ulong);

static int __init mpc834x_rtc_hookup(void)
{
	struct timespec tv;

	ppc_md.get_rtc_time = ds1374_get_rtc_time;
	ppc_md.set_rtc_time = ds1374_set_rtc_time;

	tv.tv_nsec = 0;
	tv.tv_sec = (ppc_md.get_rtc_time) ();
	do_settimeofday(&tv);

	return 0;
}

late_initcall(mpc834x_rtc_hookup);
#endif

/*
 * Called very early, MMU is off, device-tree isn't unflattened
 */
static int __init mpc834x_sys_probe(void)
{
	/* We always match for now, eventually we should look at the flat
	   dev tree to ensure this is the board we are suppose to run on
	*/
	return 1;
}

define_machine(mpc834x_sys) {
	.name			= "MPC834x SYS",
	.probe			= mpc834x_sys_probe,
	.setup_arch		= mpc834x_sys_setup_arch,
	.init_IRQ		= mpc834x_sys_init_IRQ,
	.get_irq		= ipic_get_irq,
	.restart		= mpc83xx_restart,
	.time_init		= mpc83xx_time_init,
	.calibrate_decr		= generic_calibrate_decr,
	.progress		= udbg_progress,
};
