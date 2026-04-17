{
    int rc = 0;
    unsigned char *message = NULL;
    unsigned char *language = NULL;
    size_t message_len = 0;
    size_t language_len = 0;
    LIBSSH2_CHANNEL *channelp = NULL;
    size_t data_head = 0;
    unsigned char msg = data[0];

    switch(session->packAdd_state) {
    case libssh2_NB_state_idle:
        _libssh2_debug((session, LIBSSH2_TRACE_TRANS,
                       "Packet type %u received, length=%ld",
                       (unsigned int) msg, (long) datalen));

        if((macstate == LIBSSH2_MAC_INVALID) &&
            (!session->macerror ||
             LIBSSH2_MACERROR(session, (char *) data, datalen))) {
            /* Bad MAC input, but no callback set or non-zero return from the
               callback */

            LIBSSH2_FREE(session, data);
            return _libssh2_error(session, LIBSSH2_ERROR_INVALID_MAC,
                                  "Invalid MAC received");
        }
        session->packAdd_state = libssh2_NB_state_allocated;
        break;
    case libssh2_NB_state_jump1:
        goto libssh2_packet_add_jump_point1;
    case libssh2_NB_state_jump2:
        goto libssh2_packet_add_jump_point2;
    case libssh2_NB_state_jump3:
        goto libssh2_packet_add_jump_point3;
    case libssh2_NB_state_jump4:
        goto libssh2_packet_add_jump_point4;
    case libssh2_NB_state_jump5:
        goto libssh2_packet_add_jump_point5;
    case libssh2_NB_state_jumpauthagent:
        goto libssh2_packet_add_jump_authagent;
    default: /* nothing to do */
        break;
    }

    if(session->state & LIBSSH2_STATE_INITIAL_KEX) {
        if(msg == SSH_MSG_KEXINIT) {
            if(!session->kex_strict) {
                if(datalen < 17) {
                    LIBSSH2_FREE(session, data);
                    session->packAdd_state = libssh2_NB_state_idle;
                    return _libssh2_error(session,
                                          LIBSSH2_ERROR_BUFFER_TOO_SMALL,
                                          "Data too short extracting kex");
                }
                else {
                    const unsigned char *strict =
                    (unsigned char *)"kex-strict-s-v00@openssh.com";
                    struct string_buf buf;
                    unsigned char *algs = NULL;
                    size_t algs_len = 0;

                    buf.data = (unsigned char *)data;
                    buf.dataptr = buf.data;
                    buf.len = datalen;
                    buf.dataptr += 17; /* advance past type and cookie */

                    if(_libssh2_get_string(&buf, &algs, &algs_len)) {
                        LIBSSH2_FREE(session, data);
                        session->packAdd_state = libssh2_NB_state_idle;
                        return _libssh2_error(session,
                                              LIBSSH2_ERROR_BUFFER_TOO_SMALL,
                                              "Algs too short");
                    }

                    if(algs_len == 0 ||
                       _libssh2_kex_agree_instr(algs, algs_len, strict, 28)) {
                        session->kex_strict = 1;
                    }
                }
            }

            if(session->kex_strict && seq) {
                LIBSSH2_FREE(session, data);
                session->socket_state = LIBSSH2_SOCKET_DISCONNECTED;
                session->packAdd_state = libssh2_NB_state_idle;
                libssh2_session_disconnect(session, "strict KEX violation: "
                                           "KEXINIT was not the first packet");

                return _libssh2_error(session, LIBSSH2_ERROR_SOCKET_DISCONNECT,
                                      "strict KEX violation: "
                                      "KEXINIT was not the first packet");
            }
        }

        if(session->kex_strict && session->fullpacket_required_type &&
