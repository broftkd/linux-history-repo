/*
 * Scatterlist Cryptographic API.
 *
 * Copyright (c) 2002 James Morris <jmorris@intercode.com.au>
 * Copyright (c) 2002 David S. Miller (davem@redhat.com)
 * Copyright (c) 2005 Herbert Xu <herbert@gondor.apana.org.au>
 *
 * Portions derived from Cryptoapi, by Alexander Kjeldaas <astor@fast.no>
 * and Nettle, by Niels M�ller.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 */
#ifndef _LINUX_CRYPTO_H
#define _LINUX_CRYPTO_H

#include <asm/atomic.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

/*
 * Algorithm masks and types.
 */
#define CRYPTO_ALG_TYPE_MASK		0x0000000f
#define CRYPTO_ALG_TYPE_CIPHER		0x00000001
#define CRYPTO_ALG_TYPE_DIGEST		0x00000002
#define CRYPTO_ALG_TYPE_COMPRESS	0x00000004

#define CRYPTO_ALG_LARVAL		0x00000010
#define CRYPTO_ALG_DEAD			0x00000020
#define CRYPTO_ALG_DYING		0x00000040
#define CRYPTO_ALG_ASYNC		0x00000080

/*
 * Transform masks and values (for crt_flags).
 */
#define CRYPTO_TFM_MODE_MASK		0x000000ff
#define CRYPTO_TFM_REQ_MASK		0x000fff00
#define CRYPTO_TFM_RES_MASK		0xfff00000

#define CRYPTO_TFM_MODE_ECB		0x00000001
#define CRYPTO_TFM_MODE_CBC		0x00000002
#define CRYPTO_TFM_MODE_CFB		0x00000004
#define CRYPTO_TFM_MODE_CTR		0x00000008

#define CRYPTO_TFM_REQ_WEAK_KEY		0x00000100
#define CRYPTO_TFM_REQ_MAY_SLEEP	0x00000200
#define CRYPTO_TFM_RES_WEAK_KEY		0x00100000
#define CRYPTO_TFM_RES_BAD_KEY_LEN   	0x00200000
#define CRYPTO_TFM_RES_BAD_KEY_SCHED 	0x00400000
#define CRYPTO_TFM_RES_BAD_BLOCK_LEN 	0x00800000
#define CRYPTO_TFM_RES_BAD_FLAGS 	0x01000000

/*
 * Miscellaneous stuff.
 */
#define CRYPTO_UNSPEC			0
#define CRYPTO_MAX_ALG_NAME		64

#define CRYPTO_DIR_ENCRYPT		1
#define CRYPTO_DIR_DECRYPT		0

/*
 * The macro CRYPTO_MINALIGN_ATTR (along with the void * type in the actual
 * declaration) is used to ensure that the crypto_tfm context structure is
 * aligned correctly for the given architecture so that there are no alignment
 * faults for C data types.  In particular, this is required on platforms such
 * as arm where pointers are 32-bit aligned but there are data types such as
 * u64 which require 64-bit alignment.
 */
#if defined(ARCH_KMALLOC_MINALIGN)
#define CRYPTO_MINALIGN ARCH_KMALLOC_MINALIGN
#elif defined(ARCH_SLAB_MINALIGN)
#define CRYPTO_MINALIGN ARCH_SLAB_MINALIGN
#endif

#ifdef CRYPTO_MINALIGN
#define CRYPTO_MINALIGN_ATTR __attribute__ ((__aligned__(CRYPTO_MINALIGN)))
#else
#define CRYPTO_MINALIGN_ATTR
#endif

struct scatterlist;
struct crypto_tfm;
struct crypto_type;

struct cipher_desc {
	struct crypto_tfm *tfm;
	void (*crfn)(struct crypto_tfm *tfm, u8 *dst, const u8 *src);
	unsigned int (*prfn)(const struct cipher_desc *desc, u8 *dst,
			     const u8 *src, unsigned int nbytes);
	void *info;
};

/*
 * Algorithms: modular crypto algorithm implementations, managed
 * via crypto_register_alg() and crypto_unregister_alg().
 */
struct cipher_alg {
	unsigned int cia_min_keysize;
	unsigned int cia_max_keysize;
	int (*cia_setkey)(struct crypto_tfm *tfm, const u8 *key,
	                  unsigned int keylen);
	void (*cia_encrypt)(struct crypto_tfm *tfm, u8 *dst, const u8 *src);
	void (*cia_decrypt)(struct crypto_tfm *tfm, u8 *dst, const u8 *src);

	unsigned int (*cia_encrypt_ecb)(const struct cipher_desc *desc,
					u8 *dst, const u8 *src,
					unsigned int nbytes);
	unsigned int (*cia_decrypt_ecb)(const struct cipher_desc *desc,
					u8 *dst, const u8 *src,
					unsigned int nbytes);
	unsigned int (*cia_encrypt_cbc)(const struct cipher_desc *desc,
					u8 *dst, const u8 *src,
					unsigned int nbytes);
	unsigned int (*cia_decrypt_cbc)(const struct cipher_desc *desc,
					u8 *dst, const u8 *src,
					unsigned int nbytes);
};

struct digest_alg {
	unsigned int dia_digestsize;
	void (*dia_init)(struct crypto_tfm *tfm);
	void (*dia_update)(struct crypto_tfm *tfm, const u8 *data,
			   unsigned int len);
	void (*dia_final)(struct crypto_tfm *tfm, u8 *out);
	int (*dia_setkey)(struct crypto_tfm *tfm, const u8 *key,
	                  unsigned int keylen);
};

struct compress_alg {
	int (*coa_compress)(struct crypto_tfm *tfm, const u8 *src,
			    unsigned int slen, u8 *dst, unsigned int *dlen);
	int (*coa_decompress)(struct crypto_tfm *tfm, const u8 *src,
			      unsigned int slen, u8 *dst, unsigned int *dlen);
};

#define cra_cipher	cra_u.cipher
#define cra_digest	cra_u.digest
#define cra_compress	cra_u.compress

struct crypto_alg {
	struct list_head cra_list;
	struct list_head cra_users;

	u32 cra_flags;
	unsigned int cra_blocksize;
	unsigned int cra_ctxsize;
	unsigned int cra_alignmask;

	int cra_priority;
	atomic_t cra_refcnt;

	char cra_name[CRYPTO_MAX_ALG_NAME];
	char cra_driver_name[CRYPTO_MAX_ALG_NAME];

	const struct crypto_type *cra_type;

	union {
		struct cipher_alg cipher;
		struct digest_alg digest;
		struct compress_alg compress;
	} cra_u;

	int (*cra_init)(struct crypto_tfm *tfm);
	void (*cra_exit)(struct crypto_tfm *tfm);
	void (*cra_destroy)(struct crypto_alg *alg);
	
	struct module *cra_module;
};

/*
 * Algorithm registration interface.
 */
int crypto_register_alg(struct crypto_alg *alg);
int crypto_unregister_alg(struct crypto_alg *alg);

/*
 * Algorithm query interface.
 */
#ifdef CONFIG_CRYPTO
int crypto_alg_available(const char *name, u32 flags);
#else
static inline int crypto_alg_available(const char *name, u32 flags)
{
	return 0;
}
#endif

/*
 * Transforms: user-instantiated objects which encapsulate algorithms
 * and core processing logic.  Managed via crypto_alloc_*() and
 * crypto_free_*(), as well as the various helpers below.
 */

struct cipher_tfm {
	void *cit_iv;
	unsigned int cit_ivsize;
	u32 cit_mode;
	int (*cit_setkey)(struct crypto_tfm *tfm,
	                  const u8 *key, unsigned int keylen);
	int (*cit_encrypt)(struct crypto_tfm *tfm,
			   struct scatterlist *dst,
			   struct scatterlist *src,
			   unsigned int nbytes);
	int (*cit_encrypt_iv)(struct crypto_tfm *tfm,
	                      struct scatterlist *dst,
	                      struct scatterlist *src,
	                      unsigned int nbytes, u8 *iv);
	int (*cit_decrypt)(struct crypto_tfm *tfm,
			   struct scatterlist *dst,
			   struct scatterlist *src,
			   unsigned int nbytes);
	int (*cit_decrypt_iv)(struct crypto_tfm *tfm,
			   struct scatterlist *dst,
			   struct scatterlist *src,
			   unsigned int nbytes, u8 *iv);
	void (*cit_xor_block)(u8 *dst, const u8 *src);
	void (*cit_encrypt_one)(struct crypto_tfm *tfm, u8 *dst, const u8 *src);
	void (*cit_decrypt_one)(struct crypto_tfm *tfm, u8 *dst, const u8 *src);
};

struct digest_tfm {
	void (*dit_init)(struct crypto_tfm *tfm);
	void (*dit_update)(struct crypto_tfm *tfm,
	                   struct scatterlist *sg, unsigned int nsg);
	void (*dit_final)(struct crypto_tfm *tfm, u8 *out);
	void (*dit_digest)(struct crypto_tfm *tfm, struct scatterlist *sg,
	                   unsigned int nsg, u8 *out);
	int (*dit_setkey)(struct crypto_tfm *tfm,
	                  const u8 *key, unsigned int keylen);
#ifdef CONFIG_CRYPTO_HMAC
	void *dit_hmac_block;
#endif
};

struct compress_tfm {
	int (*cot_compress)(struct crypto_tfm *tfm,
	                    const u8 *src, unsigned int slen,
	                    u8 *dst, unsigned int *dlen);
	int (*cot_decompress)(struct crypto_tfm *tfm,
	                      const u8 *src, unsigned int slen,
	                      u8 *dst, unsigned int *dlen);
};

#define crt_cipher	crt_u.cipher
#define crt_digest	crt_u.digest
#define crt_compress	crt_u.compress

struct crypto_tfm {

	u32 crt_flags;
	
	union {
		struct cipher_tfm cipher;
		struct digest_tfm digest;
		struct compress_tfm compress;
	} crt_u;
	
	struct crypto_alg *__crt_alg;

	void *__crt_ctx[] CRYPTO_MINALIGN_ATTR;
};

#define crypto_cipher crypto_tfm

enum {
	CRYPTOA_UNSPEC,
	CRYPTOA_ALG,
};

struct crypto_attr_alg {
	char name[CRYPTO_MAX_ALG_NAME];
};

/* 
 * Transform user interface.
 */
 
struct crypto_tfm *crypto_alloc_tfm(const char *alg_name, u32 tfm_flags);
struct crypto_tfm *crypto_alloc_base(const char *alg_name, u32 type, u32 mask);
void crypto_free_tfm(struct crypto_tfm *tfm);

/*
 * Transform helpers which query the underlying algorithm.
 */
static inline const char *crypto_tfm_alg_name(struct crypto_tfm *tfm)
{
	return tfm->__crt_alg->cra_name;
}

static inline const char *crypto_tfm_alg_driver_name(struct crypto_tfm *tfm)
{
	return tfm->__crt_alg->cra_driver_name;
}

static inline int crypto_tfm_alg_priority(struct crypto_tfm *tfm)
{
	return tfm->__crt_alg->cra_priority;
}

static inline const char *crypto_tfm_alg_modname(struct crypto_tfm *tfm)
{
	return module_name(tfm->__crt_alg->cra_module);
}

static inline u32 crypto_tfm_alg_type(struct crypto_tfm *tfm)
{
	return tfm->__crt_alg->cra_flags & CRYPTO_ALG_TYPE_MASK;
}

static inline unsigned int crypto_tfm_alg_min_keysize(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->__crt_alg->cra_cipher.cia_min_keysize;
}

static inline unsigned int crypto_tfm_alg_max_keysize(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->__crt_alg->cra_cipher.cia_max_keysize;
}

static inline unsigned int crypto_tfm_alg_ivsize(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->crt_cipher.cit_ivsize;
}

static inline unsigned int crypto_tfm_alg_blocksize(struct crypto_tfm *tfm)
{
	return tfm->__crt_alg->cra_blocksize;
}

static inline unsigned int crypto_tfm_alg_digestsize(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	return tfm->__crt_alg->cra_digest.dia_digestsize;
}

static inline unsigned int crypto_tfm_alg_alignmask(struct crypto_tfm *tfm)
{
	return tfm->__crt_alg->cra_alignmask;
}

static inline u32 crypto_tfm_get_flags(struct crypto_tfm *tfm)
{
	return tfm->crt_flags;
}

static inline void crypto_tfm_set_flags(struct crypto_tfm *tfm, u32 flags)
{
	tfm->crt_flags |= flags;
}

static inline void crypto_tfm_clear_flags(struct crypto_tfm *tfm, u32 flags)
{
	tfm->crt_flags &= ~flags;
}

static inline void *crypto_tfm_ctx(struct crypto_tfm *tfm)
{
	return tfm->__crt_ctx;
}

static inline unsigned int crypto_tfm_ctx_alignment(void)
{
	struct crypto_tfm *tfm;
	return __alignof__(tfm->__crt_ctx);
}

/*
 * API wrappers.
 */
static inline struct crypto_cipher *__crypto_cipher_cast(struct crypto_tfm *tfm)
{
	return (struct crypto_cipher *)tfm;
}

static inline struct crypto_cipher *crypto_cipher_cast(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return __crypto_cipher_cast(tfm);
}

static inline struct crypto_cipher *crypto_alloc_cipher(const char *alg_name,
							u32 type, u32 mask)
{
	type &= ~CRYPTO_ALG_TYPE_MASK;
	type |= CRYPTO_ALG_TYPE_CIPHER;
	mask |= CRYPTO_ALG_TYPE_MASK;

	return __crypto_cipher_cast(crypto_alloc_base(alg_name, type, mask));
}

static inline struct crypto_tfm *crypto_cipher_tfm(struct crypto_cipher *tfm)
{
	return tfm;
}

static inline void crypto_free_cipher(struct crypto_cipher *tfm)
{
	crypto_free_tfm(crypto_cipher_tfm(tfm));
}

static inline struct cipher_tfm *crypto_cipher_crt(struct crypto_cipher *tfm)
{
	return &crypto_cipher_tfm(tfm)->crt_cipher;
}

static inline unsigned int crypto_cipher_blocksize(struct crypto_cipher *tfm)
{
	return crypto_tfm_alg_blocksize(crypto_cipher_tfm(tfm));
}

static inline unsigned int crypto_cipher_alignmask(struct crypto_cipher *tfm)
{
	return crypto_tfm_alg_alignmask(crypto_cipher_tfm(tfm));
}

static inline u32 crypto_cipher_get_flags(struct crypto_cipher *tfm)
{
	return crypto_tfm_get_flags(crypto_cipher_tfm(tfm));
}

static inline void crypto_cipher_set_flags(struct crypto_cipher *tfm,
					   u32 flags)
{
	crypto_tfm_set_flags(crypto_cipher_tfm(tfm), flags);
}

static inline void crypto_cipher_clear_flags(struct crypto_cipher *tfm,
					     u32 flags)
{
	crypto_tfm_clear_flags(crypto_cipher_tfm(tfm), flags);
}

static inline void crypto_cipher_encrypt_one(struct crypto_cipher *tfm,
					     u8 *dst, const u8 *src)
{
	crypto_cipher_crt(tfm)->cit_encrypt_one(crypto_cipher_tfm(tfm),
						dst, src);
}

static inline void crypto_cipher_decrypt_one(struct crypto_cipher *tfm,
					     u8 *dst, const u8 *src)
{
	crypto_cipher_crt(tfm)->cit_decrypt_one(crypto_cipher_tfm(tfm),
						dst, src);
}

static inline void crypto_digest_init(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	tfm->crt_digest.dit_init(tfm);
}

static inline void crypto_digest_update(struct crypto_tfm *tfm,
                                        struct scatterlist *sg,
                                        unsigned int nsg)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	tfm->crt_digest.dit_update(tfm, sg, nsg);
}

static inline void crypto_digest_final(struct crypto_tfm *tfm, u8 *out)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	tfm->crt_digest.dit_final(tfm, out);
}

static inline void crypto_digest_digest(struct crypto_tfm *tfm,
                                        struct scatterlist *sg,
                                        unsigned int nsg, u8 *out)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	tfm->crt_digest.dit_digest(tfm, sg, nsg, out);
}

static inline int crypto_digest_setkey(struct crypto_tfm *tfm,
                                       const u8 *key, unsigned int keylen)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	return tfm->crt_digest.dit_setkey(tfm, key, keylen);
}

static inline int crypto_cipher_setkey(struct crypto_tfm *tfm,
                                       const u8 *key, unsigned int keylen)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->crt_cipher.cit_setkey(tfm, key, keylen);
}

static inline int crypto_cipher_encrypt(struct crypto_tfm *tfm,
                                        struct scatterlist *dst,
                                        struct scatterlist *src,
                                        unsigned int nbytes)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->crt_cipher.cit_encrypt(tfm, dst, src, nbytes);
}                                        

static inline int crypto_cipher_encrypt_iv(struct crypto_tfm *tfm,
                                           struct scatterlist *dst,
                                           struct scatterlist *src,
                                           unsigned int nbytes, u8 *iv)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->crt_cipher.cit_encrypt_iv(tfm, dst, src, nbytes, iv);
}                                        

static inline int crypto_cipher_decrypt(struct crypto_tfm *tfm,
                                        struct scatterlist *dst,
                                        struct scatterlist *src,
                                        unsigned int nbytes)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->crt_cipher.cit_decrypt(tfm, dst, src, nbytes);
}

static inline int crypto_cipher_decrypt_iv(struct crypto_tfm *tfm,
                                           struct scatterlist *dst,
                                           struct scatterlist *src,
                                           unsigned int nbytes, u8 *iv)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->crt_cipher.cit_decrypt_iv(tfm, dst, src, nbytes, iv);
}

static inline void crypto_cipher_set_iv(struct crypto_tfm *tfm,
                                        const u8 *src, unsigned int len)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	memcpy(tfm->crt_cipher.cit_iv, src, len);
}

static inline void crypto_cipher_get_iv(struct crypto_tfm *tfm,
                                        u8 *dst, unsigned int len)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	memcpy(dst, tfm->crt_cipher.cit_iv, len);
}

static inline int crypto_comp_compress(struct crypto_tfm *tfm,
                                       const u8 *src, unsigned int slen,
                                       u8 *dst, unsigned int *dlen)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_COMPRESS);
	return tfm->crt_compress.cot_compress(tfm, src, slen, dst, dlen);
}

static inline int crypto_comp_decompress(struct crypto_tfm *tfm,
                                         const u8 *src, unsigned int slen,
                                         u8 *dst, unsigned int *dlen)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_COMPRESS);
	return tfm->crt_compress.cot_decompress(tfm, src, slen, dst, dlen);
}

/*
 * HMAC support.
 */
#ifdef CONFIG_CRYPTO_HMAC
void crypto_hmac_init(struct crypto_tfm *tfm, u8 *key, unsigned int *keylen);
void crypto_hmac_update(struct crypto_tfm *tfm,
                        struct scatterlist *sg, unsigned int nsg);
void crypto_hmac_final(struct crypto_tfm *tfm, u8 *key,
                       unsigned int *keylen, u8 *out);
void crypto_hmac(struct crypto_tfm *tfm, u8 *key, unsigned int *keylen,
                 struct scatterlist *sg, unsigned int nsg, u8 *out);
#endif	/* CONFIG_CRYPTO_HMAC */

#endif	/* _LINUX_CRYPTO_H */

