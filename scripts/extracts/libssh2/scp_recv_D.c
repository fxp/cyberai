            if(session->scpRecv_state == libssh2_NB_state_sent3) {
                rc = (int)_libssh2_channel_write(session->scpRecv_channel, 0,
                                                 session->scpRecv_response, 1);
                if(rc == LIBSSH2_ERROR_EAGAIN) {
                    _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                                   "Would block waiting to send SCP ACK");
                    return NULL;
                }
                else if(rc != 1) {
                    goto scp_recv_error;
                }

                _libssh2_debug((session, LIBSSH2_TRACE_SCP,
                               "mtime = %ld, atime = %ld",
                               session->scpRecv_mtime,
                               session->scpRecv_atime));

                /* We *should* check that atime.usec is valid, but why let
                   that stop use? */
                break;
            }
        }

        session->scpRecv_state = libssh2_NB_state_sent4;
    }

    if(session->scpRecv_state == libssh2_NB_state_sent4) {
        session->scpRecv_response_len = 0;

        session->scpRecv_state = libssh2_NB_state_sent5;
    }

    if((session->scpRecv_state == libssh2_NB_state_sent5)
        || (session->scpRecv_state == libssh2_NB_state_sent6)) {
        while(session->scpRecv_response_len < LIBSSH2_SCP_RESPONSE_BUFLEN) {
            char *s, *p, *e = NULL;

            if(session->scpRecv_state == libssh2_NB_state_sent5) {
                rc = (int)_libssh2_channel_read(session->scpRecv_channel, 0,
                                                (char *) session->
                                                scpRecv_response +
                                                session->scpRecv_response_len,
                                                1);
                if(rc == LIBSSH2_ERROR_EAGAIN) {
                    _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                                   "Would block waiting for SCP response");
                    return NULL;
                }
                else if(rc < 0) {
                    /* error, bail out */
                    _libssh2_error(session, rc, "Failed reading SCP response");
                    goto scp_recv_error;
                }
                else if(rc == 0)
                    goto scp_recv_empty_channel;

                session->scpRecv_response_len++;

                if(session->scpRecv_response[0] != 'C') {
                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Invalid response from SCP server");
                    goto scp_recv_error;
                }

                if((session->scpRecv_response_len > 1) &&
                    (session->
                     scpRecv_response[session->scpRecv_response_len - 1] !=
                     '\r')
                    && (session->
                        scpRecv_response[session->scpRecv_response_len - 1] !=
                        '\n')
                    &&
                    (session->
                     scpRecv_response[session->scpRecv_response_len - 1]
                     < 32)) {
                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Invalid data in SCP response");
                    goto scp_recv_error;
                }

                if((session->scpRecv_response_len < 7)
                    || (session->
                        scpRecv_response[session->scpRecv_response_len - 1] !=
                        '\n')) {
                    if(session->scpRecv_response_len ==
                        LIBSSH2_SCP_RESPONSE_BUFLEN) {
                        /* You had your chance */
                        _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                       "Unterminated response "
                                       "from SCP server");
                        goto scp_recv_error;
