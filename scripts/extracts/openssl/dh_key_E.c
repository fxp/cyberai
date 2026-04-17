/* OpenSSL 3.4.1 — dh_key.c: generate_key: FIPS validation + BN ops — L310-405 */
    }
    if (generate_new_key) {
        /* Is it an approved safe prime ?*/
        if (DH_get_nid(dh) != NID_undef) {
            int max_strength =
                    ossl_ifc_ffc_compute_security_bits(BN_num_bits(dh->params.p));

            if (dh->params.q == NULL
                || dh->length > BN_num_bits(dh->params.q))
                goto err;
            /* dh->length = maximum bit length of generated private key */
            if (!ossl_ffc_generate_private_key(ctx, &dh->params, dh->length,
                                               max_strength, priv_key))
                goto err;
        } else {
#ifdef FIPS_MODULE
            if (dh->params.q == NULL)
                goto err;
#else
            if (dh->params.q == NULL) {
                /* secret exponent length, must satisfy 2^(l-1) <= p */
                if (dh->length != 0
                    && dh->length >= BN_num_bits(dh->params.p))
                    goto err;
                l = dh->length ? dh->length : BN_num_bits(dh->params.p) - 1;
                if (!BN_priv_rand_ex(priv_key, l, BN_RAND_TOP_ONE,
                                     BN_RAND_BOTTOM_ANY, 0, ctx))
                    goto err;
                /*
                 * We handle just one known case where g is a quadratic non-residue:
                 * for g = 2: p % 8 == 3
                 */
                if (BN_is_word(dh->params.g, DH_GENERATOR_2)
                    && !BN_is_bit_set(dh->params.p, 2)) {
                    /* clear bit 0, since it won't be a secret anyway */
                    if (!BN_clear_bit(priv_key, 0))
                        goto err;
                }
            } else
#endif
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
