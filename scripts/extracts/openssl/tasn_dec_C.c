/* OpenSSL 3.4.1 — tasn_dec.c: asn1_item_embed_d2i B: SET/CHOICE decode — L220-340 */
    case ASN1_ITYPE_MSTRING:
        /*
         * It never makes sense for multi-strings to have implicit tagging, so
         * if tag != -1, then this looks like an error in the template.
         */
        if (tag != -1) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_BAD_TEMPLATE);
            goto err;
        }

        p = *in;
        /* Just read in tag and class */
        ret = asn1_check_tlen(NULL, &otag, &oclass, NULL, NULL,
                              &p, len, -1, 0, 1, ctx);
        if (!ret) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
            goto err;
        }

        /* Must be UNIVERSAL class */
        if (oclass != V_ASN1_UNIVERSAL) {
            /* If OPTIONAL, assume this is OK */
            if (opt)
                return -1;
            ERR_raise(ERR_LIB_ASN1, ASN1_R_MSTRING_NOT_UNIVERSAL);
            goto err;
        }

        /* Check tag matches bit map */
        if (!(ASN1_tag2bit(otag) & it->utype)) {
            /* If OPTIONAL, assume this is OK */
            if (opt)
                return -1;
            ERR_raise(ERR_LIB_ASN1, ASN1_R_MSTRING_WRONG_TAG);
            goto err;
        }
        return asn1_d2i_ex_primitive(pval, in, len, it, otag, 0, 0, ctx);

    case ASN1_ITYPE_EXTERN:
        /* Use new style d2i */
        ef = it->funcs;
        if (ef->asn1_ex_d2i_ex != NULL)
            return ef->asn1_ex_d2i_ex(pval, in, len, it, tag, aclass, opt, ctx,
                                      libctx, propq);
        return ef->asn1_ex_d2i(pval, in, len, it, tag, aclass, opt, ctx);

    case ASN1_ITYPE_CHOICE:
        /*
         * It never makes sense for CHOICE types to have implicit tagging, so
         * if tag != -1, then this looks like an error in the template.
         */
        if (tag != -1) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_BAD_TEMPLATE);
            goto err;
        }

        if (asn1_cb && !asn1_cb(ASN1_OP_D2I_PRE, pval, it, NULL))
            goto auxerr;
        if (*pval) {
            /* Free up and zero CHOICE value if initialised */
            i = ossl_asn1_get_choice_selector(pval, it);
            if ((i >= 0) && (i < it->tcount)) {
                tt = it->templates + i;
                pchptr = ossl_asn1_get_field_ptr(pval, tt);
                ossl_asn1_template_free(pchptr, tt);
                ossl_asn1_set_choice_selector(pval, -1, it);
            }
        } else if (!ossl_asn1_item_ex_new_intern(pval, it, libctx, propq)) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
            goto err;
        }
        /* CHOICE type, try each possibility in turn */
        p = *in;
        for (i = 0, tt = it->templates; i < it->tcount; i++, tt++) {
            pchptr = ossl_asn1_get_field_ptr(pval, tt);
            /*
             * We mark field as OPTIONAL so its absence can be recognised.
             */
            ret = asn1_template_ex_d2i(pchptr, &p, len, tt, 1, ctx, depth,
                                       libctx, propq);
            /* If field not present, try the next one */
            if (ret == -1)
                continue;
            /* If positive return, read OK, break loop */
            if (ret > 0)
                break;
            /*
             * Must be an ASN1 parsing error.
             * Free up any partial choice value
             */
            ossl_asn1_template_free(pchptr, tt);
            errtt = tt;
            ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
            goto err;
        }

        /* Did we fall off the end without reading anything? */
        if (i == it->tcount) {
            /* If OPTIONAL, this is OK */
            if (opt) {
                /* Free and zero it */
                ASN1_item_ex_free(pval, it);
                return -1;
            }
            ERR_raise(ERR_LIB_ASN1, ASN1_R_NO_MATCHING_CHOICE_TYPE);
            goto err;
        }

        ossl_asn1_set_choice_selector(pval, i, it);

        if (asn1_cb && !asn1_cb(ASN1_OP_D2I_POST, pval, it, NULL))
            goto auxerr;
        *in = p;
        return 1;

    case ASN1_ITYPE_NDEF_SEQUENCE:
    case ASN1_ITYPE_SEQUENCE:
        p = *in;
        tmplen = len;

        /* If no IMPLICIT tagging set to SEQUENCE, UNIVERSAL */
