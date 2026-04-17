/* OpenSSL 3.4.1 — a_int.c: asn1_string_get/set_uint64 + i2c_ASN1_INTEGER — L370-510 */
        ERR_raise(ERR_LIB_ASN1, ASN1_R_WRONG_INTEGER_TYPE);
        return 0;
    }
    if (a->type & V_ASN1_NEG) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_ILLEGAL_NEGATIVE_VALUE);
        return 0;
    }
    return asn1_get_uint64(pr, a->data, a->length);
}

static int asn1_string_set_uint64(ASN1_STRING *a, uint64_t r, int itype)
{
    unsigned char tbuf[sizeof(r)];
    size_t off;

    a->type = itype;
    off = asn1_put_uint64(tbuf, r);
    return ASN1_STRING_set(a, tbuf + off, sizeof(tbuf) - off);
}

/*
 * This is a version of d2i_ASN1_INTEGER that ignores the sign bit of ASN1
 * integers: some broken software can encode a positive INTEGER with its MSB
 * set as negative (it doesn't add a padding zero).
 */

ASN1_INTEGER *d2i_ASN1_UINTEGER(ASN1_INTEGER **a, const unsigned char **pp,
                                long length)
{
    ASN1_INTEGER *ret = NULL;
    const unsigned char *p;
    unsigned char *s;
    long len = 0;
    int inf, tag, xclass;
    int i = 0;

    if ((a == NULL) || ((*a) == NULL)) {
        if ((ret = ASN1_INTEGER_new()) == NULL)
            return NULL;
        ret->type = V_ASN1_INTEGER;
    } else
        ret = (*a);

    p = *pp;
    inf = ASN1_get_object(&p, &len, &tag, &xclass, length);
    if (inf & 0x80) {
        i = ASN1_R_BAD_OBJECT_HEADER;
        goto err;
    }

    if (tag != V_ASN1_INTEGER) {
        i = ASN1_R_EXPECTING_AN_INTEGER;
        goto err;
    }

    if (len < 0) {
        i = ASN1_R_ILLEGAL_NEGATIVE_VALUE;
        goto err;
    }
    /*
     * We must OPENSSL_malloc stuff, even for 0 bytes otherwise it signifies
     * a missing NULL parameter.
     */
    s = OPENSSL_malloc((int)len + 1);
    if (s == NULL)
        goto err;
    ret->type = V_ASN1_INTEGER;
    if (len) {
        if ((*p == 0) && (len != 1)) {
            p++;
            len--;
        }
        memcpy(s, p, (int)len);
        p += len;
    }

    ASN1_STRING_set0(ret, s, (int)len);
    if (a != NULL)
        (*a) = ret;
    *pp = p;
    return ret;
 err:
    if (i != 0)
        ERR_raise(ERR_LIB_ASN1, i);
    if ((a == NULL) || (*a != ret))
        ASN1_INTEGER_free(ret);
    return NULL;
}

static ASN1_STRING *bn_to_asn1_string(const BIGNUM *bn, ASN1_STRING *ai,
                                      int atype)
{
    ASN1_INTEGER *ret;
    int len;

    if (ai == NULL) {
        ret = ASN1_STRING_type_new(atype);
    } else {
        ret = ai;
        ret->type = atype;
    }

    if (ret == NULL) {
        ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
        goto err;
    }

    if (BN_is_negative(bn) && !BN_is_zero(bn))
        ret->type |= V_ASN1_NEG_INTEGER;

    len = BN_num_bytes(bn);

    if (len == 0)
        len = 1;

    if (ASN1_STRING_set(ret, NULL, len) == 0) {
        ERR_raise(ERR_LIB_ASN1, ERR_R_ASN1_LIB);
        goto err;
    }

    /* Correct zero case */
    if (BN_is_zero(bn))
        ret->data[0] = 0;
    else
        len = BN_bn2bin(bn, ret->data);
    ret->length = len;
    return ret;
 err:
    if (ret != ai)
        ASN1_INTEGER_free(ret);
    return NULL;
}

static BIGNUM *asn1_string_to_bn(const ASN1_INTEGER *ai, BIGNUM *bn,
                                 int itype)
{
    BIGNUM *ret;

    if ((ai->type & ~V_ASN1_NEG) != itype) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_WRONG_INTEGER_TYPE);
        return NULL;
