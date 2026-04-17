/* OpenSSL 3.4.1 — tasn_dec.c: asn1_item_embed_d2i D: EXPLICIT+primitive types — L440-570 */
            ERR_raise(ERR_LIB_ASN1, ASN1_R_MISSING_EOC);
            goto err;
        }
        /* Check all data read */
        if (!seq_nolen && len) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_SEQUENCE_LENGTH_MISMATCH);
            goto err;
        }

        /*
         * If we get here we've got no more data in the SEQUENCE, however we
         * may not have read all fields so check all remaining are OPTIONAL
         * and clear any that are.
         */
        for (; i < it->tcount; tt++, i++) {
            const ASN1_TEMPLATE *seqtt;
            seqtt = ossl_asn1_do_adb(*pval, tt, 1);
            if (seqtt == NULL)
                goto err;
            if (seqtt->flags & ASN1_TFLG_OPTIONAL) {
                ASN1_VALUE **pseqval;
                pseqval = ossl_asn1_get_field_ptr(pval, seqtt);
                ossl_asn1_template_free(pseqval, seqtt);
            } else {
                errtt = seqtt;
                ERR_raise(ERR_LIB_ASN1, ASN1_R_FIELD_MISSING);
                goto err;
            }
        }
        /* Save encoding */
        if (!ossl_asn1_enc_save(pval, *in, p - *in, it))
            goto auxerr;
        if (asn1_cb && !asn1_cb(ASN1_OP_D2I_POST, pval, it, NULL))
            goto auxerr;
        *in = p;
        return 1;

    default:
        return 0;
    }
 auxerr:
    ERR_raise(ERR_LIB_ASN1, ASN1_R_AUX_ERROR);
 err:
    if (errtt)
        ERR_add_error_data(4, "Field=", errtt->field_name,
                           ", Type=", it->sname);
    else
        ERR_add_error_data(2, "Type=", it->sname);
    return 0;
}

/*
 * Templates are handled with two separate functions. One handles any
 * EXPLICIT tag and the other handles the rest.
 */

static int asn1_template_ex_d2i(ASN1_VALUE **val,
                                const unsigned char **in, long inlen,
                                const ASN1_TEMPLATE *tt, char opt,
                                ASN1_TLC *ctx, int depth,
                                OSSL_LIB_CTX *libctx, const char *propq)
{
    int flags, aclass;
    int ret;
    long len;
    const unsigned char *p, *q;
    char exp_eoc;
    if (!val)
        return 0;
    flags = tt->flags;
    aclass = flags & ASN1_TFLG_TAG_CLASS;

    p = *in;

    /* Check if EXPLICIT tag expected */
    if (flags & ASN1_TFLG_EXPTAG) {
        char cst;
        /*
         * Need to work out amount of data available to the inner content and
         * where it starts: so read in EXPLICIT header to get the info.
         */
        ret = asn1_check_tlen(&len, NULL, NULL, &exp_eoc, &cst,
                              &p, inlen, tt->tag, aclass, opt, ctx);
        q = p;
        if (!ret) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
            return 0;
        } else if (ret == -1)
            return -1;
        if (!cst) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_EXPLICIT_TAG_NOT_CONSTRUCTED);
            return 0;
        }
        /* We've found the field so it can't be OPTIONAL now */
        ret = asn1_template_noexp_d2i(val, &p, len, tt, 0, ctx, depth, libctx,
                                      propq);
        if (!ret) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
            return 0;
        }
        /* We read the field in OK so update length */
        len -= p - q;
        if (exp_eoc) {
            /* If NDEF we must have an EOC here */
            if (!asn1_check_eoc(&p, len)) {
                ERR_raise(ERR_LIB_ASN1, ASN1_R_MISSING_EOC);
                goto err;
            }
        } else {
            /*
             * Otherwise we must hit the EXPLICIT tag end or its an error
             */
            if (len) {
                ERR_raise(ERR_LIB_ASN1, ASN1_R_EXPLICIT_LENGTH_MISMATCH);
                goto err;
            }
        }
    } else
        return asn1_template_noexp_d2i(val, in, inlen, tt, opt, ctx, depth,
                                       libctx, propq);

    *in = p;
    return 1;

 err:
    return 0;
}

static int asn1_template_noexp_d2i(ASN1_VALUE **val,
                                   const unsigned char **in, long len,
                                   const ASN1_TEMPLATE *tt, char opt,
