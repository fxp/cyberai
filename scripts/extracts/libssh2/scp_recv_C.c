                    && (session->
                        scpRecv_response[session->scpRecv_response_len - 1] !=
                        '\n')) {
                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Invalid data in SCP response");
                    goto scp_recv_error;
                }

                if((session->scpRecv_response_len < 9)
                    || (session->
                        scpRecv_response[session->scpRecv_response_len - 1] !=
                        '\n')) {
                    if(session->scpRecv_response_len ==
                        LIBSSH2_SCP_RESPONSE_BUFLEN) {
                        /* You had your chance */
                        _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                       "Unterminated response from "
                                       "SCP server");
                        goto scp_recv_error;
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
                                            1] == '\n'))
                    session->scpRecv_response_len--;
                session->scpRecv_response[session->scpRecv_response_len] =
                    '\0';

                if(session->scpRecv_response_len < 8) {
                    /* EOL came too soon */
                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Invalid response from SCP server, "
                                   "too short");
                    goto scp_recv_error;
                }

                s = session->scpRecv_response + 1;

                p = (unsigned char *) strchr((char *) s, ' ');
                if(!p || ((p - s) <= 0)) {
                    /* No spaces or space in the wrong spot */
                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Invalid response from SCP server, "
                                   "malformed mtime");
                    goto scp_recv_error;
                }

                *(p++) = '\0';
                /* Make sure we don't get fooled by leftover values */
                session->scpRecv_mtime = strtol((char *) s, NULL, 10);

                s = (unsigned char *) strchr((char *) p, ' ');
                if(!s || ((s - p) <= 0)) {
                    /* No spaces or space in the wrong spot */
                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Invalid response from SCP server, "
                                   "malformed mtime.usec");
                    goto scp_recv_error;
                }

                /* Ignore mtime.usec */
                s++;
                p = (unsigned char *) strchr((char *) s, ' ');
                if(!p || ((p - s) <= 0)) {
                    /* No spaces or space in the wrong spot */
                    _libssh2_error(session, LIBSSH2_ERROR_SCP_PROTOCOL,
                                   "Invalid response from SCP server, "
                                   "too short or malformed");
                    goto scp_recv_error;
                }

                *p = '\0';
                /* Make sure we don't get fooled by leftover values */
                session->scpRecv_atime = strtol((char *) s, NULL, 10);

                /* SCP ACK */
                session->scpRecv_response[0] = '\0';

                session->scpRecv_state = libssh2_NB_state_sent3;
            }

