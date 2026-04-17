{
    int rc = LIBSSH2_ERROR_DECRYPT;
    LIBSSH2_PRIVKEY_SK *sk_info = (LIBSSH2_PRIVKEY_SK *) (*abstract);
    LIBSSH2_SK_SIG_INFO sig_info = { 0 };

    if(sk_info->handle_len <= 0) {
        return LIBSSH2_ERROR_DECRYPT;
    }

    rc = sk_info->sign_callback(session,
                                &sig_info,
                                data,
                                data_len,
                                sk_info->algorithm,
                                sk_info->flags,
                                sk_info->application,
                                sk_info->key_handle,
                                sk_info->handle_len,
                                sk_info->orig_abstract);

    if(rc == 0 && sig_info.sig_r_len > 0 && sig_info.sig_r) {
        unsigned char *p = NULL;

        if(sig_info.sig_s_len > 0 && sig_info.sig_s) {
            /* sig length, sig_r, sig_s, flags, counter, plus 4 bytes for each
               component's length, and up to 1 extra byte for each component */
            *sig_len = 4 + 5 + sig_info.sig_r_len + 5 + sig_info.sig_s_len + 5;
            *sig = LIBSSH2_ALLOC(session, *sig_len);

            if(*sig) {
                unsigned char *x = *sig;
                p = *sig;

                _libssh2_store_u32(&p, 0);

                _libssh2_store_bignum2_bytes(&p,
                                             sig_info.sig_r,
                                             sig_info.sig_r_len);

                _libssh2_store_bignum2_bytes(&p,
                                             sig_info.sig_s,
                                             sig_info.sig_s_len);

                *sig_len = p - *sig;

                _libssh2_store_u32(&x, (uint32_t)(*sig_len - 4));
            }
            else {
                _libssh2_debug((session,
                               LIBSSH2_ERROR_ALLOC,
                               "Unable to allocate ecdsa-sk signature."));
                rc = LIBSSH2_ERROR_ALLOC;
            }
        }
        else {
            /* sig, flags, counter, plus 4 bytes for sig length. */
            *sig_len = 4 + sig_info.sig_r_len + 1 + 4;
            *sig = LIBSSH2_ALLOC(session, *sig_len);

            if(*sig) {
                p = *sig;

                _libssh2_store_str(&p,
                                   (const char *)sig_info.sig_r,
                                   sig_info.sig_r_len);
            }
            else {
                _libssh2_debug((session,
                               LIBSSH2_ERROR_ALLOC,
                               "Unable to allocate ed25519-sk signature."));
                rc = LIBSSH2_ERROR_ALLOC;
            }
        }

        if(p) {
            *p = sig_info.flags;
            ++p;
            _libssh2_store_u32(&p, sig_info.counter);

            *sig_len = p - *sig;
        }

        LIBSSH2_FREE(session, sig_info.sig_r);

        if(sig_info.sig_s) {
            LIBSSH2_FREE(session, sig_info.sig_s);
        }
    }
    else {
        _libssh2_debug((session,
                       LIBSSH2_ERROR_DECRYPT,
                       "sign_callback failed or returned invalid signature."));
        *sig_len = 0;
    }

    return rc;
