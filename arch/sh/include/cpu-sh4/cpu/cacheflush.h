/*
 * include/asm-sh/cpu-sh4/cacheflush.h
 *
 * Copyright (C) 1999 Niibe Yutaka
 * Copyright (C) 2003 Paul Mundt
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#ifndef __ASM_CPU_SH4_CACHEFLUSH_H
#define __ASM_CPU_SH4_CACHEFLUSH_H

/*
 *  Caches are broken on SH-4 (unless we use write-through
 *  caching; in which case they're only semi-broken),
 *  so we need them.
 */
void flush_cache_all(void);
void flush_cache_mm(struct mm_struct *mm);
#define flush_cache_dup_mm(mm) flush_cache_mm(mm)
void flush_cache_range(struct vm_area_struct *vma, unsigned long start,
		       unsigned long end);
void flush_cache_page(struct vm_area_struct *vma, unsigned long addr,
		      unsigned long pfn);
void flush_dcache_page(struct page *pg);
void flush_icache_range(unsigned long start, unsigned long end);

#define flush_icache_page(vma,pg)		do { } while (0)

#endif /* __ASM_CPU_SH4_CACHEFLUSH_H */
