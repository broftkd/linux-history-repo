#ifndef __ASM_SH_PROCESSOR_H
#define __ASM_SH_PROCESSOR_H

#include <asm/cpu-features.h>

/*
 *  CPU type and hardware bug flags. Kept separately for each CPU.
 *
 *  Each one of these also needs a CONFIG_CPU_SUBTYPE_xxx entry
 *  in arch/sh/mm/Kconfig, as well as an entry in arch/sh/kernel/setup.c
 *  for parsing the subtype in get_cpu_subtype().
 */
enum cpu_type {
	/* SH-2 types */
	CPU_SH7619,

	/* SH-2A types */
	CPU_SH7206,

	/* SH-3 types */
	CPU_SH7705, CPU_SH7706, CPU_SH7707,
	CPU_SH7708, CPU_SH7708S, CPU_SH7708R,
	CPU_SH7709, CPU_SH7709A, CPU_SH7710, CPU_SH7712,
	CPU_SH7720, CPU_SH7729,

	/* SH-4 types */
	CPU_SH7750, CPU_SH7750S, CPU_SH7750R, CPU_SH7751, CPU_SH7751R,
	CPU_SH7760, CPU_SH4_202, CPU_SH4_501,

	/* SH-4A types */
	CPU_SH7770, CPU_SH7780, CPU_SH7781, CPU_SH7785, CPU_SHX3,

	/* SH4AL-DSP types */
	CPU_SH7343, CPU_SH7722,

	/* SH-5 types */
        CPU_SH5_101, CPU_SH5_103,

	/* Unknown subtype */
	CPU_SH_NONE
};

#ifdef CONFIG_SUPERH32
# include "processor_32.h"
#else
# include "processor_64.h"
#endif

/* arch/sh/kernel/setup.c */
const char *get_cpu_subtype(struct sh_cpuinfo *c);

#endif /* __ASM_SH_PROCESSOR_H */
