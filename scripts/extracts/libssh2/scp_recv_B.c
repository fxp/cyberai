                                         session->scpRecv_response, 1);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                           "Would block sending initial wakeup");
            return NULL;
        }
        else if(rc != 1) {
            goto scp_recv_error;
        }

        /* Parse SCP response */
        session->scpRecv_response_len = 0;

        session->scpRecv_state = libssh2_NB_state_sent2;
    }

    if((session->scpRecv_state == libssh2_NB_state_sent2)
        || (session->scpRecv_state == libssh2_NB_state_sent3)) {
        while(sb && (session->scpRecv_response_len <
                     LIBSSH2_SCP_RESPONSE_BUFLEN)) {
            unsigned char *s, *p;

            if(session->scpRecv_state == libssh2_NB_state_sent2) {
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
                    /* error, give up */
                    _libssh2_error(session, rc, "Failed reading SCP response");
                    goto scp_recv_error;
                }
                else if(rc == 0)
                    goto scp_recv_empty_channel;

                session->scpRecv_response_len++;

                if(session->scpRecv_response[0] != 'T') {
                    size_t err_len;
                    char *err_msg;

                    /* there can be
                       01 for warnings
                       02 for errors

                       The following string MUST be newline terminated
                    */
                    err_len =
                        _libssh2_channel_packet_data_len(session->
                                                         scpRecv_channel, 0);
                    err_msg = LIBSSH2_ALLOC(session, err_len + 1);
                    if(!err_msg) {
                        _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                       "Failed to get memory ");
                        goto scp_recv_error;
                    }

                    /* Read the remote error message */
                    (void)_libssh2_channel_read(session->scpRecv_channel, 0,
                                                err_msg, err_len);
                    /* If it failed for any reason, we ignore it anyway. */

                    /* zero terminate the error */
                    err_msg[err_len] = 0;

                    _libssh2_debug((session, LIBSSH2_TRACE_SCP,
                                   "got %02x %s", session->scpRecv_response[0],
                                   err_msg));

                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Failed to recv file");

                    LIBSSH2_FREE(session, err_msg);
                    goto scp_recv_error;
                }

                if((session->scpRecv_response_len > 1) &&
                    ((session->
                      scpRecv_response[session->scpRecv_response_len - 1] <
                      '0')
                     || (session->
                         scpRecv_response[session->scpRecv_response_len - 1] >
                         '9'))
                    && (session->
                        scpRecv_response[session->scpRecv_response_len - 1] !=
                        ' ')
                    && (session->
                        scpRecv_response[session->scpRecv_response_len - 1] !=
                        '\r')
