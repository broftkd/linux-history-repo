/*
 * OMAP3-specific clock framework functions
 *
 * Copyright (C) 2007-2008 Texas Instruments, Inc.
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Written by Paul Walmsley
 * Testing and integration fixes by Jouni Högander
 *
 * Parts of this code are based on code written by
 * Richard Woodruff, Tony Lindgren, Tuukka Tikkanen, Karthik Dasu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#undef DEBUG

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/limits.h>
#include <linux/bitops.h>

#include <mach/clock.h>
#include <mach/sram.h>
#include <asm/div64.h>
#include <asm/clkdev.h>

#include "memory.h"
#include "clock.h"
#include "prm.h"
#include "prm-regbits-34xx.h"
#include "cm.h"
#include "cm-regbits-34xx.h"

static const struct clkops clkops_noncore_dpll_ops;

#include "clock34xx.h"

struct omap_clk {
	u32		cpu;
	struct clk_lookup lk;
};

#define CLK(dev, con, ck, cp) 		\
	{				\
		 .cpu = cp,		\
		.lk = {			\
			.dev_id = dev,	\
			.con_id = con,	\
			.clk = ck,	\
		},			\
	}

#define CK_343X		(1 << 0)
#define CK_3430ES1	(1 << 1)
#define CK_3430ES2	(1 << 2)

static struct omap_clk omap34xx_clks[] = {
	CLK(NULL,	"omap_32k_fck",	&omap_32k_fck,	CK_343X),
	CLK(NULL,	"virt_12m_ck",	&virt_12m_ck,	CK_343X),
	CLK(NULL,	"virt_13m_ck",	&virt_13m_ck,	CK_343X),
	CLK(NULL,	"virt_16_8m_ck", &virt_16_8m_ck, CK_3430ES2),
	CLK(NULL,	"virt_19_2m_ck", &virt_19_2m_ck, CK_343X),
	CLK(NULL,	"virt_26m_ck",	&virt_26m_ck,	CK_343X),
	CLK(NULL,	"virt_38_4m_ck", &virt_38_4m_ck, CK_343X),
	CLK(NULL,	"osc_sys_ck",	&osc_sys_ck,	CK_343X),
	CLK(NULL,	"sys_ck",	&sys_ck,	CK_343X),
	CLK(NULL,	"sys_altclk",	&sys_altclk,	CK_343X),
	CLK(NULL,	"mcbsp_clks",	&mcbsp_clks,	CK_343X),
	CLK(NULL,	"sys_clkout1",	&sys_clkout1,	CK_343X),
	CLK(NULL,	"dpll1_ck",	&dpll1_ck,	CK_343X),
	CLK(NULL,	"dpll1_x2_ck",	&dpll1_x2_ck,	CK_343X),
	CLK(NULL,	"dpll1_x2m2_ck", &dpll1_x2m2_ck, CK_343X),
	CLK(NULL,	"dpll2_ck",	&dpll2_ck,	CK_343X),
	CLK(NULL,	"dpll2_m2_ck",	&dpll2_m2_ck,	CK_343X),
	CLK(NULL,	"dpll3_ck",	&dpll3_ck,	CK_343X),
	CLK(NULL,	"core_ck",	&core_ck,	CK_343X),
	CLK(NULL,	"dpll3_x2_ck",	&dpll3_x2_ck,	CK_343X),
	CLK(NULL,	"dpll3_m2_ck",	&dpll3_m2_ck,	CK_343X),
	CLK(NULL,	"dpll3_m2x2_ck", &dpll3_m2x2_ck, CK_343X),
	CLK(NULL,	"dpll3_m3_ck",	&dpll3_m3_ck,	CK_343X),
	CLK(NULL,	"dpll3_m3x2_ck", &dpll3_m3x2_ck, CK_343X),
	CLK(NULL,	"emu_core_alwon_ck", &emu_core_alwon_ck, CK_343X),
	CLK(NULL,	"dpll4_ck",	&dpll4_ck,	CK_343X),
	CLK(NULL,	"dpll4_x2_ck",	&dpll4_x2_ck,	CK_343X),
	CLK(NULL,	"omap_96m_alwon_fck", &omap_96m_alwon_fck, CK_343X),
	CLK(NULL,	"omap_96m_fck",	&omap_96m_fck,	CK_343X),
	CLK(NULL,	"cm_96m_fck",	&cm_96m_fck,	CK_343X),
	CLK(NULL,	"virt_omap_54m_fck", &virt_omap_54m_fck, CK_343X),
	CLK(NULL,	"omap_54m_fck",	&omap_54m_fck,	CK_343X),
	CLK(NULL,	"omap_48m_fck",	&omap_48m_fck,	CK_343X),
	CLK(NULL,	"omap_12m_fck",	&omap_12m_fck,	CK_343X),
	CLK(NULL,	"dpll4_m2_ck",	&dpll4_m2_ck,	CK_343X),
	CLK(NULL,	"dpll4_m2x2_ck", &dpll4_m2x2_ck, CK_343X),
	CLK(NULL,	"dpll4_m3_ck",	&dpll4_m3_ck,	CK_343X),
	CLK(NULL,	"dpll4_m3x2_ck", &dpll4_m3x2_ck, CK_343X),
	CLK(NULL,	"dpll4_m4_ck",	&dpll4_m4_ck,	CK_343X),
	CLK(NULL,	"dpll4_m4x2_ck", &dpll4_m4x2_ck, CK_343X),
	CLK(NULL,	"dpll4_m5_ck",	&dpll4_m5_ck,	CK_343X),
	CLK(NULL,	"dpll4_m5x2_ck", &dpll4_m5x2_ck, CK_343X),
	CLK(NULL,	"dpll4_m6_ck",	&dpll4_m6_ck,	CK_343X),
	CLK(NULL,	"dpll4_m6x2_ck", &dpll4_m6x2_ck, CK_343X),
	CLK(NULL,	"emu_per_alwon_ck", &emu_per_alwon_ck, CK_343X),
	CLK(NULL,	"dpll5_ck",	&dpll5_ck,	CK_3430ES2),
	CLK(NULL,	"dpll5_m2_ck",	&dpll5_m2_ck,	CK_3430ES2),
	CLK(NULL,	"omap_120m_fck", &omap_120m_fck, CK_3430ES2),
	CLK(NULL,	"clkout2_src_ck", &clkout2_src_ck, CK_343X),
	CLK(NULL,	"sys_clkout2",	&sys_clkout2,	CK_343X),
	CLK(NULL,	"corex2_fck",	&corex2_fck,	CK_343X),
	CLK(NULL,	"dpll1_fck",	&dpll1_fck,	CK_343X),
	CLK(NULL,	"mpu_ck",	&mpu_ck,	CK_343X),
	CLK(NULL,	"arm_fck",	&arm_fck,	CK_343X),
	CLK(NULL,	"emu_mpu_alwon_ck", &emu_mpu_alwon_ck, CK_343X),
	CLK(NULL,	"dpll2_fck",	&dpll2_fck,	CK_343X),
	CLK(NULL,	"iva2_ck",	&iva2_ck,	CK_343X),
	CLK(NULL,	"l3_ick",	&l3_ick,	CK_343X),
	CLK(NULL,	"l4_ick",	&l4_ick,	CK_343X),
	CLK(NULL,	"rm_ick",	&rm_ick,	CK_343X),
	CLK(NULL,	"gfx_l3_ck",	&gfx_l3_ck,	CK_3430ES1),
	CLK(NULL,	"gfx_l3_fck",	&gfx_l3_fck,	CK_3430ES1),
	CLK(NULL,	"gfx_l3_ick",	&gfx_l3_ick,	CK_3430ES1),
	CLK(NULL,	"gfx_cg1_ck",	&gfx_cg1_ck,	CK_3430ES1),
	CLK(NULL,	"gfx_cg2_ck",	&gfx_cg2_ck,	CK_3430ES1),
	CLK(NULL,	"sgx_fck",	&sgx_fck,	CK_3430ES2),
	CLK(NULL,	"sgx_ick",	&sgx_ick,	CK_3430ES2),
	CLK(NULL,	"d2d_26m_fck",	&d2d_26m_fck,	CK_3430ES1),
	CLK(NULL,	"gpt10_fck",	&gpt10_fck,	CK_343X),
	CLK(NULL,	"gpt11_fck",	&gpt11_fck,	CK_343X),
	CLK(NULL,	"cpefuse_fck",	&cpefuse_fck,	CK_3430ES2),
	CLK(NULL,	"ts_fck",	&ts_fck,	CK_3430ES2),
	CLK(NULL,	"usbtll_fck",	&usbtll_fck,	CK_3430ES2),
	CLK(NULL,	"core_96m_fck",	&core_96m_fck,	CK_343X),
	CLK("mmci-omap-hs.2",	"mmchs_fck",	&mmchs3_fck,	CK_3430ES2),
	CLK("mmci-omap-hs.1",	"mmchs_fck",	&mmchs2_fck,	CK_343X),
	CLK(NULL,	"mspro_fck",	&mspro_fck,	CK_343X),
	CLK("mmci-omap-hs.0",	"mmchs_fck",	&mmchs1_fck,	CK_343X),
	CLK("i2c_omap.3", "i2c_fck",	&i2c3_fck,	CK_343X),
	CLK("i2c_omap.2", "i2c_fck",	&i2c2_fck,	CK_343X),
	CLK("i2c_omap.1", "i2c_fck",	&i2c1_fck,	CK_343X),
	CLK("omap-mcbsp.5", "mcbsp_fck", &mcbsp5_fck,	CK_343X),
	CLK("omap-mcbsp.1", "mcbsp_fck", &mcbsp1_fck,	CK_343X),
	CLK(NULL,	"core_48m_fck",	&core_48m_fck,	CK_343X),
	CLK("omap2_mcspi.4", "mcspi_fck", &mcspi4_fck,	CK_343X),
	CLK("omap2_mcspi.3", "mcspi_fck", &mcspi3_fck,	CK_343X),
	CLK("omap2_mcspi.2", "mcspi_fck", &mcspi2_fck,	CK_343X),
	CLK("omap2_mcspi.1", "mcspi_fck", &mcspi1_fck,	CK_343X),
	CLK(NULL,	"uart2_fck",	&uart2_fck,	CK_343X),
	CLK(NULL,	"uart1_fck",	&uart1_fck,	CK_343X),
	CLK(NULL,	"fshostusb_fck", &fshostusb_fck, CK_3430ES1),
	CLK(NULL,	"core_12m_fck",	&core_12m_fck,	CK_343X),
	CLK(NULL,	"hdq_fck",	&hdq_fck,	CK_343X),
	CLK(NULL,	"ssi_ssr_fck",	&ssi_ssr_fck,	CK_343X),
	CLK(NULL,	"ssi_sst_fck",	&ssi_sst_fck,	CK_343X),
	CLK(NULL,	"core_l3_ick",	&core_l3_ick,	CK_343X),
	CLK(NULL,	"hsotgusb_ick",	&hsotgusb_ick,	CK_343X),
	CLK(NULL,	"sdrc_ick",	&sdrc_ick,	CK_343X),
	CLK(NULL,	"gpmc_fck",	&gpmc_fck,	CK_343X),
	CLK(NULL,	"security_l3_ick", &security_l3_ick, CK_343X),
	CLK(NULL,	"pka_ick",	&pka_ick,	CK_343X),
	CLK(NULL,	"core_l4_ick",	&core_l4_ick,	CK_343X),
	CLK(NULL,	"usbtll_ick",	&usbtll_ick,	CK_3430ES2),
	CLK("mmci-omap-hs.2",	"mmchs_ick",	&mmchs3_ick,	CK_3430ES2),
	CLK(NULL,	"icr_ick",	&icr_ick,	CK_343X),
	CLK(NULL,	"aes2_ick",	&aes2_ick,	CK_343X),
	CLK(NULL,	"sha12_ick",	&sha12_ick,	CK_343X),
	CLK(NULL,	"des2_ick",	&des2_ick,	CK_343X),
	CLK("mmci-omap-hs.1",	"mmchs_ick",	&mmchs2_ick,	CK_343X),
	CLK("mmci-omap-hs.0",	"mmchs_ick",	&mmchs1_ick,	CK_343X),
	CLK(NULL,	"mspro_ick",	&mspro_ick,	CK_343X),
	CLK(NULL,	"hdq_ick",	&hdq_ick,	CK_343X),
	CLK("omap2_mcspi.4", "mcspi_ick", &mcspi4_ick,	CK_343X),
	CLK("omap2_mcspi.3", "mcspi_ick", &mcspi3_ick,	CK_343X),
	CLK("omap2_mcspi.2", "mcspi_ick", &mcspi2_ick,	CK_343X),
	CLK("omap2_mcspi.1", "mcspi_ick", &mcspi1_ick,	CK_343X),
	CLK("i2c_omap.3", "i2c_ick",	&i2c3_ick,	CK_343X),
	CLK("i2c_omap.2", "i2c_ick",	&i2c2_ick,	CK_343X),
	CLK("i2c_omap.1", "i2c_ick",	&i2c1_ick,	CK_343X),
	CLK(NULL,	"uart2_ick",	&uart2_ick,	CK_343X),
	CLK(NULL,	"uart1_ick",	&uart1_ick,	CK_343X),
	CLK(NULL,	"gpt11_ick",	&gpt11_ick,	CK_343X),
	CLK(NULL,	"gpt10_ick",	&gpt10_ick,	CK_343X),
	CLK("omap-mcbsp.5", "mcbsp_ick", &mcbsp5_ick,	CK_343X),
	CLK("omap-mcbsp.1", "mcbsp_ick", &mcbsp1_ick,	CK_343X),
	CLK(NULL,	"fac_ick",	&fac_ick,	CK_3430ES1),
	CLK(NULL,	"mailboxes_ick", &mailboxes_ick, CK_343X),
	CLK(NULL,	"omapctrl_ick",	&omapctrl_ick,	CK_343X),
	CLK(NULL,	"ssi_l4_ick",	&ssi_l4_ick,	CK_343X),
	CLK(NULL,	"ssi_ick",	&ssi_ick,	CK_343X),
	CLK(NULL,	"usb_l4_ick",	&usb_l4_ick,	CK_3430ES1),
	CLK(NULL,	"security_l4_ick2", &security_l4_ick2, CK_343X),
	CLK(NULL,	"aes1_ick",	&aes1_ick,	CK_343X),
	CLK(NULL,	"rng_ick",	&rng_ick,	CK_343X),
	CLK(NULL,	"sha11_ick",	&sha11_ick,	CK_343X),
	CLK(NULL,	"des1_ick",	&des1_ick,	CK_343X),
	CLK(NULL,	"dss1_alwon_fck", &dss1_alwon_fck, CK_343X),
	CLK(NULL,	"dss_tv_fck",	&dss_tv_fck,	CK_343X),
	CLK(NULL,	"dss_96m_fck",	&dss_96m_fck,	CK_343X),
	CLK(NULL,	"dss2_alwon_fck", &dss2_alwon_fck, CK_343X),
	CLK(NULL,	"dss_ick",	&dss_ick,	CK_343X),
	CLK(NULL,	"cam_mclk",	&cam_mclk,	CK_343X),
	CLK(NULL,	"cam_ick",	&cam_ick,	CK_343X),
	CLK(NULL,	"usbhost_120m_fck", &usbhost_120m_fck, CK_3430ES2),
	CLK(NULL,	"usbhost_48m_fck", &usbhost_48m_fck, CK_3430ES2),
	CLK(NULL,	"usbhost_ick",	&usbhost_ick,	CK_3430ES2),
	CLK(NULL,	"usbhost_sar_fck", &usbhost_sar_fck, CK_3430ES2),
	CLK(NULL,	"usim_fck",	&usim_fck,	CK_3430ES2),
	CLK(NULL,	"gpt1_fck",	&gpt1_fck,	CK_343X),
	CLK(NULL,	"wkup_32k_fck",	&wkup_32k_fck,	CK_343X),
	CLK(NULL,	"gpio1_dbck",	&gpio1_dbck,	CK_343X),
	CLK("omap_wdt",	"fck",		&wdt2_fck,	CK_343X),
	CLK(NULL,	"wkup_l4_ick",	&wkup_l4_ick,	CK_343X),
	CLK(NULL,	"usim_ick",	&usim_ick,	CK_3430ES2),
	CLK("omap_wdt",	"ick",		&wdt2_ick,	CK_343X),
	CLK(NULL,	"wdt1_ick",	&wdt1_ick,	CK_343X),
	CLK(NULL,	"gpio1_ick",	&gpio1_ick,	CK_343X),
	CLK(NULL,	"omap_32ksync_ick", &omap_32ksync_ick, CK_343X),
	CLK(NULL,	"gpt12_ick",	&gpt12_ick,	CK_343X),
	CLK(NULL,	"gpt1_ick",	&gpt1_ick,	CK_343X),
	CLK(NULL,	"per_96m_fck",	&per_96m_fck,	CK_343X),
	CLK(NULL,	"per_48m_fck",	&per_48m_fck,	CK_343X),
	CLK(NULL,	"uart3_fck",	&uart3_fck,	CK_343X),
	CLK(NULL,	"gpt2_fck",	&gpt2_fck,	CK_343X),
	CLK(NULL,	"gpt3_fck",	&gpt3_fck,	CK_343X),
	CLK(NULL,	"gpt4_fck",	&gpt4_fck,	CK_343X),
	CLK(NULL,	"gpt5_fck",	&gpt5_fck,	CK_343X),
	CLK(NULL,	"gpt6_fck",	&gpt6_fck,	CK_343X),
	CLK(NULL,	"gpt7_fck",	&gpt7_fck,	CK_343X),
	CLK(NULL,	"gpt8_fck",	&gpt8_fck,	CK_343X),
	CLK(NULL,	"gpt9_fck",	&gpt9_fck,	CK_343X),
	CLK(NULL,	"per_32k_alwon_fck", &per_32k_alwon_fck, CK_343X),
	CLK(NULL,	"gpio6_dbck",	&gpio6_dbck,	CK_343X),
	CLK(NULL,	"gpio5_dbck",	&gpio5_dbck,	CK_343X),
	CLK(NULL,	"gpio4_dbck",	&gpio4_dbck,	CK_343X),
	CLK(NULL,	"gpio3_dbck",	&gpio3_dbck,	CK_343X),
	CLK(NULL,	"gpio2_dbck",	&gpio2_dbck,	CK_343X),
	CLK(NULL,	"wdt3_fck",	&wdt3_fck,	CK_343X),
	CLK(NULL,	"per_l4_ick",	&per_l4_ick,	CK_343X),
	CLK(NULL,	"gpio6_ick",	&gpio6_ick,	CK_343X),
	CLK(NULL,	"gpio5_ick",	&gpio5_ick,	CK_343X),
	CLK(NULL,	"gpio4_ick",	&gpio4_ick,	CK_343X),
	CLK(NULL,	"gpio3_ick",	&gpio3_ick,	CK_343X),
	CLK(NULL,	"gpio2_ick",	&gpio2_ick,	CK_343X),
	CLK(NULL,	"wdt3_ick",	&wdt3_ick,	CK_343X),
	CLK(NULL,	"uart3_ick",	&uart3_ick,	CK_343X),
	CLK(NULL,	"gpt9_ick",	&gpt9_ick,	CK_343X),
	CLK(NULL,	"gpt8_ick",	&gpt8_ick,	CK_343X),
	CLK(NULL,	"gpt7_ick",	&gpt7_ick,	CK_343X),
	CLK(NULL,	"gpt6_ick",	&gpt6_ick,	CK_343X),
	CLK(NULL,	"gpt5_ick",	&gpt5_ick,	CK_343X),
	CLK(NULL,	"gpt4_ick",	&gpt4_ick,	CK_343X),
	CLK(NULL,	"gpt3_ick",	&gpt3_ick,	CK_343X),
	CLK(NULL,	"gpt2_ick",	&gpt2_ick,	CK_343X),
	CLK("omap-mcbsp.2", "mcbsp_ick", &mcbsp2_ick,	CK_343X),
	CLK("omap-mcbsp.3", "mcbsp_ick", &mcbsp3_ick,	CK_343X),
	CLK("omap-mcbsp.4", "mcbsp_ick", &mcbsp4_ick,	CK_343X),
	CLK("omap-mcbsp.2", "mcbsp_fck", &mcbsp2_fck,	CK_343X),
	CLK("omap-mcbsp.3", "mcbsp_fck", &mcbsp3_fck,	CK_343X),
	CLK("omap-mcbsp.4", "mcbsp_fck", &mcbsp4_fck,	CK_343X),
	CLK(NULL,	"emu_src_ck",	&emu_src_ck,	CK_343X),
	CLK(NULL,	"pclk_fck",	&pclk_fck,	CK_343X),
	CLK(NULL,	"pclkx2_fck",	&pclkx2_fck,	CK_343X),
	CLK(NULL,	"atclk_fck",	&atclk_fck,	CK_343X),
	CLK(NULL,	"traceclk_src_fck", &traceclk_src_fck, CK_343X),
	CLK(NULL,	"traceclk_fck",	&traceclk_fck,	CK_343X),
	CLK(NULL,	"sr1_fck",	&sr1_fck,	CK_343X),
	CLK(NULL,	"sr2_fck",	&sr2_fck,	CK_343X),
	CLK(NULL,	"sr_l4_ick",	&sr_l4_ick,	CK_343X),
	CLK(NULL,	"secure_32k_fck", &secure_32k_fck, CK_343X),
	CLK(NULL,	"gpt12_fck",	&gpt12_fck,	CK_343X),
	CLK(NULL,	"wdt1_fck",	&wdt1_fck,	CK_343X),
};

/* CM_AUTOIDLE_PLL*.AUTO_* bit values */
#define DPLL_AUTOIDLE_DISABLE			0x0
#define DPLL_AUTOIDLE_LOW_POWER_STOP		0x1

#define MAX_DPLL_WAIT_TRIES		1000000

/**
 * omap3_dpll_recalc - recalculate DPLL rate
 * @clk: DPLL struct clk
 *
 * Recalculate and propagate the DPLL rate.
 */
static void omap3_dpll_recalc(struct clk *clk)
{
	clk->rate = omap2_get_dpll_rate(clk);
}

/* _omap3_dpll_write_clken - write clken_bits arg to a DPLL's enable bits */
static void _omap3_dpll_write_clken(struct clk *clk, u8 clken_bits)
{
	const struct dpll_data *dd;
	u32 v;

	dd = clk->dpll_data;

	v = __raw_readl(dd->control_reg);
	v &= ~dd->enable_mask;
	v |= clken_bits << __ffs(dd->enable_mask);
	__raw_writel(v, dd->control_reg);
}

/* _omap3_wait_dpll_status: wait for a DPLL to enter a specific state */
static int _omap3_wait_dpll_status(struct clk *clk, u8 state)
{
	const struct dpll_data *dd;
	int i = 0;
	int ret = -EINVAL;
	u32 idlest_mask;

	dd = clk->dpll_data;

	state <<= dd->idlest_bit;
	idlest_mask = 1 << dd->idlest_bit;

	while (((__raw_readl(dd->idlest_reg) & idlest_mask) != state) &&
	       i < MAX_DPLL_WAIT_TRIES) {
		i++;
		udelay(1);
	}

	if (i == MAX_DPLL_WAIT_TRIES) {
		printk(KERN_ERR "clock: %s failed transition to '%s'\n",
		       clk->name, (state) ? "locked" : "bypassed");
	} else {
		pr_debug("clock: %s transition to '%s' in %d loops\n",
			 clk->name, (state) ? "locked" : "bypassed", i);

		ret = 0;
	}

	return ret;
}

/* Non-CORE DPLL (e.g., DPLLs that do not control SDRC) clock functions */

/*
 * _omap3_noncore_dpll_lock - instruct a DPLL to lock and wait for readiness
 * @clk: pointer to a DPLL struct clk
 *
 * Instructs a non-CORE DPLL to lock.  Waits for the DPLL to report
 * readiness before returning.  Will save and restore the DPLL's
 * autoidle state across the enable, per the CDP code.  If the DPLL
 * locked successfully, return 0; if the DPLL did not lock in the time
 * allotted, or DPLL3 was passed in, return -EINVAL.
 */
static int _omap3_noncore_dpll_lock(struct clk *clk)
{
	u8 ai;
	int r;

	if (clk == &dpll3_ck)
		return -EINVAL;

	pr_debug("clock: locking DPLL %s\n", clk->name);

	ai = omap3_dpll_autoidle_read(clk);

	_omap3_dpll_write_clken(clk, DPLL_LOCKED);

	if (ai) {
		/*
		 * If no downstream clocks are enabled, CM_IDLEST bit
		 * may never become active, so don't wait for DPLL to lock.
		 */
		r = 0;
		omap3_dpll_allow_idle(clk);
	} else {
		r = _omap3_wait_dpll_status(clk, 1);
		omap3_dpll_deny_idle(clk);
	};

	return r;
}

/*
 * omap3_noncore_dpll_bypass - instruct a DPLL to bypass and wait for readiness
 * @clk: pointer to a DPLL struct clk
 *
 * Instructs a non-CORE DPLL to enter low-power bypass mode.  In
 * bypass mode, the DPLL's rate is set equal to its parent clock's
 * rate.  Waits for the DPLL to report readiness before returning.
 * Will save and restore the DPLL's autoidle state across the enable,
 * per the CDP code.  If the DPLL entered bypass mode successfully,
 * return 0; if the DPLL did not enter bypass in the time allotted, or
 * DPLL3 was passed in, or the DPLL does not support low-power bypass,
 * return -EINVAL.
 */
static int _omap3_noncore_dpll_bypass(struct clk *clk)
{
	int r;
	u8 ai;

	if (clk == &dpll3_ck)
		return -EINVAL;

	if (!(clk->dpll_data->modes & (1 << DPLL_LOW_POWER_BYPASS)))
		return -EINVAL;

	pr_debug("clock: configuring DPLL %s for low-power bypass\n",
		 clk->name);

	ai = omap3_dpll_autoidle_read(clk);

	_omap3_dpll_write_clken(clk, DPLL_LOW_POWER_BYPASS);

	r = _omap3_wait_dpll_status(clk, 0);

	if (ai)
		omap3_dpll_allow_idle(clk);
	else
		omap3_dpll_deny_idle(clk);

	return r;
}

/*
 * _omap3_noncore_dpll_stop - instruct a DPLL to stop
 * @clk: pointer to a DPLL struct clk
 *
 * Instructs a non-CORE DPLL to enter low-power stop. Will save and
 * restore the DPLL's autoidle state across the stop, per the CDP
 * code.  If DPLL3 was passed in, or the DPLL does not support
 * low-power stop, return -EINVAL; otherwise, return 0.
 */
static int _omap3_noncore_dpll_stop(struct clk *clk)
{
	u8 ai;

	if (clk == &dpll3_ck)
		return -EINVAL;

	if (!(clk->dpll_data->modes & (1 << DPLL_LOW_POWER_STOP)))
		return -EINVAL;

	pr_debug("clock: stopping DPLL %s\n", clk->name);

	ai = omap3_dpll_autoidle_read(clk);

	_omap3_dpll_write_clken(clk, DPLL_LOW_POWER_STOP);

	if (ai)
		omap3_dpll_allow_idle(clk);
	else
		omap3_dpll_deny_idle(clk);

	return 0;
}

/**
 * omap3_noncore_dpll_enable - instruct a DPLL to enter bypass or lock mode
 * @clk: pointer to a DPLL struct clk
 *
 * Instructs a non-CORE DPLL to enable, e.g., to enter bypass or lock.
 * The choice of modes depends on the DPLL's programmed rate: if it is
 * the same as the DPLL's parent clock, it will enter bypass;
 * otherwise, it will enter lock.  This code will wait for the DPLL to
 * indicate readiness before returning, unless the DPLL takes too long
 * to enter the target state.  Intended to be used as the struct clk's
 * enable function.  If DPLL3 was passed in, or the DPLL does not
 * support low-power stop, or if the DPLL took too long to enter
 * bypass or lock, return -EINVAL; otherwise, return 0.
 */
static int omap3_noncore_dpll_enable(struct clk *clk)
{
	int r;

	if (clk == &dpll3_ck)
		return -EINVAL;

	if (clk->parent->rate == clk_get_rate(clk))
		r = _omap3_noncore_dpll_bypass(clk);
	else
		r = _omap3_noncore_dpll_lock(clk);

	return r;
}

/**
 * omap3_noncore_dpll_enable - instruct a DPLL to enter bypass or lock mode
 * @clk: pointer to a DPLL struct clk
 *
 * Instructs a non-CORE DPLL to enable, e.g., to enter bypass or lock.
 * The choice of modes depends on the DPLL's programmed rate: if it is
 * the same as the DPLL's parent clock, it will enter bypass;
 * otherwise, it will enter lock.  This code will wait for the DPLL to
 * indicate readiness before returning, unless the DPLL takes too long
 * to enter the target state.  Intended to be used as the struct clk's
 * enable function.  If DPLL3 was passed in, or the DPLL does not
 * support low-power stop, or if the DPLL took too long to enter
 * bypass or lock, return -EINVAL; otherwise, return 0.
 */
static void omap3_noncore_dpll_disable(struct clk *clk)
{
	if (clk == &dpll3_ck)
		return;

	_omap3_noncore_dpll_stop(clk);
}

static const struct clkops clkops_noncore_dpll_ops = {
	.enable		= &omap3_noncore_dpll_enable,
	.disable	= &omap3_noncore_dpll_disable,
};

/**
 * omap3_dpll_autoidle_read - read a DPLL's autoidle bits
 * @clk: struct clk * of the DPLL to read
 *
 * Return the DPLL's autoidle bits, shifted down to bit 0.  Returns
 * -EINVAL if passed a null pointer or if the struct clk does not
 * appear to refer to a DPLL.
 */
static u32 omap3_dpll_autoidle_read(struct clk *clk)
{
	const struct dpll_data *dd;
	u32 v;

	if (!clk || !clk->dpll_data)
		return -EINVAL;

	dd = clk->dpll_data;

	v = __raw_readl(dd->autoidle_reg);
	v &= dd->autoidle_mask;
	v >>= __ffs(dd->autoidle_mask);

	return v;
}

/**
 * omap3_dpll_allow_idle - enable DPLL autoidle bits
 * @clk: struct clk * of the DPLL to operate on
 *
 * Enable DPLL automatic idle control.  This automatic idle mode
 * switching takes effect only when the DPLL is locked, at least on
 * OMAP3430.  The DPLL will enter low-power stop when its downstream
 * clocks are gated.  No return value.
 */
static void omap3_dpll_allow_idle(struct clk *clk)
{
	const struct dpll_data *dd;
	u32 v;

	if (!clk || !clk->dpll_data)
		return;

	dd = clk->dpll_data;

	/*
	 * REVISIT: CORE DPLL can optionally enter low-power bypass
	 * by writing 0x5 instead of 0x1.  Add some mechanism to
	 * optionally enter this mode.
	 */
	v = __raw_readl(dd->autoidle_reg);
	v &= ~dd->autoidle_mask;
	v |= DPLL_AUTOIDLE_LOW_POWER_STOP << __ffs(dd->autoidle_mask);
	__raw_writel(v, dd->autoidle_reg);
}

/**
 * omap3_dpll_deny_idle - prevent DPLL from automatically idling
 * @clk: struct clk * of the DPLL to operate on
 *
 * Disable DPLL automatic idle control.  No return value.
 */
static void omap3_dpll_deny_idle(struct clk *clk)
{
	const struct dpll_data *dd;
	u32 v;

	if (!clk || !clk->dpll_data)
		return;

	dd = clk->dpll_data;

	v = __raw_readl(dd->autoidle_reg);
	v &= ~dd->autoidle_mask;
	v |= DPLL_AUTOIDLE_DISABLE << __ffs(dd->autoidle_mask);
	__raw_writel(v, dd->autoidle_reg);
}

/* Clock control for DPLL outputs */

/**
 * omap3_clkoutx2_recalc - recalculate DPLL X2 output virtual clock rate
 * @clk: DPLL output struct clk
 *
 * Using parent clock DPLL data, look up DPLL state.  If locked, set our
 * rate to the dpll_clk * 2; otherwise, just use dpll_clk.
 */
static void omap3_clkoutx2_recalc(struct clk *clk)
{
	const struct dpll_data *dd;
	u32 v;
	struct clk *pclk;

	/* Walk up the parents of clk, looking for a DPLL */
	pclk = clk->parent;
	while (pclk && !pclk->dpll_data)
		pclk = pclk->parent;

	/* clk does not have a DPLL as a parent? */
	WARN_ON(!pclk);

	dd = pclk->dpll_data;

	WARN_ON(!dd->control_reg || !dd->enable_mask);

	v = __raw_readl(dd->control_reg) & dd->enable_mask;
	v >>= __ffs(dd->enable_mask);
	if (v != DPLL_LOCKED)
		clk->rate = clk->parent->rate;
	else
		clk->rate = clk->parent->rate * 2;
}

/* Common clock code */

/*
 * As it is structured now, this will prevent an OMAP2/3 multiboot
 * kernel from compiling.  This will need further attention.
 */
#if defined(CONFIG_ARCH_OMAP3)

static struct clk_functions omap2_clk_functions = {
	.clk_enable		= omap2_clk_enable,
	.clk_disable		= omap2_clk_disable,
	.clk_round_rate		= omap2_clk_round_rate,
	.clk_set_rate		= omap2_clk_set_rate,
	.clk_set_parent		= omap2_clk_set_parent,
	.clk_disable_unused	= omap2_clk_disable_unused,
};

/*
 * Set clocks for bypass mode for reboot to work.
 */
void omap2_clk_prepare_for_reboot(void)
{
	/* REVISIT: Not ready for 343x */
#if 0
	u32 rate;

	if (vclk == NULL || sclk == NULL)
		return;

	rate = clk_get_rate(sclk);
	clk_set_rate(vclk, rate);
#endif
}

/* REVISIT: Move this init stuff out into clock.c */

/*
 * Switch the MPU rate if specified on cmdline.
 * We cannot do this early until cmdline is parsed.
 */
static int __init omap2_clk_arch_init(void)
{
	if (!mpurate)
		return -EINVAL;

	/* REVISIT: not yet ready for 343x */
#if 0
	if (omap2_select_table_rate(&virt_prcm_set, mpurate))
		printk(KERN_ERR "Could not find matching MPU rate\n");
#endif

	recalculate_root_clocks();

	printk(KERN_INFO "Switched to new clocking rate (Crystal/DPLL3/MPU): "
	       "%ld.%01ld/%ld/%ld MHz\n",
	       (osc_sys_ck.rate / 1000000), (osc_sys_ck.rate / 100000) % 10,
	       (core_ck.rate / 1000000), (dpll1_fck.rate / 1000000)) ;

	return 0;
}
arch_initcall(omap2_clk_arch_init);

int __init omap2_clk_init(void)
{
	/* struct prcm_config *prcm; */
	struct omap_clk *c;
	/* u32 clkrate; */
	u32 cpu_clkflg;

	if (cpu_is_omap34xx()) {
		cpu_mask = RATE_IN_343X;
		cpu_clkflg = CK_343X;

		/*
		 * Update this if there are further clock changes between ES2
		 * and production parts
		 */
		if (omap_rev() == OMAP3430_REV_ES1_0) {
			/* No 3430ES1-only rates exist, so no RATE_IN_3430ES1 */
			cpu_clkflg |= CK_3430ES1;
		} else {
			cpu_mask |= RATE_IN_3430ES2;
			cpu_clkflg |= CK_3430ES2;
		}
	}

	clk_init(&omap2_clk_functions);

	for (c = omap34xx_clks; c < omap34xx_clks + ARRAY_SIZE(omap34xx_clks); c++)
		if (c->cpu & cpu_clkflg) {
			clkdev_add(&c->lk);
			clk_register(c->lk.clk);
			omap2_init_clk_clkdm(c->lk.clk);
		}

	/* REVISIT: Not yet ready for OMAP3 */
#if 0
	/* Check the MPU rate set by bootloader */
	clkrate = omap2_get_dpll_rate_24xx(&dpll_ck);
	for (prcm = rate_table; prcm->mpu_speed; prcm++) {
		if (!(prcm->flags & cpu_mask))
			continue;
		if (prcm->xtal_speed != sys_ck.rate)
			continue;
		if (prcm->dpll_speed <= clkrate)
			 break;
	}
	curr_prcm_set = prcm;
#endif

	recalculate_root_clocks();

	printk(KERN_INFO "Clocking rate (Crystal/DPLL/ARM core): "
	       "%ld.%01ld/%ld/%ld MHz\n",
	       (osc_sys_ck.rate / 1000000), (osc_sys_ck.rate / 100000) % 10,
	       (core_ck.rate / 1000000), (arm_fck.rate / 1000000));

	/*
	 * Only enable those clocks we will need, let the drivers
	 * enable other clocks as necessary
	 */
	clk_enable_init_clocks();

	/* Avoid sleeping during omap2_clk_prepare_for_reboot() */
	/* REVISIT: not yet ready for 343x */
#if 0
	vclk = clk_get(NULL, "virt_prcm_set");
	sclk = clk_get(NULL, "sys_ck");
#endif
	return 0;
}

#endif
