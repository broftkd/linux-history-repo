/*
 * Cryptographic API.
 *
 * RIPEMD-128 - RACE Integrity Primitives Evaluation Message Digest.
 *
 * Based on the reference implementation by Antoon Bosselaers, ESAT-COSIC
 *
 * Copyright (c) 2008 Adrian-Ken Rueegsegger <rueegsegger (at) swiss-it.ch>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/crypto.h>
#include <linux/cryptohash.h>
#include <linux/types.h>
#include <asm/byteorder.h>

#include "ripemd.h"

struct rmd128_ctx {
	u64 byte_count;
	u32 state[4];
	u32 buffer[16];
};

#define K1  RMD_K1
#define K2  RMD_K2
#define K3  RMD_K3
#define K4  RMD_K4
#define KK1 RMD_K6
#define KK2 RMD_K7
#define KK3 RMD_K8
#define KK4 RMD_K1

#define F1(x, y, z) (x ^ y ^ z)		/* XOR */
#define F2(x, y, z) (z ^ (x & (y ^ z)))	/* x ? y : z */
#define F3(x, y, z) ((x | ~y) ^ z)
#define F4(x, y, z) (y ^ (z & (x ^ y)))	/* z ? x : y */

#define ROUND(a, b, c, d, f, k, x, s)  { \
	(a) += f((b), (c), (d)) + (x) + (k); \
	(a) = rol32((a), (s)); \
}

static void rmd128_transform(u32 *state, u32 const *in)
{
	u32 aa, bb, cc, dd, aaa, bbb, ccc, ddd;

	/* Initialize left lane */
	aa = state[0];
	bb = state[1];
	cc = state[2];
	dd = state[3];

	/* Initialize right lane */
	aaa = state[0];
	bbb = state[1];
	ccc = state[2];
	ddd = state[3];

	/* round 1: left lane */
	ROUND(aa, bb, cc, dd, F1, K1, in[0],  11);
	ROUND(dd, aa, bb, cc, F1, K1, in[1],  14);
	ROUND(cc, dd, aa, bb, F1, K1, in[2],  15);
	ROUND(bb, cc, dd, aa, F1, K1, in[3],  12);
	ROUND(aa, bb, cc, dd, F1, K1, in[4],   5);
	ROUND(dd, aa, bb, cc, F1, K1, in[5],   8);
	ROUND(cc, dd, aa, bb, F1, K1, in[6],   7);
	ROUND(bb, cc, dd, aa, F1, K1, in[7],   9);
	ROUND(aa, bb, cc, dd, F1, K1, in[8],  11);
	ROUND(dd, aa, bb, cc, F1, K1, in[9],  13);
	ROUND(cc, dd, aa, bb, F1, K1, in[10], 14);
	ROUND(bb, cc, dd, aa, F1, K1, in[11], 15);
	ROUND(aa, bb, cc, dd, F1, K1, in[12],  6);
	ROUND(dd, aa, bb, cc, F1, K1, in[13],  7);
	ROUND(cc, dd, aa, bb, F1, K1, in[14],  9);
	ROUND(bb, cc, dd, aa, F1, K1, in[15],  8);

	/* round 2: left lane */
	ROUND(aa, bb, cc, dd, F2, K2, in[7],   7);
	ROUND(dd, aa, bb, cc, F2, K2, in[4],   6);
	ROUND(cc, dd, aa, bb, F2, K2, in[13],  8);
	ROUND(bb, cc, dd, aa, F2, K2, in[1],  13);
	ROUND(aa, bb, cc, dd, F2, K2, in[10], 11);
	ROUND(dd, aa, bb, cc, F2, K2, in[6],   9);
	ROUND(cc, dd, aa, bb, F2, K2, in[15],  7);
	ROUND(bb, cc, dd, aa, F2, K2, in[3],  15);
	ROUND(aa, bb, cc, dd, F2, K2, in[12],  7);
	ROUND(dd, aa, bb, cc, F2, K2, in[0],  12);
	ROUND(cc, dd, aa, bb, F2, K2, in[9],  15);
	ROUND(bb, cc, dd, aa, F2, K2, in[5],   9);
	ROUND(aa, bb, cc, dd, F2, K2, in[2],  11);
	ROUND(dd, aa, bb, cc, F2, K2, in[14],  7);
	ROUND(cc, dd, aa, bb, F2, K2, in[11], 13);
	ROUND(bb, cc, dd, aa, F2, K2, in[8],  12);

	/* round 3: left lane */
	ROUND(aa, bb, cc, dd, F3, K3, in[3],  11);
	ROUND(dd, aa, bb, cc, F3, K3, in[10], 13);
	ROUND(cc, dd, aa, bb, F3, K3, in[14],  6);
	ROUND(bb, cc, dd, aa, F3, K3, in[4],   7);
	ROUND(aa, bb, cc, dd, F3, K3, in[9],  14);
	ROUND(dd, aa, bb, cc, F3, K3, in[15],  9);
	ROUND(cc, dd, aa, bb, F3, K3, in[8],  13);
	ROUND(bb, cc, dd, aa, F3, K3, in[1],  15);
	ROUND(aa, bb, cc, dd, F3, K3, in[2],  14);
	ROUND(dd, aa, bb, cc, F3, K3, in[7],   8);
	ROUND(cc, dd, aa, bb, F3, K3, in[0],  13);
	ROUND(bb, cc, dd, aa, F3, K3, in[6],   6);
	ROUND(aa, bb, cc, dd, F3, K3, in[13],  5);
	ROUND(dd, aa, bb, cc, F3, K3, in[11], 12);
	ROUND(cc, dd, aa, bb, F3, K3, in[5],   7);
	ROUND(bb, cc, dd, aa, F3, K3, in[12],  5);

	/* round 4: left lane */
	ROUND(aa, bb, cc, dd, F4, K4, in[1],  11);
	ROUND(dd, aa, bb, cc, F4, K4, in[9],  12);
	ROUND(cc, dd, aa, bb, F4, K4, in[11], 14);
	ROUND(bb, cc, dd, aa, F4, K4, in[10], 15);
	ROUND(aa, bb, cc, dd, F4, K4, in[0],  14);
	ROUND(dd, aa, bb, cc, F4, K4, in[8],  15);
	ROUND(cc, dd, aa, bb, F4, K4, in[12],  9);
	ROUND(bb, cc, dd, aa, F4, K4, in[4],   8);
	ROUND(aa, bb, cc, dd, F4, K4, in[13],  9);
	ROUND(dd, aa, bb, cc, F4, K4, in[3],  14);
	ROUND(cc, dd, aa, bb, F4, K4, in[7],   5);
	ROUND(bb, cc, dd, aa, F4, K4, in[15],  6);
	ROUND(aa, bb, cc, dd, F4, K4, in[14],  8);
	ROUND(dd, aa, bb, cc, F4, K4, in[5],   6);
	ROUND(cc, dd, aa, bb, F4, K4, in[6],   5);
	ROUND(bb, cc, dd, aa, F4, K4, in[2],  12);

	/* round 1: right lane */
	ROUND(aaa, bbb, ccc, ddd, F4, KK1, in[5],   8);
	ROUND(ddd, aaa, bbb, ccc, F4, KK1, in[14],  9);
	ROUND(ccc, ddd, aaa, bbb, F4, KK1, in[7],   9);
	ROUND(bbb, ccc, ddd, aaa, F4, KK1, in[0],  11);
	ROUND(aaa, bbb, ccc, ddd, F4, KK1, in[9],  13);
	ROUND(ddd, aaa, bbb, ccc, F4, KK1, in[2],  15);
	ROUND(ccc, ddd, aaa, bbb, F4, KK1, in[11], 15);
	ROUND(bbb, ccc, ddd, aaa, F4, KK1, in[4],   5);
	ROUND(aaa, bbb, ccc, ddd, F4, KK1, in[13],  7);
	ROUND(ddd, aaa, bbb, ccc, F4, KK1, in[6],   7);
	ROUND(ccc, ddd, aaa, bbb, F4, KK1, in[15],  8);
	ROUND(bbb, ccc, ddd, aaa, F4, KK1, in[8],  11);
	ROUND(aaa, bbb, ccc, ddd, F4, KK1, in[1],  14);
	ROUND(ddd, aaa, bbb, ccc, F4, KK1, in[10], 14);
	ROUND(ccc, ddd, aaa, bbb, F4, KK1, in[3],  12);
	ROUND(bbb, ccc, ddd, aaa, F4, KK1, in[12],  6);

	/* round 2: right lane */
	ROUND(aaa, bbb, ccc, ddd, F3, KK2, in[6],   9);
	ROUND(ddd, aaa, bbb, ccc, F3, KK2, in[11], 13);
	ROUND(ccc, ddd, aaa, bbb, F3, KK2, in[3],  15);
	ROUND(bbb, ccc, ddd, aaa, F3, KK2, in[7],   7);
	ROUND(aaa, bbb, ccc, ddd, F3, KK2, in[0],  12);
	ROUND(ddd, aaa, bbb, ccc, F3, KK2, in[13],  8);
	ROUND(ccc, ddd, aaa, bbb, F3, KK2, in[5],   9);
	ROUND(bbb, ccc, ddd, aaa, F3, KK2, in[10], 11);
	ROUND(aaa, bbb, ccc, ddd, F3, KK2, in[14],  7);
	ROUND(ddd, aaa, bbb, ccc, F3, KK2, in[15],  7);
	ROUND(ccc, ddd, aaa, bbb, F3, KK2, in[8],  12);
	ROUND(bbb, ccc, ddd, aaa, F3, KK2, in[12],  7);
	ROUND(aaa, bbb, ccc, ddd, F3, KK2, in[4],   6);
	ROUND(ddd, aaa, bbb, ccc, F3, KK2, in[9],  15);
	ROUND(ccc, ddd, aaa, bbb, F3, KK2, in[1],  13);
	ROUND(bbb, ccc, ddd, aaa, F3, KK2, in[2],  11);

	/* round 3: right lane */
	ROUND(aaa, bbb, ccc, ddd, F2, KK3, in[15],  9);
	ROUND(ddd, aaa, bbb, ccc, F2, KK3, in[5],   7);
	ROUND(ccc, ddd, aaa, bbb, F2, KK3, in[1],  15);
	ROUND(bbb, ccc, ddd, aaa, F2, KK3, in[3],  11);
	ROUND(aaa, bbb, ccc, ddd, F2, KK3, in[7],   8);
	ROUND(ddd, aaa, bbb, ccc, F2, KK3, in[14],  6);
	ROUND(ccc, ddd, aaa, bbb, F2, KK3, in[6],   6);
	ROUND(bbb, ccc, ddd, aaa, F2, KK3, in[9],  14);
	ROUND(aaa, bbb, ccc, ddd, F2, KK3, in[11], 12);
	ROUND(ddd, aaa, bbb, ccc, F2, KK3, in[8],  13);
	ROUND(ccc, ddd, aaa, bbb, F2, KK3, in[12],  5);
	ROUND(bbb, ccc, ddd, aaa, F2, KK3, in[2],  14);
	ROUND(aaa, bbb, ccc, ddd, F2, KK3, in[10], 13);
	ROUND(ddd, aaa, bbb, ccc, F2, KK3, in[0],  13);
	ROUND(ccc, ddd, aaa, bbb, F2, KK3, in[4],   7);
	ROUND(bbb, ccc, ddd, aaa, F2, KK3, in[13],  5);

	/* round 4: right lane */
	ROUND(aaa, bbb, ccc, ddd, F1, KK4, in[8],  15);
	ROUND(ddd, aaa, bbb, ccc, F1, KK4, in[6],   5);
	ROUND(ccc, ddd, aaa, bbb, F1, KK4, in[4],   8);
	ROUND(bbb, ccc, ddd, aaa, F1, KK4, in[1],  11);
	ROUND(aaa, bbb, ccc, ddd, F1, KK4, in[3],  14);
	ROUND(ddd, aaa, bbb, ccc, F1, KK4, in[11], 14);
	ROUND(ccc, ddd, aaa, bbb, F1, KK4, in[15],  6);
	ROUND(bbb, ccc, ddd, aaa, F1, KK4, in[0],  14);
	ROUND(aaa, bbb, ccc, ddd, F1, KK4, in[5],   6);
	ROUND(ddd, aaa, bbb, ccc, F1, KK4, in[12],  9);
	ROUND(ccc, ddd, aaa, bbb, F1, KK4, in[2],  12);
	ROUND(bbb, ccc, ddd, aaa, F1, KK4, in[13],  9);
	ROUND(aaa, bbb, ccc, ddd, F1, KK4, in[9],  12);
	ROUND(ddd, aaa, bbb, ccc, F1, KK4, in[7],   5);
	ROUND(ccc, ddd, aaa, bbb, F1, KK4, in[10], 15);
	ROUND(bbb, ccc, ddd, aaa, F1, KK4, in[14],  8);

	/* combine results */
	ddd += cc + state[1];		/* final result for state[0] */
	state[1] = state[2] + dd + aaa;
	state[2] = state[3] + aa + bbb;
	state[3] = state[0] + bb + ccc;
	state[0] = ddd;

	return;
}

static inline void le32_to_cpu_array(u32 *buf, unsigned int words)
{
	while (words--) {
		le32_to_cpus(buf);
		buf++;
	}
}

static inline void cpu_to_le32_array(u32 *buf, unsigned int words)
{
	while (words--) {
		cpu_to_le32s(buf);
		buf++;
	}
}

static inline void rmd128_transform_helper(struct rmd128_ctx *ctx)
{
	le32_to_cpu_array(ctx->buffer, sizeof(ctx->buffer) / sizeof(u32));
	rmd128_transform(ctx->state, ctx->buffer);
}

static void rmd128_init(struct crypto_tfm *tfm)
{
	struct rmd128_ctx *rctx = crypto_tfm_ctx(tfm);

	rctx->byte_count = 0;

	rctx->state[0] = RMD_H0;
	rctx->state[1] = RMD_H1;
	rctx->state[2] = RMD_H2;
	rctx->state[3] = RMD_H3;

	memset(rctx->buffer, 0, sizeof(rctx->buffer));
}

static void rmd128_update(struct crypto_tfm *tfm, const u8 *data,
			  unsigned int len)
{
	struct rmd128_ctx *rctx = crypto_tfm_ctx(tfm);
	const u32 avail = sizeof(rctx->buffer) - (rctx->byte_count & 0x3f);

	rctx->byte_count += len;

	/* Enough space in buffer? If so copy and we're done */
	if (avail > len) {
		memcpy((char *)rctx->buffer + (sizeof(rctx->buffer) - avail),
		       data, len);
		return;
	}

	memcpy((char *)rctx->buffer + (sizeof(rctx->buffer) - avail),
	       data, avail);

	rmd128_transform_helper(rctx);
	data += avail;
	len -= avail;

	while (len >= sizeof(rctx->buffer)) {
		memcpy(rctx->buffer, data, sizeof(rctx->buffer));
		rmd128_transform_helper(rctx);
		data += sizeof(rctx->buffer);
		len -= sizeof(rctx->buffer);
	}

	memcpy(rctx->buffer, data, len);
}

/* Add padding and return the message digest. */
static void rmd128_final(struct crypto_tfm *tfm, u8 *out)
{
	struct rmd128_ctx *rctx = crypto_tfm_ctx(tfm);
	u32 index, padlen;
	u64 bits;
	static const u8 padding[64] = { 0x80, };
	bits = rctx->byte_count << 3;

	/* Pad out to 56 mod 64 */
	index = rctx->byte_count & 0x3f;
	padlen = (index < 56) ? (56 - index) : ((64+56) - index);
	rmd128_update(tfm, padding, padlen);

	/* Append length */
	rmd128_update(tfm, (const u8 *)&bits, sizeof(bits));

	/* Store state in digest */
	memcpy(out, rctx->state, sizeof(rctx->state));

	/* Wipe context */
	memset(rctx, 0, sizeof(*rctx));
}

static struct crypto_alg alg = {
	.cra_name	 =	"rmd128",
	.cra_driver_name =	"rmd128",
	.cra_flags	 =	CRYPTO_ALG_TYPE_DIGEST,
	.cra_blocksize	 =	RMD128_BLOCK_SIZE,
	.cra_ctxsize	 =	sizeof(struct rmd128_ctx),
	.cra_module	 =	THIS_MODULE,
	.cra_list	 =	LIST_HEAD_INIT(alg.cra_list),
	.cra_u		 =	{ .digest = {
	.dia_digestsize	 =	RMD128_DIGEST_SIZE,
	.dia_init	 =	rmd128_init,
	.dia_update	 =	rmd128_update,
	.dia_final	 =	rmd128_final } }
};

static int __init rmd128_mod_init(void)
{
	return crypto_register_alg(&alg);
}

static void __exit rmd128_mod_fini(void)
{
	crypto_unregister_alg(&alg);
}

module_init(rmd128_mod_init);
module_exit(rmd128_mod_fini);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RIPEMD-128 Message Digest");

MODULE_ALIAS("rmd128");
