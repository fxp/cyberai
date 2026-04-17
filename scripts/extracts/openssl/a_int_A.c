/* OpenSSL 3.4.1 — a_int.c: ASN1_INTEGER_cmp + c2i_ASN1_INTEGER entry — L1-130 */
/*
 * Copyright 1995-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <stdio.h>
#include "internal/cryptlib.h"
#include "internal/numbers.h"
#include <limits.h>
#include <openssl/asn1.h>
#include <openssl/bn.h>
#include "asn1_local.h"

ASN1_INTEGER *ASN1_INTEGER_dup(const ASN1_INTEGER *x)
{
    return ASN1_STRING_dup(x);
}

int ASN1_INTEGER_cmp(const ASN1_INTEGER *x, const ASN1_INTEGER *y)
{
    int neg, ret;
    /* Compare signs */
    neg = x->type & V_ASN1_NEG;
    if (neg != (y->type & V_ASN1_NEG)) {
        if (neg)
            return -1;
        else
            return 1;
    }

    ret = ASN1_STRING_cmp(x, y);

    if (neg)
        return -ret;
    else
        return ret;
}

/*-
 * This converts a big endian buffer and sign into its content encoding.
 * This is used for INTEGER and ENUMERATED types.
 * The internal representation is an ASN1_STRING whose data is a big endian
 * representation of the value, ignoring the sign. The sign is determined by
 * the type: if type & V_ASN1_NEG is true it is negative, otherwise positive.
 *
 * Positive integers are no problem: they are almost the same as the DER
 * encoding, except if the first byte is >= 0x80 we need to add a zero pad.
 *
 * Negative integers are a bit trickier...
 * The DER representation of negative integers is in 2s complement form.
 * The internal form is converted by complementing each octet and finally
 * adding one to the result. This can be done less messily with a little trick.
 * If the internal form has trailing zeroes then they will become FF by the
 * complement and 0 by the add one (due to carry) so just copy as many trailing
 * zeros to the destination as there are in the source. The carry will add one
 * to the last none zero octet: so complement this octet and add one and finally
 * complement any left over until you get to the start of the string.
 *
 * Padding is a little trickier too. If the first bytes is > 0x80 then we pad
 * with 0xff. However if the first byte is 0x80 and one of the following bytes
 * is non-zero we pad with 0xff. The reason for this distinction is that 0x80
 * followed by optional zeros isn't padded.
 */

/*
 * If |pad| is zero, the operation is effectively reduced to memcpy,
 * and if |pad| is 0xff, then it performs two's complement, ~dst + 1.
 * Note that in latter case sequence of zeros yields itself, and so
 * does 0x80 followed by any number of zeros. These properties are
 * used elsewhere below...
 */
static void twos_complement(unsigned char *dst, const unsigned char *src,
                            size_t len, unsigned char pad)
{
    unsigned int carry = pad & 1;

    /* Begin at the end of the encoding */
    if (len != 0) {
        /*
         * if len == 0 then src/dst could be NULL, and this would be undefined
         * behaviour.
         */
        dst += len;
        src += len;
    }
    /* two's complement value: ~value + 1 */
    while (len-- != 0) {
        *(--dst) = (unsigned char)(carry += *(--src) ^ pad);
        carry >>= 8;
    }
}

static size_t i2c_ibuf(const unsigned char *b, size_t blen, int neg,
                       unsigned char **pp)
{
    unsigned int pad = 0;
    size_t ret, i;
    unsigned char *p, pb = 0;

    if (b != NULL && blen) {
        ret = blen;
        i = b[0];
        if (!neg && (i > 127)) {
            pad = 1;
            pb = 0;
        } else if (neg) {
            pb = 0xFF;
            if (i > 128) {
                pad = 1;
            } else if (i == 128) {
                /*
                 * Special case [of minimal negative for given length]:
                 * if any other bytes non zero we pad, otherwise we don't.
                 */
                for (pad = 0, i = 1; i < blen; i++)
                    pad |= b[i];
                pb = pad != 0 ? 0xffU : 0;
                pad = pb & 1;
            }
        }
        ret += pad;
    } else {
        ret = 1;
        blen = 0;   /* reduce '(b == NULL || blen == 0)' to '(blen == 0)' */
    }

