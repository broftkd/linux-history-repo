#ifndef ___ASM_SPARC_PGTABLE_H
#define ___ASM_SPARC_PGTABLE_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-sparc/pgtable_64.h>
#else
#include <asm-sparc/pgtable_32.h>
#endif
#endif
