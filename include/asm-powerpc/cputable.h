#ifndef __ASM_POWERPC_CPUTABLE_H
#define __ASM_POWERPC_CPUTABLE_H

#include <asm/asm-compat.h>

#define PPC_FEATURE_32			0x80000000
#define PPC_FEATURE_64			0x40000000
#define PPC_FEATURE_601_INSTR		0x20000000
#define PPC_FEATURE_HAS_ALTIVEC		0x10000000
#define PPC_FEATURE_HAS_FPU		0x08000000
#define PPC_FEATURE_HAS_MMU		0x04000000
#define PPC_FEATURE_HAS_4xxMAC		0x02000000
#define PPC_FEATURE_UNIFIED_CACHE	0x01000000
#define PPC_FEATURE_HAS_SPE		0x00800000
#define PPC_FEATURE_HAS_EFP_SINGLE	0x00400000
#define PPC_FEATURE_HAS_EFP_DOUBLE	0x00200000
#define PPC_FEATURE_NO_TB		0x00100000
#define PPC_FEATURE_POWER4		0x00080000
#define PPC_FEATURE_POWER5		0x00040000
#define PPC_FEATURE_POWER5_PLUS		0x00020000
#define PPC_FEATURE_CELL		0x00010000
#define PPC_FEATURE_BOOKE		0x00008000
#define PPC_FEATURE_SMT			0x00004000
#define PPC_FEATURE_ICACHE_SNOOP	0x00002000
#define PPC_FEATURE_ARCH_2_05		0x00001000
#define PPC_FEATURE_PA6T		0x00000800
#define PPC_FEATURE_HAS_DFP		0x00000400
#define PPC_FEATURE_POWER6_EXT		0x00000200

#define PPC_FEATURE_TRUE_LE		0x00000002
#define PPC_FEATURE_PPC_LE		0x00000001

#ifdef __KERNEL__
#ifndef __ASSEMBLY__

/* This structure can grow, it's real size is used by head.S code
 * via the mkdefs mechanism.
 */
struct cpu_spec;

typedef	void (*cpu_setup_t)(unsigned long offset, struct cpu_spec* spec);
typedef	void (*cpu_restore_t)(void);

enum powerpc_oprofile_type {
	PPC_OPROFILE_INVALID = 0,
	PPC_OPROFILE_RS64 = 1,
	PPC_OPROFILE_POWER4 = 2,
	PPC_OPROFILE_G4 = 3,
	PPC_OPROFILE_BOOKE = 4,
	PPC_OPROFILE_CELL = 5,
};

enum powerpc_pmc_type {
	PPC_PMC_DEFAULT = 0,
	PPC_PMC_IBM = 1,
	PPC_PMC_PA6T = 2,
};

struct cpu_spec {
	/* CPU is matched via (PVR & pvr_mask) == pvr_value */
	unsigned int	pvr_mask;
	unsigned int	pvr_value;

	char		*cpu_name;
	unsigned long	cpu_features;		/* Kernel features */
	unsigned int	cpu_user_features;	/* Userland features */

	/* cache line sizes */
	unsigned int	icache_bsize;
	unsigned int	dcache_bsize;

	/* number of performance monitor counters */
	unsigned int	num_pmcs;
	enum powerpc_pmc_type pmc_type;

	/* this is called to initialize various CPU bits like L1 cache,
	 * BHT, SPD, etc... from head.S before branching to identify_machine
	 */
	cpu_setup_t	cpu_setup;
	/* Used to restore cpu setup on secondary processors and at resume */
	cpu_restore_t	cpu_restore;

	/* Used by oprofile userspace to select the right counters */
	char		*oprofile_cpu_type;

	/* Processor specific oprofile operations */
	enum powerpc_oprofile_type oprofile_type;

	/* Bit locations inside the mmcra change */
	unsigned long	oprofile_mmcra_sihv;
	unsigned long	oprofile_mmcra_sipr;

	/* Bits to clear during an oprofile exception */
	unsigned long	oprofile_mmcra_clear;

	/* Name of processor class, for the ELF AT_PLATFORM entry */
	char		*platform;
};

extern struct cpu_spec		*cur_cpu_spec;

extern unsigned int __start___ftr_fixup, __stop___ftr_fixup;

extern struct cpu_spec *identify_cpu(unsigned long offset, unsigned int pvr);
extern void do_feature_fixups(unsigned long value, void *fixup_start,
			      void *fixup_end);

#endif /* __ASSEMBLY__ */

/* CPU kernel features */

/* Retain the 32b definitions all use bottom half of word */
#define CPU_FTR_SPLIT_ID_CACHE		ASM_CONST(0x0000000000000001)
#define CPU_FTR_L2CR			ASM_CONST(0x0000000000000002)
#define CPU_FTR_SPEC7450		ASM_CONST(0x0000000000000004)
#define CPU_FTR_ALTIVEC			ASM_CONST(0x0000000000000008)
#define CPU_FTR_TAU			ASM_CONST(0x0000000000000010)
#define CPU_FTR_CAN_DOZE		ASM_CONST(0x0000000000000020)
#define CPU_FTR_USE_TB			ASM_CONST(0x0000000000000040)
#define CPU_FTR_604_PERF_MON		ASM_CONST(0x0000000000000080)
#define CPU_FTR_601			ASM_CONST(0x0000000000000100)
#define CPU_FTR_HPTE_TABLE		ASM_CONST(0x0000000000000200)
#define CPU_FTR_CAN_NAP			ASM_CONST(0x0000000000000400)
#define CPU_FTR_L3CR			ASM_CONST(0x0000000000000800)
#define CPU_FTR_L3_DISABLE_NAP		ASM_CONST(0x0000000000001000)
#define CPU_FTR_NAP_DISABLE_L2_PR	ASM_CONST(0x0000000000002000)
#define CPU_FTR_DUAL_PLL_750FX		ASM_CONST(0x0000000000004000)
#define CPU_FTR_NO_DPM			ASM_CONST(0x0000000000008000)
#define CPU_FTR_HAS_HIGH_BATS		ASM_CONST(0x0000000000010000)
#define CPU_FTR_NEED_COHERENT		ASM_CONST(0x0000000000020000)
#define CPU_FTR_NO_BTIC			ASM_CONST(0x0000000000040000)
#define CPU_FTR_BIG_PHYS		ASM_CONST(0x0000000000080000)
#define CPU_FTR_NODSISRALIGN		ASM_CONST(0x0000000000100000)
#define CPU_FTR_PPC_LE			ASM_CONST(0x0000000000200000)
#define CPU_FTR_REAL_LE			ASM_CONST(0x0000000000400000)
#define CPU_FTR_FPU_UNAVAILABLE		ASM_CONST(0x0000000000800000)

/*
 * Add the 64-bit processor unique features in the top half of the word;
 * on 32-bit, make the names available but defined to be 0.
 */
#ifdef __powerpc64__
#define LONG_ASM_CONST(x)		ASM_CONST(x)
#else
#define LONG_ASM_CONST(x)		0
#endif

#define CPU_FTR_SLB			LONG_ASM_CONST(0x0000000100000000)
#define CPU_FTR_16M_PAGE		LONG_ASM_CONST(0x0000000200000000)
#define CPU_FTR_TLBIEL			LONG_ASM_CONST(0x0000000400000000)
#define CPU_FTR_NOEXECUTE		LONG_ASM_CONST(0x0000000800000000)
#define CPU_FTR_IABR			LONG_ASM_CONST(0x0000002000000000)
#define CPU_FTR_MMCRA			LONG_ASM_CONST(0x0000004000000000)
#define CPU_FTR_CTRL			LONG_ASM_CONST(0x0000008000000000)
#define CPU_FTR_SMT			LONG_ASM_CONST(0x0000010000000000)
#define CPU_FTR_COHERENT_ICACHE		LONG_ASM_CONST(0x0000020000000000)
#define CPU_FTR_LOCKLESS_TLBIE		LONG_ASM_CONST(0x0000040000000000)
#define CPU_FTR_CI_LARGE_PAGE		LONG_ASM_CONST(0x0000100000000000)
#define CPU_FTR_PAUSE_ZERO		LONG_ASM_CONST(0x0000200000000000)
#define CPU_FTR_PURR			LONG_ASM_CONST(0x0000400000000000)
#define CPU_FTR_CELL_TB_BUG		LONG_ASM_CONST(0x0000800000000000)
#define CPU_FTR_SPURR			LONG_ASM_CONST(0x0001000000000000)
#define CPU_FTR_DSCR			LONG_ASM_CONST(0x0002000000000000)

#ifndef __ASSEMBLY__

#define CPU_FTR_PPCAS_ARCH_V2	(CPU_FTR_SLB | \
				 CPU_FTR_TLBIEL | CPU_FTR_NOEXECUTE | \
				 CPU_FTR_NODSISRALIGN | CPU_FTR_16M_PAGE)

/* We only set the altivec features if the kernel was compiled with altivec
 * support
 */
#ifdef CONFIG_ALTIVEC
#define CPU_FTR_ALTIVEC_COMP	CPU_FTR_ALTIVEC
#define PPC_FEATURE_HAS_ALTIVEC_COMP PPC_FEATURE_HAS_ALTIVEC
#else
#define CPU_FTR_ALTIVEC_COMP	0
#define PPC_FEATURE_HAS_ALTIVEC_COMP    0
#endif

/* We need to mark all pages as being coherent if we're SMP or we
 * have a 74[45]x and an MPC107 host bridge. Also 83xx requires
 * it for PCI "streaming/prefetch" to work properly.
 */
#if defined(CONFIG_SMP) || defined(CONFIG_MPC10X_BRIDGE) \
	|| defined(CONFIG_PPC_83xx)
#define CPU_FTR_COMMON                  CPU_FTR_NEED_COHERENT
#else
#define CPU_FTR_COMMON                  0
#endif

/* The powersave features NAP & DOZE seems to confuse BDI when
   debugging. So if a BDI is used, disable theses
 */
#ifndef CONFIG_BDI_SWITCH
#define CPU_FTR_MAYBE_CAN_DOZE	CPU_FTR_CAN_DOZE
#define CPU_FTR_MAYBE_CAN_NAP	CPU_FTR_CAN_NAP
#else
#define CPU_FTR_MAYBE_CAN_DOZE	0
#define CPU_FTR_MAYBE_CAN_NAP	0
#endif

#define CLASSIC_PPC (!defined(CONFIG_8xx) && !defined(CONFIG_4xx) && \
		     !defined(CONFIG_POWER3) && !defined(CONFIG_POWER4) && \
		     !defined(CONFIG_BOOKE))

#define CPU_FTRS_PPC601	(CPU_FTR_COMMON | CPU_FTR_601 | CPU_FTR_HPTE_TABLE)
#define CPU_FTRS_603	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_MAYBE_CAN_DOZE | CPU_FTR_USE_TB | \
	    CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_PPC_LE)
#define CPU_FTRS_604	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_USE_TB | CPU_FTR_604_PERF_MON | CPU_FTR_HPTE_TABLE | \
	    CPU_FTR_PPC_LE)
#define CPU_FTRS_740_NOTAU	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_MAYBE_CAN_DOZE | CPU_FTR_USE_TB | CPU_FTR_L2CR | \
	    CPU_FTR_HPTE_TABLE | CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_PPC_LE)
#define CPU_FTRS_740	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_MAYBE_CAN_DOZE | CPU_FTR_USE_TB | CPU_FTR_L2CR | \
	    CPU_FTR_TAU | CPU_FTR_HPTE_TABLE | CPU_FTR_MAYBE_CAN_NAP | \
	    CPU_FTR_PPC_LE)
#define CPU_FTRS_750	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_MAYBE_CAN_DOZE | CPU_FTR_USE_TB | CPU_FTR_L2CR | \
	    CPU_FTR_TAU | CPU_FTR_HPTE_TABLE | CPU_FTR_MAYBE_CAN_NAP | \
	    CPU_FTR_PPC_LE)
#define CPU_FTRS_750CL	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_MAYBE_CAN_DOZE | CPU_FTR_USE_TB | CPU_FTR_L2CR | \
	    CPU_FTR_TAU | CPU_FTR_HPTE_TABLE | CPU_FTR_MAYBE_CAN_NAP | \
	    CPU_FTR_HAS_HIGH_BATS | CPU_FTR_PPC_LE)
#define CPU_FTRS_750FX1	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_MAYBE_CAN_DOZE | CPU_FTR_USE_TB | CPU_FTR_L2CR | \
	    CPU_FTR_TAU | CPU_FTR_HPTE_TABLE | CPU_FTR_MAYBE_CAN_NAP | \
	    CPU_FTR_DUAL_PLL_750FX | CPU_FTR_NO_DPM | CPU_FTR_PPC_LE)
#define CPU_FTRS_750FX2	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_MAYBE_CAN_DOZE | CPU_FTR_USE_TB | CPU_FTR_L2CR | \
	    CPU_FTR_TAU | CPU_FTR_HPTE_TABLE | CPU_FTR_MAYBE_CAN_NAP | \
	    CPU_FTR_NO_DPM | CPU_FTR_PPC_LE)
#define CPU_FTRS_750FX	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_MAYBE_CAN_DOZE | CPU_FTR_USE_TB | CPU_FTR_L2CR | \
	    CPU_FTR_TAU | CPU_FTR_HPTE_TABLE | CPU_FTR_MAYBE_CAN_NAP | \
	    CPU_FTR_DUAL_PLL_750FX | CPU_FTR_HAS_HIGH_BATS | CPU_FTR_PPC_LE)
#define CPU_FTRS_750GX	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_MAYBE_CAN_DOZE | CPU_FTR_USE_TB | CPU_FTR_L2CR | \
	    CPU_FTR_TAU | CPU_FTR_HPTE_TABLE | CPU_FTR_MAYBE_CAN_NAP | \
	    CPU_FTR_DUAL_PLL_750FX | CPU_FTR_HAS_HIGH_BATS | CPU_FTR_PPC_LE)
#define CPU_FTRS_7400_NOTAU	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_MAYBE_CAN_DOZE | CPU_FTR_USE_TB | CPU_FTR_L2CR | \
	    CPU_FTR_ALTIVEC_COMP | CPU_FTR_HPTE_TABLE | \
	    CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_PPC_LE)
#define CPU_FTRS_7400	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_MAYBE_CAN_DOZE | CPU_FTR_USE_TB | CPU_FTR_L2CR | \
	    CPU_FTR_TAU | CPU_FTR_ALTIVEC_COMP | CPU_FTR_HPTE_TABLE | \
	    CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_PPC_LE)
#define CPU_FTRS_7450_20	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_USE_TB | CPU_FTR_L2CR | CPU_FTR_ALTIVEC_COMP | \
	    CPU_FTR_L3CR | CPU_FTR_HPTE_TABLE | CPU_FTR_SPEC7450 | \
	    CPU_FTR_NEED_COHERENT | CPU_FTR_PPC_LE)
#define CPU_FTRS_7450_21	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_USE_TB | \
	    CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_L2CR | CPU_FTR_ALTIVEC_COMP | \
	    CPU_FTR_L3CR | CPU_FTR_HPTE_TABLE | CPU_FTR_SPEC7450 | \
	    CPU_FTR_NAP_DISABLE_L2_PR | CPU_FTR_L3_DISABLE_NAP | \
	    CPU_FTR_NEED_COHERENT | CPU_FTR_PPC_LE)
#define CPU_FTRS_7450_23	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_USE_TB | \
	    CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_L2CR | CPU_FTR_ALTIVEC_COMP | \
	    CPU_FTR_L3CR | CPU_FTR_HPTE_TABLE | CPU_FTR_SPEC7450 | \
	    CPU_FTR_NAP_DISABLE_L2_PR | CPU_FTR_NEED_COHERENT | CPU_FTR_PPC_LE)
#define CPU_FTRS_7455_1	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_USE_TB | \
	    CPU_FTR_L2CR | CPU_FTR_ALTIVEC_COMP | CPU_FTR_L3CR | \
	    CPU_FTR_HPTE_TABLE | CPU_FTR_SPEC7450 | CPU_FTR_HAS_HIGH_BATS | \
	    CPU_FTR_NEED_COHERENT | CPU_FTR_PPC_LE)
#define CPU_FTRS_7455_20	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_USE_TB | \
	    CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_L2CR | CPU_FTR_ALTIVEC_COMP | \
	    CPU_FTR_L3CR | CPU_FTR_HPTE_TABLE | CPU_FTR_SPEC7450 | \
	    CPU_FTR_NAP_DISABLE_L2_PR | CPU_FTR_L3_DISABLE_NAP | \
	    CPU_FTR_NEED_COHERENT | CPU_FTR_HAS_HIGH_BATS | CPU_FTR_PPC_LE)
#define CPU_FTRS_7455	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_USE_TB | \
	    CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_L2CR | CPU_FTR_ALTIVEC_COMP | \
	    CPU_FTR_L3CR | CPU_FTR_HPTE_TABLE | CPU_FTR_SPEC7450 | \
	    CPU_FTR_NAP_DISABLE_L2_PR | CPU_FTR_HAS_HIGH_BATS | \
	    CPU_FTR_NEED_COHERENT | CPU_FTR_PPC_LE)
#define CPU_FTRS_7447_10	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_USE_TB | \
	    CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_L2CR | CPU_FTR_ALTIVEC_COMP | \
	    CPU_FTR_L3CR | CPU_FTR_HPTE_TABLE | CPU_FTR_SPEC7450 | \
	    CPU_FTR_NAP_DISABLE_L2_PR | CPU_FTR_HAS_HIGH_BATS | \
	    CPU_FTR_NEED_COHERENT | CPU_FTR_NO_BTIC | CPU_FTR_PPC_LE)
#define CPU_FTRS_7447	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_USE_TB | \
	    CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_L2CR | CPU_FTR_ALTIVEC_COMP | \
	    CPU_FTR_L3CR | CPU_FTR_HPTE_TABLE | CPU_FTR_SPEC7450 | \
	    CPU_FTR_NAP_DISABLE_L2_PR | CPU_FTR_HAS_HIGH_BATS | \
	    CPU_FTR_NEED_COHERENT | CPU_FTR_PPC_LE)
#define CPU_FTRS_7447A	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_USE_TB | \
	    CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_L2CR | CPU_FTR_ALTIVEC_COMP | \
	    CPU_FTR_HPTE_TABLE | CPU_FTR_SPEC7450 | \
	    CPU_FTR_NAP_DISABLE_L2_PR | CPU_FTR_HAS_HIGH_BATS | \
	    CPU_FTR_NEED_COHERENT | CPU_FTR_PPC_LE)
#define CPU_FTRS_82XX	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_MAYBE_CAN_DOZE | CPU_FTR_USE_TB)
#define CPU_FTRS_G2_LE	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_MAYBE_CAN_DOZE | \
	    CPU_FTR_USE_TB | CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_HAS_HIGH_BATS)
#define CPU_FTRS_E300	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_MAYBE_CAN_DOZE | \
	    CPU_FTR_USE_TB | CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_HAS_HIGH_BATS | \
	    CPU_FTR_COMMON)
#define CPU_FTRS_E300C2	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_MAYBE_CAN_DOZE | \
	    CPU_FTR_USE_TB | CPU_FTR_MAYBE_CAN_NAP | CPU_FTR_HAS_HIGH_BATS | \
	    CPU_FTR_COMMON | CPU_FTR_FPU_UNAVAILABLE)
#define CPU_FTRS_CLASSIC32	(CPU_FTR_COMMON | CPU_FTR_SPLIT_ID_CACHE | \
	    CPU_FTR_USE_TB | CPU_FTR_HPTE_TABLE)
#define CPU_FTRS_8XX	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB)
#define CPU_FTRS_40X	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_NODSISRALIGN)
#define CPU_FTRS_44X	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_NODSISRALIGN)
#define CPU_FTRS_E200	(CPU_FTR_USE_TB | CPU_FTR_NODSISRALIGN)
#define CPU_FTRS_E500	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_NODSISRALIGN)
#define CPU_FTRS_E500_2	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_BIG_PHYS | CPU_FTR_NODSISRALIGN)
#define CPU_FTRS_GENERIC_32	(CPU_FTR_COMMON | CPU_FTR_NODSISRALIGN)

/* 64-bit CPUs */
#define CPU_FTRS_POWER3	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_HPTE_TABLE | CPU_FTR_IABR | CPU_FTR_PPC_LE)
#define CPU_FTRS_RS64	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_HPTE_TABLE | CPU_FTR_IABR | \
	    CPU_FTR_MMCRA | CPU_FTR_CTRL)
#define CPU_FTRS_POWER4	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_HPTE_TABLE | CPU_FTR_PPCAS_ARCH_V2 | CPU_FTR_CTRL | \
	    CPU_FTR_MMCRA)
#define CPU_FTRS_PPC970	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_HPTE_TABLE | CPU_FTR_PPCAS_ARCH_V2 | CPU_FTR_CTRL | \
	    CPU_FTR_ALTIVEC_COMP | CPU_FTR_CAN_NAP | CPU_FTR_MMCRA)
#define CPU_FTRS_POWER5	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_HPTE_TABLE | CPU_FTR_PPCAS_ARCH_V2 | CPU_FTR_CTRL | \
	    CPU_FTR_MMCRA | CPU_FTR_SMT | \
	    CPU_FTR_COHERENT_ICACHE | CPU_FTR_LOCKLESS_TLBIE | \
	    CPU_FTR_PURR)
#define CPU_FTRS_POWER6 (CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_HPTE_TABLE | CPU_FTR_PPCAS_ARCH_V2 | CPU_FTR_CTRL | \
	    CPU_FTR_MMCRA | CPU_FTR_SMT | \
	    CPU_FTR_COHERENT_ICACHE | CPU_FTR_LOCKLESS_TLBIE | \
	    CPU_FTR_PURR | CPU_FTR_SPURR | CPU_FTR_REAL_LE | \
	    CPU_FTR_DSCR)
#define CPU_FTRS_CELL	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_HPTE_TABLE | CPU_FTR_PPCAS_ARCH_V2 | CPU_FTR_CTRL | \
	    CPU_FTR_ALTIVEC_COMP | CPU_FTR_MMCRA | CPU_FTR_SMT | \
	    CPU_FTR_PAUSE_ZERO | CPU_FTR_CI_LARGE_PAGE | CPU_FTR_CELL_TB_BUG)
#define CPU_FTRS_PA6T (CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_HPTE_TABLE | CPU_FTR_PPCAS_ARCH_V2 | \
	    CPU_FTR_ALTIVEC_COMP | CPU_FTR_CI_LARGE_PAGE | \
	    CPU_FTR_PURR | CPU_FTR_REAL_LE)
#define CPU_FTRS_COMPATIBLE	(CPU_FTR_SPLIT_ID_CACHE | CPU_FTR_USE_TB | \
	    CPU_FTR_HPTE_TABLE | CPU_FTR_PPCAS_ARCH_V2)

#ifdef __powerpc64__
#define CPU_FTRS_POSSIBLE	\
	    (CPU_FTRS_POWER3 | CPU_FTRS_RS64 | CPU_FTRS_POWER4 |	\
	    CPU_FTRS_PPC970 | CPU_FTRS_POWER5 | CPU_FTRS_POWER6 |	\
	    CPU_FTRS_CELL | CPU_FTRS_PA6T)
#else
enum {
	CPU_FTRS_POSSIBLE =
#if CLASSIC_PPC
	    CPU_FTRS_PPC601 | CPU_FTRS_603 | CPU_FTRS_604 | CPU_FTRS_740_NOTAU |
	    CPU_FTRS_740 | CPU_FTRS_750 | CPU_FTRS_750FX1 |
	    CPU_FTRS_750FX2 | CPU_FTRS_750FX | CPU_FTRS_750GX |
	    CPU_FTRS_7400_NOTAU | CPU_FTRS_7400 | CPU_FTRS_7450_20 |
	    CPU_FTRS_7450_21 | CPU_FTRS_7450_23 | CPU_FTRS_7455_1 |
	    CPU_FTRS_7455_20 | CPU_FTRS_7455 | CPU_FTRS_7447_10 |
	    CPU_FTRS_7447 | CPU_FTRS_7447A | CPU_FTRS_82XX |
	    CPU_FTRS_G2_LE | CPU_FTRS_E300 | CPU_FTRS_E300C2 |
	    CPU_FTRS_CLASSIC32 |
#else
	    CPU_FTRS_GENERIC_32 |
#endif
#ifdef CONFIG_8xx
	    CPU_FTRS_8XX |
#endif
#ifdef CONFIG_40x
	    CPU_FTRS_40X |
#endif
#ifdef CONFIG_44x
	    CPU_FTRS_44X |
#endif
#ifdef CONFIG_E200
	    CPU_FTRS_E200 |
#endif
#ifdef CONFIG_E500
	    CPU_FTRS_E500 | CPU_FTRS_E500_2 |
#endif
	    0,
};
#endif /* __powerpc64__ */

#ifdef __powerpc64__
#define CPU_FTRS_ALWAYS		\
	    (CPU_FTRS_POWER3 & CPU_FTRS_RS64 & CPU_FTRS_POWER4 &	\
	    CPU_FTRS_PPC970 & CPU_FTRS_POWER5 & CPU_FTRS_POWER6 &	\
	    CPU_FTRS_CELL & CPU_FTRS_PA6T & CPU_FTRS_POSSIBLE)
#else
enum {
	CPU_FTRS_ALWAYS =
#if CLASSIC_PPC
	    CPU_FTRS_PPC601 & CPU_FTRS_603 & CPU_FTRS_604 & CPU_FTRS_740_NOTAU &
	    CPU_FTRS_740 & CPU_FTRS_750 & CPU_FTRS_750FX1 &
	    CPU_FTRS_750FX2 & CPU_FTRS_750FX & CPU_FTRS_750GX &
	    CPU_FTRS_7400_NOTAU & CPU_FTRS_7400 & CPU_FTRS_7450_20 &
	    CPU_FTRS_7450_21 & CPU_FTRS_7450_23 & CPU_FTRS_7455_1 &
	    CPU_FTRS_7455_20 & CPU_FTRS_7455 & CPU_FTRS_7447_10 &
	    CPU_FTRS_7447 & CPU_FTRS_7447A & CPU_FTRS_82XX &
	    CPU_FTRS_G2_LE & CPU_FTRS_E300 & CPU_FTRS_E300C2 &
	    CPU_FTRS_CLASSIC32 &
#else
	    CPU_FTRS_GENERIC_32 &
#endif
#ifdef CONFIG_8xx
	    CPU_FTRS_8XX &
#endif
#ifdef CONFIG_40x
	    CPU_FTRS_40X &
#endif
#ifdef CONFIG_44x
	    CPU_FTRS_44X &
#endif
#ifdef CONFIG_E200
	    CPU_FTRS_E200 &
#endif
#ifdef CONFIG_E500
	    CPU_FTRS_E500 & CPU_FTRS_E500_2 &
#endif
	    CPU_FTRS_POSSIBLE,
};
#endif /* __powerpc64__ */

static inline int cpu_has_feature(unsigned long feature)
{
	return (CPU_FTRS_ALWAYS & feature) ||
	       (CPU_FTRS_POSSIBLE
		& cur_cpu_spec->cpu_features
		& feature);
}

#endif /* !__ASSEMBLY__ */

#ifdef __ASSEMBLY__

#define BEGIN_FTR_SECTION_NESTED(label)	label:
#define BEGIN_FTR_SECTION		BEGIN_FTR_SECTION_NESTED(97)
#define END_FTR_SECTION_NESTED(msk, val, label) \
	MAKE_FTR_SECTION_ENTRY(msk, val, label, __ftr_fixup)
#define END_FTR_SECTION(msk, val)		\
	END_FTR_SECTION_NESTED(msk, val, 97)

#define END_FTR_SECTION_IFSET(msk)	END_FTR_SECTION((msk), (msk))
#define END_FTR_SECTION_IFCLR(msk)	END_FTR_SECTION((msk), 0)
#endif /* __ASSEMBLY__ */

#endif /* __KERNEL__ */
#endif /* __ASM_POWERPC_CPUTABLE_H */
