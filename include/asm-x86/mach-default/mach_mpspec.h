#ifndef ASM_X86__MACH_DEFAULT__MACH_MPSPEC_H
#define ASM_X86__MACH_DEFAULT__MACH_MPSPEC_H

#define MAX_IRQ_SOURCES 256

#if CONFIG_BASE_SMALL == 0
#define MAX_MP_BUSSES 256
#else
#define MAX_MP_BUSSES 32
#endif

#endif /* ASM_X86__MACH_DEFAULT__MACH_MPSPEC_H */
