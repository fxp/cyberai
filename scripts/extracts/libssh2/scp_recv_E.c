                    }
                    /* Way too short to be an SCP response, or not done yet,
                       short circuit */
                    continue;
                }

                /* We're guaranteed not to go under response_len == 0 by the
                   logic above */
                while((session->
                        scpRecv_response[session->scpRecv_response_len - 1] ==
                        '\r')
                       || (session->
                           scpRecv_response[session->scpRecv_response_len -
                                            1] == '\n')) {
                    session->scpRecv_response_len--;
                }
                session->scpRecv_response[session->scpRecv_response_len] =
                    '\0';

                if(session->scpRecv_response_len < 6) {
                    /* EOL came too soon */
                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Invalid response from SCP server, "
                                   "too short");
                    goto scp_recv_error;
                }

                s = (char *) session->scpRecv_response + 1;

                p = strchr(s, ' ');
                if(!p || ((p - s) <= 0)) {
                    /* No spaces or space in the wrong spot */
                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Invalid response from SCP server, "
                                   "malformed mode");
                    goto scp_recv_error;
                }

                *(p++) = '\0';
                /* Make sure we don't get fooled by leftover values */
                session->scpRecv_mode = strtol(s, &e, 8);
                if(e && *e) {
                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Invalid response from SCP server, "
                                   "invalid mode");
                    goto scp_recv_error;
                }

                s = strchr(p, ' ');
                if(!s || ((s - p) <= 0)) {
                    /* No spaces or space in the wrong spot */
                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Invalid response from SCP server, "
                                   "too short or malformed");
                    goto scp_recv_error;
                }

                *s = '\0';
                /* Make sure we don't get fooled by leftover values */
                session->scpRecv_size = scpsize_strtol(p, &e, 10);
                if(e && *e) {
                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Invalid response from SCP server, "
                                   "invalid size");
                    goto scp_recv_error;
                }

                /* SCP ACK */
                session->scpRecv_response[0] = '\0';

                session->scpRecv_state = libssh2_NB_state_sent6;
            }

            if(session->scpRecv_state == libssh2_NB_state_sent6) {
                rc = (int)_libssh2_channel_write(session->scpRecv_channel, 0,
                                                 session->scpRecv_response, 1);
                if(rc == LIBSSH2_ERROR_EAGAIN) {
                    _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                                   "Would block sending SCP ACK");
                    return NULL;
                }
                else if(rc != 1) {
                    goto scp_recv_error;
                }
                _libssh2_debug((session, LIBSSH2_TRACE_SCP,
                               "mode = 0%lo size = %ld", session->scpRecv_mode,
                               (long)session->scpRecv_size));

                /* We *should* check that basename is valid, but why let that
                   stop us? */
                break;
            }
        }

        session->scpRecv_state = libssh2_NB_state_sent7;
    }

    if(sb) {
        memset(sb, 0, sizeof(libssh2_struct_stat));

        sb->st_mtime = session->scpRecv_mtime;
