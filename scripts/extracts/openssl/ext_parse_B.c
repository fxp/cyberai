/* OpenSSL 3.4.1 — extensions.c: tls_parse_all_extensions B: context check loop — L860-990 */
               /*
                * If extensions are of zero length then we don't even add the
                * extensions length bytes to a ClientHello/ServerHello
                * (for non-TLSv1.3).
                */
            || ((context &
                 (SSL_EXT_CLIENT_HELLO | SSL_EXT_TLS1_2_SERVER_HELLO)) != 0
                && !WPACKET_set_flags(pkt,
                                      WPACKET_FLAGS_ABANDON_ON_ZERO_LENGTH))) {
        if (!for_comp)
            SSLfatal(s, SSL_AD_INTERNAL_ERROR, ERR_R_INTERNAL_ERROR);
        return 0;
    }

    if ((context & SSL_EXT_CLIENT_HELLO) != 0) {
        reason = ssl_get_min_max_version(s, &min_version, &max_version, NULL);
        if (reason != 0) {
            if (!for_comp)
                SSLfatal(s, SSL_AD_INTERNAL_ERROR, reason);
            return 0;
        }
    }

    /* Add custom extensions first */
    if ((context & SSL_EXT_CLIENT_HELLO) != 0) {
        /* On the server side with initialise during ClientHello parsing */
        custom_ext_init(&s->cert->custext);
    }
    if (!custom_ext_add(s, context, pkt, x, chainidx, max_version)) {
        /* SSLfatal() already called */
        return 0;
    }

    for (i = 0, thisexd = ext_defs; i < OSSL_NELEM(ext_defs); i++, thisexd++) {
        EXT_RETURN (*construct)(SSL_CONNECTION *s, WPACKET *pkt,
                                unsigned int context,
                                X509 *x, size_t chainidx);
        EXT_RETURN ret;

        /* Skip if not relevant for our context */
        if (!should_add_extension(s, thisexd->context, context, max_version))
            continue;

        construct = s->server ? thisexd->construct_stoc
                              : thisexd->construct_ctos;

        if (construct == NULL)
            continue;

        ret = construct(s, pkt, context, x, chainidx);
        if (ret == EXT_RETURN_FAIL) {
            /* SSLfatal() already called */
            return 0;
        }
        if (ret == EXT_RETURN_SENT
                && (context & (SSL_EXT_CLIENT_HELLO
                               | SSL_EXT_TLS1_3_CERTIFICATE_REQUEST
                               | SSL_EXT_TLS1_3_NEW_SESSION_TICKET)) != 0)
            s->ext.extflags[i] |= SSL_EXT_FLAG_SENT;
    }

    if (!WPACKET_close(pkt)) {
        if (!for_comp)
            SSLfatal(s, SSL_AD_INTERNAL_ERROR, ERR_R_INTERNAL_ERROR);
        return 0;
    }

    return 1;
}

/*
 * Built in extension finalisation and initialisation functions. All initialise
 * or finalise the associated extension type for the given |context|. For
 * finalisers |sent| is set to 1 if we saw the extension during parsing, and 0
 * otherwise. These functions return 1 on success or 0 on failure.
 */

static int final_renegotiate(SSL_CONNECTION *s, unsigned int context, int sent)
{
    if (!s->server) {
        /*
         * Check if we can connect to a server that doesn't support safe
         * renegotiation
         */
        if (!(s->options & SSL_OP_LEGACY_SERVER_CONNECT)
                && !(s->options & SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION)
                && !sent) {
            SSLfatal(s, SSL_AD_HANDSHAKE_FAILURE,
                     SSL_R_UNSAFE_LEGACY_RENEGOTIATION_DISABLED);
            return 0;
        }

        return 1;
    }

    /* Need RI if renegotiating */
    if (s->renegotiate
            && !(s->options & SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION)
            && !sent) {
        SSLfatal(s, SSL_AD_HANDSHAKE_FAILURE,
                 SSL_R_UNSAFE_LEGACY_RENEGOTIATION_DISABLED);
        return 0;
    }


    return 1;
}

static ossl_inline void ssl_tsan_decr(const SSL_CTX *ctx,
                                      TSAN_QUALIFIER int *stat)
{
    if (ssl_tsan_lock(ctx)) {
        tsan_decr(stat);
        ssl_tsan_unlock(ctx);
    }
}

static int init_server_name(SSL_CONNECTION *s, unsigned int context)
{
    if (s->server) {
        s->servername_done = 0;

        OPENSSL_free(s->ext.hostname);
        s->ext.hostname = NULL;
    }

    return 1;
}

static int final_server_name(SSL_CONNECTION *s, unsigned int context, int sent)
{
