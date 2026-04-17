/* OpenSSL 3.4.1 — tasn_dec.c: asn1_template_ex_d2i + asn1_template_noexp_d2i — L570-700 */
                                   const ASN1_TEMPLATE *tt, char opt,
                                   ASN1_TLC *ctx, int depth,
                                   OSSL_LIB_CTX *libctx, const char *propq)
{
    int flags, aclass;
    int ret;
    ASN1_VALUE *tval;
    const unsigned char *p, *q;
    if (!val)
        return 0;
    flags = tt->flags;
    aclass = flags & ASN1_TFLG_TAG_CLASS;

    p = *in;

    /*
     * If field is embedded then val needs fixing so it is a pointer to
     * a pointer to a field.
     */
    if (tt->flags & ASN1_TFLG_EMBED) {
        tval = (ASN1_VALUE *)val;
        val = &tval;
    }

    if (flags & ASN1_TFLG_SK_MASK) {
        /* SET OF, SEQUENCE OF */
        int sktag, skaclass;
        char sk_eoc;
        /* First work out expected inner tag value */
        if (flags & ASN1_TFLG_IMPTAG) {
            sktag = tt->tag;
            skaclass = aclass;
        } else {
            skaclass = V_ASN1_UNIVERSAL;
            if (flags & ASN1_TFLG_SET_OF)
                sktag = V_ASN1_SET;
            else
                sktag = V_ASN1_SEQUENCE;
        }
        /* Get the tag */
        ret = asn1_check_tlen(&len, NULL, NULL, &sk_eoc, NULL,
                              &p, len, sktag, skaclass, opt, ctx);
        if (!ret) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
            return 0;
        } else if (ret == -1)
            return -1;
        if (*val == NULL)
            *val = (ASN1_VALUE *)sk_ASN1_VALUE_new_null();
        else {
            /*
             * We've got a valid STACK: free up any items present
             */
            STACK_OF(ASN1_VALUE) *sktmp = (STACK_OF(ASN1_VALUE) *)*val;
            ASN1_VALUE *vtmp;
            while (sk_ASN1_VALUE_num(sktmp) > 0) {
                vtmp = sk_ASN1_VALUE_pop(sktmp);
                ASN1_item_ex_free(&vtmp, ASN1_ITEM_ptr(tt->item));
            }
        }

        if (*val == NULL) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_CRYPTO_LIB);
            goto err;
        }

        /* Read as many items as we can */
        while (len > 0) {
            ASN1_VALUE *skfield;
            q = p;
            /* See if EOC found */
            if (asn1_check_eoc(&p, len)) {
                if (!sk_eoc) {
                    ERR_raise(ERR_LIB_ASN1, ASN1_R_UNEXPECTED_EOC);
                    goto err;
                }
                len -= p - q;
                sk_eoc = 0;
                break;
            }
            skfield = NULL;
            if (asn1_item_embed_d2i(&skfield, &p, len,
                                     ASN1_ITEM_ptr(tt->item), -1, 0, 0, ctx,
                                     depth, libctx, propq) <= 0) {
                ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
                /* |skfield| may be partially allocated despite failure. */
                ASN1_item_free(skfield, ASN1_ITEM_ptr(tt->item));
                goto err;
            }
            len -= p - q;
            if (!sk_ASN1_VALUE_push((STACK_OF(ASN1_VALUE) *)*val, skfield)) {
                ERR_raise(ERR_LIB_ASN1, ERR_R_CRYPTO_LIB);
                ASN1_item_free(skfield, ASN1_ITEM_ptr(tt->item));
                goto err;
            }
        }
        if (sk_eoc) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_MISSING_EOC);
            goto err;
        }
    } else if (flags & ASN1_TFLG_IMPTAG) {
        /* IMPLICIT tagging */
        ret = asn1_item_embed_d2i(val, &p, len,
                                  ASN1_ITEM_ptr(tt->item), tt->tag, aclass, opt,
                                  ctx, depth, libctx, propq);
        if (!ret) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
            goto err;
        } else if (ret == -1)
            return -1;
    } else {
        /* Nothing special */
        ret = asn1_item_embed_d2i(val, &p, len, ASN1_ITEM_ptr(tt->item),
                                  -1, 0, opt, ctx, depth, libctx, propq);
        if (!ret) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
            goto err;
        } else if (ret == -1)
            return -1;
    }

    *in = p;
    return 1;

 err:
    return 0;
}

static int asn1_d2i_ex_primitive(ASN1_VALUE **pval,
                                 const unsigned char **in, long inlen,
                                 const ASN1_ITEM *it,
