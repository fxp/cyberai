/* OpenSSL 3.4.1 — tasn_dec.c: asn1_item_embed_d2i C: template match+length — L340-440 */
        /* If no IMPLICIT tagging set to SEQUENCE, UNIVERSAL */
        if (tag == -1) {
            tag = V_ASN1_SEQUENCE;
            aclass = V_ASN1_UNIVERSAL;
        }
        /* Get SEQUENCE length and update len, p */
        ret = asn1_check_tlen(&len, NULL, NULL, &seq_eoc, &cst,
                              &p, len, tag, aclass, opt, ctx);
        if (!ret) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
            goto err;
        } else if (ret == -1)
            return -1;
        if (aux && (aux->flags & ASN1_AFLG_BROKEN)) {
            len = tmplen - (p - *in);
            seq_nolen = 1;
        }
        /* If indefinite we don't do a length check */
        else
            seq_nolen = seq_eoc;
        if (!cst) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_SEQUENCE_NOT_CONSTRUCTED);
            goto err;
        }

        if (*pval == NULL
                && !ossl_asn1_item_ex_new_intern(pval, it, libctx, propq)) {
            ERR_raise(ERR_LIB_ASN1, ERR_R_NESTED_ASN1_ERROR);
            goto err;
        }

        if (asn1_cb && !asn1_cb(ASN1_OP_D2I_PRE, pval, it, NULL))
            goto auxerr;

        /* Free up and zero any ADB found */
        for (i = 0, tt = it->templates; i < it->tcount; i++, tt++) {
            if (tt->flags & ASN1_TFLG_ADB_MASK) {
                const ASN1_TEMPLATE *seqtt;
                ASN1_VALUE **pseqval;
                seqtt = ossl_asn1_do_adb(*pval, tt, 0);
                if (seqtt == NULL)
                    continue;
                pseqval = ossl_asn1_get_field_ptr(pval, seqtt);
                ossl_asn1_template_free(pseqval, seqtt);
            }
        }

        /* Get each field entry */
        for (i = 0, tt = it->templates; i < it->tcount; i++, tt++) {
            const ASN1_TEMPLATE *seqtt;
            ASN1_VALUE **pseqval;
            seqtt = ossl_asn1_do_adb(*pval, tt, 1);
            if (seqtt == NULL)
                goto err;
            pseqval = ossl_asn1_get_field_ptr(pval, seqtt);
            /* Have we ran out of data? */
            if (!len)
                break;
            q = p;
            if (asn1_check_eoc(&p, len)) {
                if (!seq_eoc) {
                    ERR_raise(ERR_LIB_ASN1, ASN1_R_UNEXPECTED_EOC);
                    goto err;
                }
                len -= p - q;
                seq_eoc = 0;
                break;
            }
            /*
             * This determines the OPTIONAL flag value. The field cannot be
             * omitted if it is the last of a SEQUENCE and there is still
             * data to be read. This isn't strictly necessary but it
             * increases efficiency in some cases.
             */
            if (i == (it->tcount - 1))
                isopt = 0;
            else
                isopt = (char)(seqtt->flags & ASN1_TFLG_OPTIONAL);
            /*
             * attempt to read in field, allowing each to be OPTIONAL
             */

            ret = asn1_template_ex_d2i(pseqval, &p, len, seqtt, isopt, ctx,
                                       depth, libctx, propq);
            if (!ret) {
                errtt = seqtt;
                goto err;
            } else if (ret == -1) {
                /*
                 * OPTIONAL component absent. Free and zero the field.
                 */
                ossl_asn1_template_free(pseqval, seqtt);
                continue;
            }
            /* Update length */
            len -= p - q;
        }

        /* Check for EOC if expecting one */
        if (seq_eoc && !asn1_check_eoc(&p, len)) {
            ERR_raise(ERR_LIB_ASN1, ASN1_R_MISSING_EOC);
