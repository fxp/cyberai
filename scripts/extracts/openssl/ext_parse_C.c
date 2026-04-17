/* OpenSSL 3.4.1 — extensions.c: tls_collect_extensions + tls_check_client_hello — L990-1120 */
{
    int ret = SSL_TLSEXT_ERR_NOACK;
    int altmp = SSL_AD_UNRECOGNIZED_NAME;
    SSL *ssl = SSL_CONNECTION_GET_SSL(s);
    SSL *ussl = SSL_CONNECTION_GET_USER_SSL(s);
    SSL_CTX *sctx = SSL_CONNECTION_GET_CTX(s);
    int was_ticket = (SSL_get_options(ssl) & SSL_OP_NO_TICKET) == 0;

    if (!ossl_assert(sctx != NULL) || !ossl_assert(s->session_ctx != NULL)) {
        SSLfatal(s, SSL_AD_INTERNAL_ERROR, ERR_R_INTERNAL_ERROR);
        return 0;
    }

    if (sctx->ext.servername_cb != NULL)
        ret = sctx->ext.servername_cb(ussl, &altmp,
                                      sctx->ext.servername_arg);
    else if (s->session_ctx->ext.servername_cb != NULL)
        ret = s->session_ctx->ext.servername_cb(ussl, &altmp,
                                                s->session_ctx->ext.servername_arg);

    /*
     * For servers, propagate the SNI hostname from the temporary
     * storage in the SSL to the persistent SSL_SESSION, now that we
     * know we accepted it.
     * Clients make this copy when parsing the server's response to
     * the extension, which is when they find out that the negotiation
     * was successful.
     */
    if (s->server) {
        if (sent && ret == SSL_TLSEXT_ERR_OK && !s->hit) {
            /* Only store the hostname in the session if we accepted it. */
            OPENSSL_free(s->session->ext.hostname);
            s->session->ext.hostname = OPENSSL_strdup(s->ext.hostname);
            if (s->session->ext.hostname == NULL && s->ext.hostname != NULL) {
                SSLfatal(s, SSL_AD_INTERNAL_ERROR, ERR_R_INTERNAL_ERROR);
            }
        }
    }

    /*
     * If we switched contexts (whether here or in the client_hello callback),
     * move the sess_accept increment from the session_ctx to the new
     * context, to avoid the confusing situation of having sess_accept_good
     * exceed sess_accept (zero) for the new context.
     */
    if (SSL_IS_FIRST_HANDSHAKE(s) && sctx != s->session_ctx
            && s->hello_retry_request == SSL_HRR_NONE) {
        ssl_tsan_counter(sctx, &sctx->stats.sess_accept);
        ssl_tsan_decr(s->session_ctx, &s->session_ctx->stats.sess_accept);
    }

    /*
     * If we're expecting to send a ticket, and tickets were previously enabled,
     * and now tickets are disabled, then turn off expected ticket.
     * Also, if this is not a resumption, create a new session ID
     */
    if (ret == SSL_TLSEXT_ERR_OK && s->ext.ticket_expected
            && was_ticket && (SSL_get_options(ssl) & SSL_OP_NO_TICKET) != 0) {
        s->ext.ticket_expected = 0;
        if (!s->hit) {
            SSL_SESSION* ss = SSL_get_session(ssl);

            if (ss != NULL) {
                OPENSSL_free(ss->ext.tick);
                ss->ext.tick = NULL;
                ss->ext.ticklen = 0;
                ss->ext.tick_lifetime_hint = 0;
                ss->ext.tick_age_add = 0;
                if (!ssl_generate_session_id(s, ss)) {
                    SSLfatal(s, SSL_AD_INTERNAL_ERROR, ERR_R_INTERNAL_ERROR);
                    return 0;
                }
            } else {
                SSLfatal(s, SSL_AD_INTERNAL_ERROR, ERR_R_INTERNAL_ERROR);
                return 0;
            }
        }
    }

    switch (ret) {
    case SSL_TLSEXT_ERR_ALERT_FATAL:
        SSLfatal(s, altmp, SSL_R_CALLBACK_FAILED);
        return 0;

    case SSL_TLSEXT_ERR_ALERT_WARNING:
        /* TLSv1.3 doesn't have warning alerts so we suppress this */
        if (!SSL_CONNECTION_IS_TLS13(s))
            ssl3_send_alert(s, SSL3_AL_WARNING, altmp);
        s->servername_done = 0;
        return 1;

    case SSL_TLSEXT_ERR_NOACK:
        s->servername_done = 0;
        return 1;

    default:
        return 1;
    }
}

static int final_ec_pt_formats(SSL_CONNECTION *s, unsigned int context,
                               int sent)
{
    unsigned long alg_k, alg_a;

    if (s->server)
        return 1;

    alg_k = s->s3.tmp.new_cipher->algorithm_mkey;
    alg_a = s->s3.tmp.new_cipher->algorithm_auth;

    /*
     * If we are client and using an elliptic curve cryptography cipher
     * suite, then if server returns an EC point formats lists extension it
     * must contain uncompressed.
     */
    if (s->ext.ecpointformats != NULL
            && s->ext.ecpointformats_len > 0
            && s->ext.peer_ecpointformats != NULL
            && s->ext.peer_ecpointformats_len > 0
            && ((alg_k & SSL_kECDHE) || (alg_a & SSL_aECDSA))) {
        /* we are using an ECC cipher */
        size_t i;
        unsigned char *list = s->ext.peer_ecpointformats;

        for (i = 0; i < s->ext.peer_ecpointformats_len; i++) {
            if (*list++ == TLSEXT_ECPOINTFORMAT_uncompressed)
                break;
        }
        if (i == s->ext.peer_ecpointformats_len) {
            SSLfatal(s, SSL_AD_ILLEGAL_PARAMETER,
