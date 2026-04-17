/* OpenSSL 3.4.1 — ssl3_cbc.c: CBC padding + Lucky13 mitigation header — L1-130 */
/*
 * Copyright 2012-2023 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/*
 * This file has no dependencies on the rest of libssl because it is shared
 * with the providers. It contains functions for low level MAC calculations.
 * Responsibility for this lies with the HMAC implementation in the
 * providers. However there are legacy code paths in libssl which also need to
 * do this. In time those legacy code paths can be removed and this file can be
 * moved out of libssl.
 */

/*
 * MD5 and SHA-1 low level APIs are deprecated for public use, but still ok for
 * internal use.
 */
#include "internal/deprecated.h"

#include <openssl/evp.h>
#ifndef FIPS_MODULE
# include <openssl/md5.h>
#endif
#include <openssl/sha.h>

#include "internal/ssl3_cbc.h"
#include "internal/constant_time.h"
#include "internal/cryptlib.h"

/*
 * MAX_HASH_BIT_COUNT_BYTES is the maximum number of bytes in the hash's
 * length field. (SHA-384/512 have 128-bit length.)
 */
#define MAX_HASH_BIT_COUNT_BYTES 16

/*
 * MAX_HASH_BLOCK_SIZE is the maximum hash block size that we'll support.
 * Currently SHA-384/512 has a 128-byte block size and that's the largest
 * supported by TLS.)
 */
#define MAX_HASH_BLOCK_SIZE 128

#ifndef FIPS_MODULE
/*
 * u32toLE serializes an unsigned, 32-bit number (n) as four bytes at (p) in
 * little-endian order. The value of p is advanced by four.
 */
# define u32toLE(n, p) \
         (*((p)++) = (unsigned char)(n      ), \
          *((p)++) = (unsigned char)(n >>  8), \
          *((p)++) = (unsigned char)(n >> 16), \
          *((p)++) = (unsigned char)(n >> 24))

/*
 * These functions serialize the state of a hash and thus perform the
 * standard "final" operation without adding the padding and length that such
 * a function typically does.
 */
static void tls1_md5_final_raw(void *ctx, unsigned char *md_out)
{
    MD5_CTX *md5 = ctx;

    u32toLE(md5->A, md_out);
    u32toLE(md5->B, md_out);
    u32toLE(md5->C, md_out);
    u32toLE(md5->D, md_out);
}
#endif /* FIPS_MODULE */

static void tls1_sha1_final_raw(void *ctx, unsigned char *md_out)
{
    SHA_CTX *sha1 = ctx;

    l2n(sha1->h0, md_out);
    l2n(sha1->h1, md_out);
    l2n(sha1->h2, md_out);
    l2n(sha1->h3, md_out);
    l2n(sha1->h4, md_out);
}

static void tls1_sha256_final_raw(void *ctx, unsigned char *md_out)
{
    SHA256_CTX *sha256 = ctx;
    unsigned i;

    for (i = 0; i < 8; i++)
        l2n(sha256->h[i], md_out);
}

static void tls1_sha512_final_raw(void *ctx, unsigned char *md_out)
{
    SHA512_CTX *sha512 = ctx;
    unsigned i;

    for (i = 0; i < 8; i++)
        l2n8(sha512->h[i], md_out);
}

#undef  LARGEST_DIGEST_CTX
#define LARGEST_DIGEST_CTX SHA512_CTX

/*-
 * ssl3_cbc_digest_record computes the MAC of a decrypted, padded SSLv3/TLS
 * record.
 *
 *   ctx: the EVP_MD_CTX from which we take the hash function.
 *     ssl3_cbc_record_digest_supported must return true for this EVP_MD_CTX.
 *   md_out: the digest output. At most EVP_MAX_MD_SIZE bytes will be written.
 *   md_out_size: if non-NULL, the number of output bytes is written here.
 *   header: the 13-byte, TLS record header.
 *   data: the record data itself, less any preceding explicit IV.
 *   data_size: the secret, reported length of the data once the MAC and padding
 *              has been removed.
 *   data_plus_mac_plus_padding_size: the public length of the whole
 *     record, including MAC and padding.
 *   is_sslv3: non-zero if we are to use SSLv3. Otherwise, TLS.
 *
 * On entry: we know that data is data_plus_mac_plus_padding_size in length
 * Returns 1 on success or 0 on error
 */
int ssl3_cbc_digest_record(const EVP_MD *md,
                           unsigned char *md_out,
                           size_t *md_out_size,
                           const unsigned char *header,
                           const unsigned char *data,
