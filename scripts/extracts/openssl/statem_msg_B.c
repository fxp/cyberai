/* OpenSSL 3.4.1 — statem_lib.c: tls_get_message_body: message body read A — L1642-1720 */
int tls_get_message_body(SSL_CONNECTION *s, size_t *len)
{
    size_t n, readbytes;
    unsigned char *p;
    int i;
    SSL *ssl = SSL_CONNECTION_GET_SSL(s);
    SSL *ussl = SSL_CONNECTION_GET_USER_SSL(s);

    if (s->s3.tmp.message_type == SSL3_MT_CHANGE_CIPHER_SPEC) {
        /* We've already read everything in */
        *len = (unsigned long)s->init_num;
        return 1;
    }

    p = s->init_msg;
    n = s->s3.tmp.message_size - s->init_num;
    while (n > 0) {
        i = ssl->method->ssl_read_bytes(ssl, SSL3_RT_HANDSHAKE, NULL,
                                        &p[s->init_num], n, 0, &readbytes);
        if (i <= 0) {
            s->rwstate = SSL_READING;
            *len = 0;
            return 0;
        }
        s->init_num += readbytes;
        n -= readbytes;
    }

    /*
     * If receiving Finished, record MAC of prior handshake messages for
     * Finished verification.
     */
    if (*(s->init_buf->data) == SSL3_MT_FINISHED && !ssl3_take_mac(s)) {
        /* SSLfatal() already called */
        *len = 0;
        return 0;
    }

    /* Feed this message into MAC computation. */
    if (RECORD_LAYER_is_sslv2_record(&s->rlayer)) {
        if (!ssl3_finish_mac(s, (unsigned char *)s->init_buf->data,
                             s->init_num)) {
            /* SSLfatal() already called */
            *len = 0;
            return 0;
        }
        if (s->msg_callback)
            s->msg_callback(0, SSL2_VERSION, 0, s->init_buf->data,
                            (size_t)s->init_num, ussl, s->msg_callback_arg);
    } else {
        /*
         * We defer feeding in the HRR until later. We'll do it as part of
         * processing the message
         * The TLsv1.3 handshake transcript stops at the ClientFinished
         * message.
         */
#define SERVER_HELLO_RANDOM_OFFSET  (SSL3_HM_HEADER_LENGTH + 2)
        /* KeyUpdate and NewSessionTicket do not need to be added */
        if (!SSL_CONNECTION_IS_TLS13(s)
            || (s->s3.tmp.message_type != SSL3_MT_NEWSESSION_TICKET
                         && s->s3.tmp.message_type != SSL3_MT_KEY_UPDATE)) {
            if (s->s3.tmp.message_type != SSL3_MT_SERVER_HELLO
                    || s->init_num < SERVER_HELLO_RANDOM_OFFSET + SSL3_RANDOM_SIZE
                    || memcmp(hrrrandom,
                              s->init_buf->data + SERVER_HELLO_RANDOM_OFFSET,
                              SSL3_RANDOM_SIZE) != 0) {
                if (!ssl3_finish_mac(s, (unsigned char *)s->init_buf->data,
                                     s->init_num + SSL3_HM_HEADER_LENGTH)) {
                    /* SSLfatal() already called */
                    *len = 0;
                    return 0;
                }
            }
        }
        if (s->msg_callback)
            s->msg_callback(0, s->version, SSL3_RT_HANDSHAKE, s->init_buf->data,
                            (size_t)s->init_num + SSL3_HM_HEADER_LENGTH, ussl,
                            s->msg_callback_arg);
    }
