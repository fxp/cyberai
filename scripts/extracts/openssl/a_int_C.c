/* OpenSSL 3.4.1 — a_int.c: asn1_get_uint64 + asn1_get_int64: overflow guards — L213-370 */
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
    if (neg) {
        if (r <= INT64_MAX) {
            /* Most significant bit is guaranteed to be clear, negation
             * is guaranteed to be meaningful in platform-neutral sense. */
            *pr = -(int64_t)r;
        } else if (r == ABS_INT64_MIN) {
            /* This never happens if INT64_MAX == ABS_INT64_MIN, e.g.
             * on ones'-complement system. */
            *pr = (int64_t)(0 - r);
        } else {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_TOO_SMALL);
            return 0;
        }
    } else {
        if (r <= INT64_MAX) {
            *pr = (int64_t)r;
        } else {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_TOO_LARGE);
            return 0;
        }
    }
    return 1;
}

/* Convert ASN1 INTEGER content octets to ASN1_INTEGER structure */
ASN1_INTEGER *ossl_c2i_ASN1_INTEGER(ASN1_INTEGER **a, const unsigned char **pp,
                                    long len)
{
    ASN1_INTEGER *ret = NULL;
    size_t r;
    int neg;

    r = c2i_ibuf(NULL, NULL, *pp, len);

    if (r == 0)
        return NULL;

    if ((a == NULL) || ((*a) == NULL)) {
        ret = ASN1_INTEGER_new();
        if (ret == NULL)
            return NULL;
        ret->type = V_ASN1_INTEGER;
    } else
        ret = *a;

    if (ASN1_STRING_set(ret, NULL, r) == 0) {
        ERR_raise(ERR_LIB_ASN1, ERR_R_ASN1_LIB);
        goto err;
    }

    c2i_ibuf(ret->data, &neg, *pp, len);

    if (neg != 0)
        ret->type |= V_ASN1_NEG;
    else
        ret->type &= ~V_ASN1_NEG;

    *pp += len;
    if (a != NULL)
        (*a) = ret;
    return ret;
 err:
    if (a == NULL || *a != ret)
        ASN1_INTEGER_free(ret);
    return NULL;
}

static int asn1_string_get_int64(int64_t *pr, const ASN1_STRING *a, int itype)
{
    if (a == NULL) {
        ERR_raise(ERR_LIB_ASN1, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }
    if ((a->type & ~V_ASN1_NEG) != itype) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_WRONG_INTEGER_TYPE);
        return 0;
    }
    return asn1_get_int64(pr, a->data, a->length, a->type & V_ASN1_NEG);
}

static int asn1_string_set_int64(ASN1_STRING *a, int64_t r, int itype)
{
    unsigned char tbuf[sizeof(r)];
    size_t off;

    a->type = itype;
    if (r < 0) {
        /* Most obvious '-r' triggers undefined behaviour for most
         * common INT64_MIN. Even though below '0 - (uint64_t)r' can
         * appear two's-complement centric, it does produce correct/
         * expected result even on one's-complement. This is because
         * cast to unsigned has to change bit pattern... */
        off = asn1_put_uint64(tbuf, 0 - (uint64_t)r);
        a->type |= V_ASN1_NEG;
    } else {
        off = asn1_put_uint64(tbuf, r);
        a->type &= ~V_ASN1_NEG;
    }
    return ASN1_STRING_set(a, tbuf + off, sizeof(tbuf) - off);
}

static int asn1_string_get_uint64(uint64_t *pr, const ASN1_STRING *a,
                                  int itype)
{
    if (a == NULL) {
        ERR_raise(ERR_LIB_ASN1, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }
    if ((a->type & ~V_ASN1_NEG) != itype) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_WRONG_INTEGER_TYPE);
