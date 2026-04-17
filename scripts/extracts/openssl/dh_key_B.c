/* OpenSSL 3.4.1 — dh_key.c: DH_compute_key body + public key validation — L120-265 */
#ifdef FIPS_MODULE
    ret = ossl_dh_compute_key(key, pub_key, dh);
#else
    ret = dh->meth->compute_key(key, pub_key, dh);
#endif
    if (ret <= 0)
        return ret;

    /* count leading zero bytes, yet still touch all bytes */
    for (i = 0; i < ret; i++) {
        mask &= !key[i];
        npad += mask;
    }

    /* unpad key */
    ret -= npad;
    /* key-dependent memory access, potentially leaking npad / ret */
    memmove(key, key + npad, ret);
    /* key-dependent memory access, potentially leaking npad / ret */
    memset(key + ret, 0, npad);

    return ret;
}

int DH_compute_key_padded(unsigned char *key, const BIGNUM *pub_key, DH *dh)
{
    int rv, pad;

    /* rv is constant unless compute_key is external */
#ifdef FIPS_MODULE
    rv = ossl_dh_compute_key(key, pub_key, dh);
#else
    rv = dh->meth->compute_key(key, pub_key, dh);
#endif
    if (rv <= 0)
        return rv;
    pad = BN_num_bytes(dh->params.p) - rv;
    /* pad is constant (zero) unless compute_key is external */
    if (pad > 0) {
        memmove(key + pad, key, rv);
        memset(key, 0, pad);
    }
    return rv + pad;
}

static DH_METHOD dh_ossl = {
    "OpenSSL DH Method",
    generate_key,
    ossl_dh_compute_key,
    dh_bn_mod_exp,
    dh_init,
    dh_finish,
    DH_FLAG_FIPS_METHOD,
    NULL,
    NULL
};

static const DH_METHOD *default_DH_method = &dh_ossl;

const DH_METHOD *DH_OpenSSL(void)
{
    return &dh_ossl;
}

const DH_METHOD *DH_get_default_method(void)
{
    return default_DH_method;
}

static int dh_bn_mod_exp(const DH *dh, BIGNUM *r,
                         const BIGNUM *a, const BIGNUM *p,
                         const BIGNUM *m, BN_CTX *ctx, BN_MONT_CTX *m_ctx)
{
#ifdef S390X_MOD_EXP
    return s390x_mod_exp(r, a, p, m, ctx, m_ctx);
#else
    return BN_mod_exp_mont(r, a, p, m, ctx, m_ctx);
#endif
}

static int dh_init(DH *dh)
{
    dh->flags |= DH_FLAG_CACHE_MONT_P;
    dh->dirty_cnt++;
    return 1;
}

static int dh_finish(DH *dh)
{
    BN_MONT_CTX_free(dh->method_mont_p);
    return 1;
}

#ifndef FIPS_MODULE
void DH_set_default_method(const DH_METHOD *meth)
{
    default_DH_method = meth;
}
#endif /* FIPS_MODULE */

int DH_generate_key(DH *dh)
{
#ifdef FIPS_MODULE
    return generate_key(dh);
#else
    return dh->meth->generate_key(dh);
#endif
}

int ossl_dh_generate_public_key(BN_CTX *ctx, const DH *dh,
                                const BIGNUM *priv_key, BIGNUM *pub_key)
{
    int ret = 0;
    BIGNUM *prk = BN_new();
    BN_MONT_CTX *mont = NULL;

    if (prk == NULL)
        return 0;

    if (dh->flags & DH_FLAG_CACHE_MONT_P) {
        /*
         * We take the input DH as const, but we lie, because in some cases we
         * want to get a hold of its Montgomery context.
         *
         * We cast to remove the const qualifier in this case, it should be
         * fine...
         */
        BN_MONT_CTX **pmont = (BN_MONT_CTX **)&dh->method_mont_p;

        mont = BN_MONT_CTX_set_locked(pmont, dh->lock, dh->params.p, ctx);
        if (mont == NULL)
            goto err;
    }
    BN_with_flags(prk, priv_key, BN_FLG_CONSTTIME);

    /* pub_key = g^priv_key mod p */
    if (!dh->meth->bn_mod_exp(dh, pub_key, dh->params.g, prk, dh->params.p,
                              ctx, mont))
        goto err;
    ret = 1;
err:
    BN_clear_free(prk);
    return ret;
}

static int generate_key(DH *dh)
