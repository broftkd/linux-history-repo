/*
 * Cryptographic API.
 *
 * Copyright (c) 2002 James Morris <jmorris@intercode.com.au>
 * Copyright (c) 2005 Herbert Xu <herbert@gondor.apana.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 */
#ifndef _CRYPTO_INTERNAL_H
#define _CRYPTO_INTERNAL_H

#include <crypto/algapi.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/rwsem.h>
#include <linux/slab.h>
#include <asm/kmap_types.h>

struct crypto_instance;
struct crypto_template;

extern struct list_head crypto_alg_list;
extern struct rw_semaphore crypto_alg_sem;

extern enum km_type crypto_km_types[];

static inline enum km_type crypto_kmap_type(int out)
{
	return crypto_km_types[(in_softirq() ? 2 : 0) + out];
}

static inline void *crypto_kmap(struct page *page, int out)
{
	return kmap_atomic(page, crypto_kmap_type(out));
}

static inline void crypto_kunmap(void *vaddr, int out)
{
	kunmap_atomic(vaddr, crypto_kmap_type(out));
}

static inline void crypto_yield(struct crypto_tfm *tfm)
{
	if (tfm->crt_flags & CRYPTO_TFM_REQ_MAY_SLEEP)
		cond_resched();
}

#ifdef CONFIG_CRYPTO_HMAC
int crypto_alloc_hmac_block(struct crypto_tfm *tfm);
void crypto_free_hmac_block(struct crypto_tfm *tfm);
#else
static inline int crypto_alloc_hmac_block(struct crypto_tfm *tfm)
{
	return 0;
}

static inline void crypto_free_hmac_block(struct crypto_tfm *tfm)
{ }
#endif

#ifdef CONFIG_PROC_FS
void __init crypto_init_proc(void);
void __exit crypto_exit_proc(void);
#else
static inline void crypto_init_proc(void)
{ }
static inline void crypto_exit_proc(void)
{ }
#endif

static inline unsigned int crypto_digest_ctxsize(struct crypto_alg *alg,
						 int flags)
{
	return alg->cra_ctxsize;
}

static inline unsigned int crypto_cipher_ctxsize(struct crypto_alg *alg,
						 int flags)
{
	unsigned int len = alg->cra_ctxsize;
	
	switch (flags & CRYPTO_TFM_MODE_MASK) {
	case CRYPTO_TFM_MODE_CBC:
		len = ALIGN(len, (unsigned long)alg->cra_alignmask + 1);
		len += alg->cra_blocksize;
		break;
	}

	return len;
}

static inline unsigned int crypto_compress_ctxsize(struct crypto_alg *alg,
						   int flags)
{
	return alg->cra_ctxsize;
}

int crypto_init_digest_flags(struct crypto_tfm *tfm, u32 flags);
int crypto_init_cipher_flags(struct crypto_tfm *tfm, u32 flags);
int crypto_init_compress_flags(struct crypto_tfm *tfm, u32 flags);

int crypto_init_digest_ops(struct crypto_tfm *tfm);
int crypto_init_cipher_ops(struct crypto_tfm *tfm);
int crypto_init_compress_ops(struct crypto_tfm *tfm);

void crypto_exit_digest_ops(struct crypto_tfm *tfm);
void crypto_exit_cipher_ops(struct crypto_tfm *tfm);
void crypto_exit_compress_ops(struct crypto_tfm *tfm);

int crypto_register_instance(struct crypto_template *tmpl,
			     struct crypto_instance *inst);

static inline int crypto_tmpl_get(struct crypto_template *tmpl)
{
	return try_module_get(tmpl->module);
}

static inline void crypto_tmpl_put(struct crypto_template *tmpl)
{
	module_put(tmpl->module);
}

#endif	/* _CRYPTO_INTERNAL_H */

