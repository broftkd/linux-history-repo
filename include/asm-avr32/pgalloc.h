/*
 * Copyright (C) 2004-2006 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ASM_AVR32_PGALLOC_H
#define __ASM_AVR32_PGALLOC_H

#include <linux/mm.h>

static inline void pmd_populate_kernel(struct mm_struct *mm,
				       pmd_t *pmd, pte_t *pte)
{
	set_pmd(pmd, __pmd((unsigned long)pte));
}

static inline void pmd_populate(struct mm_struct *mm, pmd_t *pmd,
				    pgtable_t pte)
{
	set_pmd(pmd, __pmd((unsigned long)page_address(pte)));
}
#define pmd_pgtable(pmd) pmd_page(pmd)

/*
 * Allocate and free page tables
 */
static inline pgd_t *pgd_alloc(struct mm_struct *mm)
{
	pgd_t *pgd;

	pgd = (pgd_t *)get_zeroed_page(GFP_KERNEL | __GFP_REPEAT);
	if (likely(pgd))
		memcpy(pgd + USER_PTRS_PER_PGD,
			swapper_pg_dir + USER_PTRS_PER_PGD,
			(PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t));

	return pgd;
}

static inline void pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
	free_page((unsigned long)pgd);
}

static inline pte_t *pte_alloc_one_kernel(struct mm_struct *mm,
					  unsigned long address)
{
	pte_t *pte;

	pte = (pte_t *)get_zeroed_page(GFP_KERNEL | __GFP_REPEAT);

	return pte;
}

static inline struct page *pte_alloc_one(struct mm_struct *mm,
					 unsigned long address)
{
	struct page *pte;

	pte = alloc_page(GFP_KERNEL | __GFP_REPEAT | __GFP_ZERO);
	if (!pte)
		return NULL;
	pgtable_page_ctor(pte);
	return pte;
}

static inline void pte_free_kernel(struct mm_struct *mm, pte_t *pte)
{
	free_page((unsigned long)pte);
}

static inline void pte_free(struct mm_struct *mm, pgtable_t pte)
{
	pgtable_page_dtor(pte);
	__free_page(pte);
}

#define __pte_free_tlb(tlb,pte)				\
do {							\
	pgtable_page_dtor(pte);				\
	tlb_remove_page((tlb), pte);			\
} while (0)

#define check_pgt_cache() do { } while(0)

#endif /* __ASM_AVR32_PGALLOC_H */
