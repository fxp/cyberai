/* OpenSSL 3.4.1 — dh_key.c: generate_key continuation + parameter checks — L350-458 */
            {
                /* Do a partial check for invalid p, q, g */
                if (!ossl_ffc_params_simple_validate(dh->libctx, &dh->params,
                                                     FFC_PARAM_TYPE_DH, NULL))
                    goto err;
                /*
                 * For FFC FIPS 186-4 keygen
                 * security strength s = 112,
                 * Max Private key size N = len(q)
                 */
                if (!ossl_ffc_generate_private_key(ctx, &dh->params,
                                                   BN_num_bits(dh->params.q),
                                                   MIN_STRENGTH,
                                                   priv_key))
                    goto err;
            }
        }
    }

    if (!ossl_dh_generate_public_key(ctx, dh, priv_key, pub_key))
        goto err;

    dh->pub_key = pub_key;
    dh->priv_key = priv_key;
    dh->dirty_cnt++;
    ok = 1;
 err:
    if (ok != 1)
        ERR_raise(ERR_LIB_DH, ERR_R_BN_LIB);

    if (pub_key != dh->pub_key)
        BN_free(pub_key);
    if (priv_key != dh->priv_key)
        BN_free(priv_key);
    BN_CTX_free(ctx);
    return ok;
}

int ossl_dh_buf2key(DH *dh, const unsigned char *buf, size_t len)
{
    int err_reason = DH_R_BN_ERROR;
    BIGNUM *pubkey = NULL;
    const BIGNUM *p;
    int ret;

    if ((pubkey = BN_bin2bn(buf, len, NULL)) == NULL)
        goto err;
    DH_get0_pqg(dh, &p, NULL, NULL);
    if (p == NULL || BN_num_bytes(p) == 0) {
        err_reason = DH_R_NO_PARAMETERS_SET;
        goto err;
    }
    /* Prevent small subgroup attacks per RFC 8446 Section 4.2.8.1 */
    if (!ossl_dh_check_pub_key_partial(dh, pubkey, &ret)) {
        err_reason = DH_R_INVALID_PUBKEY;
        goto err;
    }
    if (DH_set0_key(dh, pubkey, NULL) != 1)
        goto err;
    return 1;
err:
    ERR_raise(ERR_LIB_DH, err_reason);
    BN_free(pubkey);
    return 0;
}

size_t ossl_dh_key2buf(const DH *dh, unsigned char **pbuf_out, size_t size,
                       int alloc)
{
    const BIGNUM *pubkey;
    unsigned char *pbuf = NULL;
    const BIGNUM *p;
    int p_size;

    DH_get0_pqg(dh, &p, NULL, NULL);
    DH_get0_key(dh, &pubkey, NULL);
    if (p == NULL || pubkey == NULL
            || (p_size = BN_num_bytes(p)) == 0
            || BN_num_bytes(pubkey) == 0) {
        ERR_raise(ERR_LIB_DH, DH_R_INVALID_PUBKEY);
        return 0;
    }
    if (pbuf_out != NULL && (alloc || *pbuf_out != NULL)) {
        if (!alloc) {
            if (size >= (size_t)p_size)
                pbuf = *pbuf_out;
            if (pbuf == NULL)
                ERR_raise(ERR_LIB_DH, DH_R_INVALID_SIZE);
        } else {
            pbuf = OPENSSL_malloc(p_size);
        }

        /* Errors raised above */
        if (pbuf == NULL)
            return 0;
        /*
         * As per Section 4.2.8.1 of RFC 8446 left pad public
         * key with zeros to the size of p
         */
        if (BN_bn2binpad(pubkey, pbuf, p_size) < 0) {
            if (alloc)
                OPENSSL_free(pbuf);
            ERR_raise(ERR_LIB_DH, DH_R_BN_ERROR);
            return 0;
        }
        *pbuf_out = pbuf;
    }
    return p_size;
}
