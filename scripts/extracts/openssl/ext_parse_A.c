/* OpenSSL 3.4.1 — extensions.c: tls_parse_extension + tls_parse_all_extensions A — L739-860 */
int tls_parse_extension(SSL_CONNECTION *s, TLSEXT_INDEX idx, int context,
                        RAW_EXTENSION *exts, X509 *x, size_t chainidx)
{
    RAW_EXTENSION *currext = &exts[idx];
    int (*parser)(SSL_CONNECTION *s, PACKET *pkt, unsigned int context, X509 *x,
                  size_t chainidx) = NULL;

    /* Skip if the extension is not present */
    if (!currext->present)
        return 1;

    /* Skip if we've already parsed this extension */
    if (currext->parsed)
        return 1;

    currext->parsed = 1;

    if (idx < OSSL_NELEM(ext_defs)) {
        /* We are handling a built-in extension */
        const EXTENSION_DEFINITION *extdef = &ext_defs[idx];

        /* Check if extension is defined for our protocol. If not, skip */
        if (!extension_is_relevant(s, extdef->context, context))
            return 1;

        parser = s->server ? extdef->parse_ctos : extdef->parse_stoc;

        if (parser != NULL)
            return parser(s, &currext->data, context, x, chainidx);

        /*
         * If the parser is NULL we fall through to the custom extension
         * processing
         */
    }

    /* Parse custom extensions */
    return custom_ext_parse(s, context, currext->type,
                            PACKET_data(&currext->data),
                            PACKET_remaining(&currext->data),
                            x, chainidx);
}

/*
 * Parse all remaining extensions that have not yet been parsed. Also calls the
 * finalisation for all extensions at the end if |fin| is nonzero, whether we
 * collected them or not. Returns 1 for success or 0 for failure. If we are
 * working on a Certificate message then we also pass the Certificate |x| and
 * its position in the |chainidx|, with 0 being the first certificate.
 */
int tls_parse_all_extensions(SSL_CONNECTION *s, int context,
                             RAW_EXTENSION *exts, X509 *x,
                             size_t chainidx, int fin)
{
    size_t i, numexts = OSSL_NELEM(ext_defs);
    const EXTENSION_DEFINITION *thisexd;

    /* Calculate the number of extensions in the extensions list */
    numexts += s->cert->custext.meths_count;

    /* Parse each extension in turn */
    for (i = 0; i < numexts; i++) {
        if (!tls_parse_extension(s, i, context, exts, x, chainidx)) {
            /* SSLfatal() already called */
            return 0;
        }
    }

    if (fin) {
        /*
         * Finalise all known extensions relevant to this context,
         * whether we have found them or not
         */
        for (i = 0, thisexd = ext_defs; i < OSSL_NELEM(ext_defs);
             i++, thisexd++) {
            if (thisexd->final != NULL && (thisexd->context & context) != 0
                && !thisexd->final(s, context, exts[i].present)) {
                /* SSLfatal() already called */
                return 0;
            }
        }
    }

    return 1;
}

int should_add_extension(SSL_CONNECTION *s, unsigned int extctx,
                         unsigned int thisctx, int max_version)
{
    /* Skip if not relevant for our context */
    if ((extctx & thisctx) == 0)
        return 0;

    /* Check if this extension is defined for our protocol. If not, skip */
    if (!extension_is_relevant(s, extctx, thisctx)
            || ((extctx & SSL_EXT_TLS1_3_ONLY) != 0
                && (thisctx & SSL_EXT_CLIENT_HELLO) != 0
                && (SSL_CONNECTION_IS_DTLS(s) || max_version < TLS1_3_VERSION)))
        return 0;

    return 1;
}

/*
 * Construct all the extensions relevant to the current |context| and write
 * them to |pkt|. If this is an extension for a Certificate in a Certificate
 * message, then |x| will be set to the Certificate we are handling, and
 * |chainidx| will indicate the position in the chainidx we are processing (with
 * 0 being the first in the chain). Returns 1 on success or 0 on failure. On a
 * failure construction stops at the first extension to fail to construct.
 */
int tls_construct_extensions(SSL_CONNECTION *s, WPACKET *pkt,
                             unsigned int context,
                             X509 *x, size_t chainidx)
{
    size_t i;
    int min_version, max_version = 0, reason;
    const EXTENSION_DEFINITION *thisexd;
    int for_comp = (context & SSL_EXT_TLS1_3_CERTIFICATE_COMPRESSION) != 0;

    if (!WPACKET_start_sub_packet_u16(pkt)
               /*
