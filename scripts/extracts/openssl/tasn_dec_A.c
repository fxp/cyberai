/* OpenSSL 3.4.1 — tasn_dec.c: ASN1_item_d2i + asn1_item_embed_d2i entry — L1-110 */
/*
 * Copyright 2000-2024 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <stddef.h>
#include <string.h>
#include <openssl/asn1.h>
#include <openssl/asn1t.h>
#include <openssl/objects.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include "internal/numbers.h"
#include "asn1_local.h"

/*
 * Constructed types with a recursive definition (such as can be found in PKCS7)
 * could eventually exceed the stack given malicious input with excessive
 * recursion. Therefore we limit the stack depth. This is the maximum number of
 * recursive invocations of asn1_item_embed_d2i().
 */
#define ASN1_MAX_CONSTRUCTED_NEST 30

static int asn1_item_embed_d2i(ASN1_VALUE **pval, const unsigned char **in,
                               long len, const ASN1_ITEM *it,
                               int tag, int aclass, char opt, ASN1_TLC *ctx,
                               int depth, OSSL_LIB_CTX *libctx,
                               const char *propq);

static int asn1_check_eoc(const unsigned char **in, long len);
static int asn1_find_end(const unsigned char **in, long len, char inf);

static int asn1_collect(BUF_MEM *buf, const unsigned char **in, long len,
                        char inf, int tag, int aclass, int depth);

static int collect_data(BUF_MEM *buf, const unsigned char **p, long plen);

static int asn1_check_tlen(long *olen, int *otag, unsigned char *oclass,
                           char *inf, char *cst,
                           const unsigned char **in, long len,
                           int exptag, int expclass, char opt, ASN1_TLC *ctx);

static int asn1_template_ex_d2i(ASN1_VALUE **pval,
                                const unsigned char **in, long len,
                                const ASN1_TEMPLATE *tt, char opt,
                                ASN1_TLC *ctx, int depth, OSSL_LIB_CTX *libctx,
                                const char *propq);
static int asn1_template_noexp_d2i(ASN1_VALUE **val,
                                   const unsigned char **in, long len,
                                   const ASN1_TEMPLATE *tt, char opt,
                                   ASN1_TLC *ctx, int depth,
                                   OSSL_LIB_CTX *libctx, const char *propq);
static int asn1_d2i_ex_primitive(ASN1_VALUE **pval,
                                 const unsigned char **in, long len,
                                 const ASN1_ITEM *it,
                                 int tag, int aclass, char opt,
                                 ASN1_TLC *ctx);
static int asn1_ex_c2i(ASN1_VALUE **pval, const unsigned char *cont, int len,
                       int utype, char *free_cont, const ASN1_ITEM *it);

/* Table to convert tags to bit values, used for MSTRING type */
static const unsigned long tag2bit[32] = {
    /* tags  0 -  3 */
    0, 0, 0, B_ASN1_BIT_STRING,
    /* tags  4- 7 */
    B_ASN1_OCTET_STRING, 0, 0, B_ASN1_UNKNOWN,
    /* tags  8-11 */
    B_ASN1_UNKNOWN, B_ASN1_UNKNOWN, 0, B_ASN1_UNKNOWN,
    /* tags 12-15 */
    B_ASN1_UTF8STRING, B_ASN1_UNKNOWN, B_ASN1_UNKNOWN, B_ASN1_UNKNOWN,
    /* tags 16-19 */
    B_ASN1_SEQUENCE, 0, B_ASN1_NUMERICSTRING, B_ASN1_PRINTABLESTRING,
    /* tags 20-22 */
    B_ASN1_T61STRING, B_ASN1_VIDEOTEXSTRING, B_ASN1_IA5STRING,
    /* tags 23-24 */
    B_ASN1_UTCTIME, B_ASN1_GENERALIZEDTIME,
    /* tags 25-27 */
    B_ASN1_GRAPHICSTRING, B_ASN1_ISO64STRING, B_ASN1_GENERALSTRING,
    /* tags 28-31 */
    B_ASN1_UNIVERSALSTRING, B_ASN1_UNKNOWN, B_ASN1_BMPSTRING, B_ASN1_UNKNOWN,
};

unsigned long ASN1_tag2bit(int tag)
{
    if ((tag < 0) || (tag > 30))
        return 0;
    return tag2bit[tag];
}

/* Macro to initialize and invalidate the cache */

#define asn1_tlc_clear(c)       do { if ((c) != NULL) (c)->valid = 0; } while (0)
/* Version to avoid compiler warning about 'c' always non-NULL */
#define asn1_tlc_clear_nc(c)    do {(c)->valid = 0; } while (0)

/*
 * Decode an ASN1 item, this currently behaves just like a standard 'd2i'
 * function. 'in' points to a buffer to read the data from, in future we
 * will have more advanced versions that can input data a piece at a time and
 * this will simply be a special case.
 */

static int asn1_item_ex_d2i_intern(ASN1_VALUE **pval, const unsigned char **in,
                                   long len, const ASN1_ITEM *it, int tag,
                                   int aclass, char opt, ASN1_TLC *ctx,
                                   OSSL_LIB_CTX *libctx, const char *propq)
