/* OpenSSL 3.4.1 — a_int.c: ASN1_INTEGER_set + to_bn conversions — L505-641 */
{
    BIGNUM *ret;

    if ((ai->type & ~V_ASN1_NEG) != itype) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_WRONG_INTEGER_TYPE);
        return NULL;
    }

    ret = BN_bin2bn(ai->data, ai->length, bn);
    if (ret == NULL) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_BN_LIB);
        return NULL;
    }
    if (ai->type & V_ASN1_NEG)
        BN_set_negative(ret, 1);
    return ret;
}

int ASN1_INTEGER_get_int64(int64_t *pr, const ASN1_INTEGER *a)
{
    return asn1_string_get_int64(pr, a, V_ASN1_INTEGER);
}

int ASN1_INTEGER_set_int64(ASN1_INTEGER *a, int64_t r)
{
    return asn1_string_set_int64(a, r, V_ASN1_INTEGER);
}

int ASN1_INTEGER_get_uint64(uint64_t *pr, const ASN1_INTEGER *a)
{
    return asn1_string_get_uint64(pr, a, V_ASN1_INTEGER);
}

int ASN1_INTEGER_set_uint64(ASN1_INTEGER *a, uint64_t r)
{
    return asn1_string_set_uint64(a, r, V_ASN1_INTEGER);
}

int ASN1_INTEGER_set(ASN1_INTEGER *a, long v)
{
    return ASN1_INTEGER_set_int64(a, v);
}

long ASN1_INTEGER_get(const ASN1_INTEGER *a)
{
    int i;
    int64_t r;
    if (a == NULL)
        return 0;
    i = ASN1_INTEGER_get_int64(&r, a);
    if (i == 0)
        return -1;
    if (r > LONG_MAX || r < LONG_MIN)
        return -1;
    return (long)r;
}

ASN1_INTEGER *BN_to_ASN1_INTEGER(const BIGNUM *bn, ASN1_INTEGER *ai)
{
    return bn_to_asn1_string(bn, ai, V_ASN1_INTEGER);
}

BIGNUM *ASN1_INTEGER_to_BN(const ASN1_INTEGER *ai, BIGNUM *bn)
{
    return asn1_string_to_bn(ai, bn, V_ASN1_INTEGER);
}

int ASN1_ENUMERATED_get_int64(int64_t *pr, const ASN1_ENUMERATED *a)
{
    return asn1_string_get_int64(pr, a, V_ASN1_ENUMERATED);
}

int ASN1_ENUMERATED_set_int64(ASN1_ENUMERATED *a, int64_t r)
{
    return asn1_string_set_int64(a, r, V_ASN1_ENUMERATED);
}

int ASN1_ENUMERATED_set(ASN1_ENUMERATED *a, long v)
{
    return ASN1_ENUMERATED_set_int64(a, v);
}

long ASN1_ENUMERATED_get(const ASN1_ENUMERATED *a)
{
    int i;
    int64_t r;
    if (a == NULL)
        return 0;
    if ((a->type & ~V_ASN1_NEG) != V_ASN1_ENUMERATED)
        return -1;
    if (a->length > (int)sizeof(long))
        return 0xffffffffL;
    i = ASN1_ENUMERATED_get_int64(&r, a);
    if (i == 0)
        return -1;
    if (r > LONG_MAX || r < LONG_MIN)
        return -1;
    return (long)r;
}

ASN1_ENUMERATED *BN_to_ASN1_ENUMERATED(const BIGNUM *bn, ASN1_ENUMERATED *ai)
{
    return bn_to_asn1_string(bn, ai, V_ASN1_ENUMERATED);
}

BIGNUM *ASN1_ENUMERATED_to_BN(const ASN1_ENUMERATED *ai, BIGNUM *bn)
{
    return asn1_string_to_bn(ai, bn, V_ASN1_ENUMERATED);
}

/* Internal functions used by x_int64.c */
int ossl_c2i_uint64_int(uint64_t *ret, int *neg,
                        const unsigned char **pp, long len)
{
    unsigned char buf[sizeof(uint64_t)];
    size_t buflen;

    buflen = c2i_ibuf(NULL, NULL, *pp, len);
    if (buflen == 0)
        return 0;
    if (buflen > sizeof(uint64_t)) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_TOO_LARGE);
        return 0;
    }
    (void)c2i_ibuf(buf, neg, *pp, len);
    return asn1_get_uint64(ret, buf, buflen);
}

int ossl_i2c_uint64_int(unsigned char *p, uint64_t r, int neg)
{
    unsigned char buf[sizeof(uint64_t)];
    size_t off;

    off = asn1_put_uint64(buf, r);
    return i2c_ibuf(buf + off, sizeof(buf) - off, neg, &p);
}

