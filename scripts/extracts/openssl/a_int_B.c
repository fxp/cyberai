/* OpenSSL 3.4.1 — a_int.c: c2i_ASN1_INTEGER body + sign handling — L130-260 */

    if (pp == NULL || (p = *pp) == NULL)
        return ret;

    /*
     * This magically handles all corner cases, such as '(b == NULL ||
     * blen == 0)', non-negative value, "negative" zero, 0x80 followed
     * by any number of zeros...
     */
    *p = pb;
    p += pad;       /* yes, p[0] can be written twice, but it's little
                     * price to pay for eliminated branches */
    twos_complement(p, b, blen, pb);

    *pp += ret;
    return ret;
}

/*
 * convert content octets into a big endian buffer. Returns the length
 * of buffer or 0 on error: for malformed INTEGER. If output buffer is
 * NULL just return length.
 */

static size_t c2i_ibuf(unsigned char *b, int *pneg,
                       const unsigned char *p, size_t plen)
{
    int neg, pad;
    /* Zero content length is illegal */
    if (plen == 0) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_ILLEGAL_ZERO_CONTENT);
        return 0;
    }
    neg = p[0] & 0x80;
    if (pneg)
        *pneg = neg;
    /* Handle common case where length is 1 octet separately */
    if (plen == 1) {
        if (b != NULL) {
            if (neg)
                b[0] = (p[0] ^ 0xFF) + 1;
            else
                b[0] = p[0];
        }
        return 1;
    }

    pad = 0;
    if (p[0] == 0) {
        pad = 1;
    } else if (p[0] == 0xFF) {
        size_t i;

        /*
         * Special case [of "one less minimal negative" for given length]:
         * if any other bytes non zero it was padded, otherwise not.
         */
        for (pad = 0, i = 1; i < plen; i++)
            pad |= p[i];
        pad = pad != 0 ? 1 : 0;
    }
    /* reject illegal padding: first two octets MSB can't match */
    if (pad && (neg == (p[1] & 0x80))) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_ILLEGAL_PADDING);
        return 0;
    }

    /* skip over pad */
    p += pad;
    plen -= pad;

    if (b != NULL)
        twos_complement(b, p, plen, neg ? 0xffU : 0);

    return plen;
}

int ossl_i2c_ASN1_INTEGER(ASN1_INTEGER *a, unsigned char **pp)
{
    return i2c_ibuf(a->data, a->length, a->type & V_ASN1_NEG, pp);
}

/* Convert big endian buffer into uint64_t, return 0 on error */
static int asn1_get_uint64(uint64_t *pr, const unsigned char *b, size_t blen)
{
    size_t i;
    uint64_t r;

    if (blen > sizeof(*pr)) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_TOO_LARGE);
        return 0;
    }
    if (b == NULL)
        return 0;
    for (r = 0, i = 0; i < blen; i++) {
        r <<= 8;
        r |= b[i];
    }
    *pr = r;
    return 1;
}

/*
 * Write uint64_t to big endian buffer and return offset to first
 * written octet. In other words it returns offset in range from 0
 * to 7, with 0 denoting 8 written octets and 7 - one.
 */
static size_t asn1_put_uint64(unsigned char b[sizeof(uint64_t)], uint64_t r)
{
    size_t off = sizeof(uint64_t);

    do {
        b[--off] = (unsigned char)r;
    } while (r >>= 8);

    return off;
}

/*
 * Absolute value of INT64_MIN: we can't just use -INT64_MIN as gcc produces
 * overflow warnings.
 */
#define ABS_INT64_MIN ((uint64_t)INT64_MAX + (-(INT64_MIN + INT64_MAX)))

/* signed version of asn1_get_uint64 */
static int asn1_get_int64(int64_t *pr, const unsigned char *b, size_t blen,
                          int neg)
{
    uint64_t r;
    if (asn1_get_uint64(&r, b, blen) == 0)
        return 0;
