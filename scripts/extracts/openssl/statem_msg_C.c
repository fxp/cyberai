/* OpenSSL 3.4.1 — statem_lib.c: ssl3_finish_mac + ssl3_take_mac: MAC computation — L1760-1870 */
    {X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE, SSL_AD_BAD_CERTIFICATE},
    {X509_V_ERR_UNABLE_TO_GET_CRL, SSL_AD_UNKNOWN_CA},
    {X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER, SSL_AD_UNKNOWN_CA},
    {X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT, SSL_AD_UNKNOWN_CA},
    {X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY, SSL_AD_UNKNOWN_CA},
    {X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE, SSL_AD_UNKNOWN_CA},
    {X509_V_ERR_UNSPECIFIED, SSL_AD_INTERNAL_ERROR},

    /* Last entry; return this if we don't find the value above. */
    {X509_V_OK, SSL_AD_CERTIFICATE_UNKNOWN}
};

int ssl_x509err2alert(int x509err)
{
    const X509ERR2ALERT *tp;

    for (tp = x509table; tp->x509err != X509_V_OK; ++tp)
        if (tp->x509err == x509err)
            break;
    return tp->alert;
}

int ssl_allow_compression(SSL_CONNECTION *s)
{
    if (s->options & SSL_OP_NO_COMPRESSION)
        return 0;
    return ssl_security(s, SSL_SECOP_COMPRESSION, 0, 0, NULL);
}

/*
 * SSL/TLS/DTLS version comparison
 *
 * Returns
 *      0 if versiona is equal to versionb
 *      1 if versiona is greater than versionb
 *     -1 if versiona is less than versionb
 */
int ssl_version_cmp(const SSL_CONNECTION *s, int versiona, int versionb)
{
    int dtls = SSL_CONNECTION_IS_DTLS(s);

    if (versiona == versionb)
        return 0;
    if (!dtls)
        return versiona < versionb ? -1 : 1;
    return DTLS_VERSION_LT(versiona, versionb) ? -1 : 1;
}

typedef struct {
    int version;
    const SSL_METHOD *(*cmeth) (void);
    const SSL_METHOD *(*smeth) (void);
} version_info;

#if TLS_MAX_VERSION_INTERNAL != TLS1_3_VERSION
# error Code needs update for TLS_method() support beyond TLS1_3_VERSION.
#endif

/* Must be in order high to low */
static const version_info tls_version_table[] = {
#ifndef OPENSSL_NO_TLS1_3
    {TLS1_3_VERSION, tlsv1_3_client_method, tlsv1_3_server_method},
#else
    {TLS1_3_VERSION, NULL, NULL},
#endif
#ifndef OPENSSL_NO_TLS1_2
    {TLS1_2_VERSION, tlsv1_2_client_method, tlsv1_2_server_method},
#else
    {TLS1_2_VERSION, NULL, NULL},
#endif
#ifndef OPENSSL_NO_TLS1_1
    {TLS1_1_VERSION, tlsv1_1_client_method, tlsv1_1_server_method},
#else
    {TLS1_1_VERSION, NULL, NULL},
#endif
#ifndef OPENSSL_NO_TLS1
    {TLS1_VERSION, tlsv1_client_method, tlsv1_server_method},
#else
    {TLS1_VERSION, NULL, NULL},
#endif
#ifndef OPENSSL_NO_SSL3
    {SSL3_VERSION, sslv3_client_method, sslv3_server_method},
#else
    {SSL3_VERSION, NULL, NULL},
#endif
    {0, NULL, NULL},
};

#if DTLS_MAX_VERSION_INTERNAL != DTLS1_2_VERSION
# error Code needs update for DTLS_method() support beyond DTLS1_2_VERSION.
#endif

/* Must be in order high to low */
static const version_info dtls_version_table[] = {
#ifndef OPENSSL_NO_DTLS1_2
    {DTLS1_2_VERSION, dtlsv1_2_client_method, dtlsv1_2_server_method},
#else
    {DTLS1_2_VERSION, NULL, NULL},
#endif
#ifndef OPENSSL_NO_DTLS1
    {DTLS1_VERSION, dtlsv1_client_method, dtlsv1_server_method},
    {DTLS1_BAD_VER, dtls_bad_ver_client_method, NULL},
#else
    {DTLS1_VERSION, NULL, NULL},
    {DTLS1_BAD_VER, NULL, NULL},
#endif
    {0, NULL, NULL},
};

/*
 * ssl_method_error - Check whether an SSL_METHOD is enabled.
