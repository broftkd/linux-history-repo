#ifndef ___ASM_SPARC_BITOPS_H
#define ___ASM_SPARC_BITOPS_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm/bitops_64.h>
#else
#include <asm/bitops_32.h>
#endif
#endif
