            session->fullpacket_required_type != msg) {
            LIBSSH2_FREE(session, data);
            session->socket_state = LIBSSH2_SOCKET_DISCONNECTED;
            session->packAdd_state = libssh2_NB_state_idle;
            libssh2_session_disconnect(session, "strict KEX violation: "
                                       "unexpected packet type");

            return _libssh2_error(session, LIBSSH2_ERROR_SOCKET_DISCONNECT,
                                  "strict KEX violation: "
                                  "unexpected packet type");
        }
    }

    if(session->packAdd_state == libssh2_NB_state_allocated) {
        /* A couple exceptions to the packet adding rule: */
        switch(msg) {

            /*
              byte      SSH_MSG_DISCONNECT
              uint32    reason code
              string    description in ISO-10646 UTF-8 encoding [RFC3629]
              string    language tag [RFC3066]
            */

        case SSH_MSG_DISCONNECT:
            if(datalen >= 5) {
                uint32_t reason = 0;
                struct string_buf buf;
                buf.data = (unsigned char *)data;
                buf.dataptr = buf.data;
                buf.len = datalen;
                buf.dataptr++; /* advance past type */

                _libssh2_get_u32(&buf, &reason);
                _libssh2_get_string(&buf, &message, &message_len);
                _libssh2_get_string(&buf, &language, &language_len);

                if(session->ssh_msg_disconnect) {
                    LIBSSH2_DISCONNECT(session, reason, (const char *)message,
                                       message_len, (const char *)language,
                                       language_len);
                }

                _libssh2_debug((session, LIBSSH2_TRACE_TRANS,
                               "Disconnect(%d): %s(%s)", reason,
                               message, language));
            }

            LIBSSH2_FREE(session, data);
            session->socket_state = LIBSSH2_SOCKET_DISCONNECTED;
            session->packAdd_state = libssh2_NB_state_idle;
            return _libssh2_error(session, LIBSSH2_ERROR_SOCKET_DISCONNECT,
                                  "socket disconnect");
            /*
              byte      SSH_MSG_IGNORE
              string    data
            */

        case SSH_MSG_IGNORE:
            if(datalen >= 2) {
                if(session->ssh_msg_ignore) {
                    LIBSSH2_IGNORE(session, (char *) data + 1, datalen - 1);
                }
            }
            else if(session->ssh_msg_ignore) {
                LIBSSH2_IGNORE(session, "", 0);
            }
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return 0;

            /*
              byte      SSH_MSG_DEBUG
              boolean   always_display
              string    message in ISO-10646 UTF-8 encoding [RFC3629]
              string    language tag [RFC3066]
            */

        case SSH_MSG_DEBUG:
            if(datalen >= 2) {
                int always_display = data[1];

                if(datalen >= 6) {
                    struct string_buf buf;
                    buf.data = (unsigned char *)data;
                    buf.dataptr = buf.data;
                    buf.len = datalen;
                    buf.dataptr += 2; /* advance past type & always display */

                    _libssh2_get_string(&buf, &message, &message_len);
                    _libssh2_get_string(&buf, &language, &language_len);
                }

                if(session->ssh_msg_debug) {
                    LIBSSH2_DEBUG(session, always_display,
                                  (const char *)message,
                                  message_len, (const char *)language,
                                  language_len);
