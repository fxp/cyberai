/* OpenSSL 3.4.1 — tasn_dec.c: asn1_check_tlen + collect_data — L1110-1228 */
            return 0;
        }
        memcpy(buf->data + len, *p, plen);
    }
    *p += plen;
    return 1;
}

/* Check for ASN1 EOC and swallow it if found */

static int asn1_check_eoc(const unsigned char **in, long len)
{
    const unsigned char *p;

    if (len < 2)
        return 0;
    p = *in;
    if (p[0] == '\0' && p[1] == '\0') {
        *in += 2;
        return 1;
    }
    return 0;
}

/*
 * Check an ASN1 tag and length: a bit like ASN1_get_object but it sets the
 * length for indefinite length constructed form, we don't know the exact
 * length but we can set an upper bound to the amount of data available minus
 * the header length just read.
 */

static int asn1_check_tlen(long *olen, int *otag, unsigned char *oclass,
                           char *inf, char *cst,
                           const unsigned char **in, long len,
                           int exptag, int expclass, char opt, ASN1_TLC *ctx)
{
    int i;
    int ptag, pclass;
    long plen;
    const unsigned char *p, *q;
    p = *in;
    q = p;

    if (len <= 0) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_TOO_SMALL);
        goto err;
    }
    if (ctx != NULL && ctx->valid) {
        i = ctx->ret;
        plen = ctx->plen;
        pclass = ctx->pclass;
        ptag = ctx->ptag;
        p += ctx->hdrlen;
    } else {
        i = ASN1_get_object(&p, &plen, &ptag, &pclass, len);
        if (ctx != NULL) {
            ctx->ret = i;
            ctx->plen = plen;
            ctx->pclass = pclass;
            ctx->ptag = ptag;
            ctx->hdrlen = p - q;
            ctx->valid = 1;
            /*
             * If definite length, and no error, length + header can't exceed
             * total amount of data available.
             */
            if ((i & 0x81) == 0 && (plen + ctx->hdrlen) > len) {
                ERR_raise(ERR_LIB_ASN1, ASN1_R_TOO_LONG);
                goto err;
            }
        }
    }

    if ((i & 0x80) != 0) {
        ERR_raise(ERR_LIB_ASN1, ASN1_R_BAD_OBJECT_HEADER);
        goto err;
    }
    if (exptag >= 0) {
        if (exptag != ptag || expclass != pclass) {
            /*
             * If type is OPTIONAL, not an error: indicate missing type.
             */
            if (opt != 0)
                return -1;
            ERR_raise(ERR_LIB_ASN1, ASN1_R_WRONG_TAG);
            goto err;
        }
        /*
         * We have a tag and class match: assume we are going to do something
         * with it
         */
        asn1_tlc_clear(ctx);
    }

    if ((i & 1) != 0)
        plen = len - (p - q);

    if (inf != NULL)
        *inf = i & 1;

    if (cst != NULL)
        *cst = i & V_ASN1_CONSTRUCTED;

    if (olen != NULL)
        *olen = plen;

    if (oclass != NULL)
        *oclass = pclass;

    if (otag != NULL)
        *otag = ptag;

    *in = p;
    return 1;

 err:
    asn1_tlc_clear(ctx);
    return 0;
}
