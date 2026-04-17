                                              session->userauth_pswd_data_len);
                        }
                        else {
                            s = session->userauth_pswd_data = NULL;
                            session->userauth_pswd_data_len = 0;
                        }

                        if(!session->userauth_pswd_data) {
                            LIBSSH2_FREE(session,
                                         session->userauth_pswd_newpw);
                            session->userauth_pswd_newpw = NULL;
                            return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                                  "Unable to allocate memory "
                                                  "for userauth password "
                                                  "change request");
                        }

                        *(s++) = SSH_MSG_USERAUTH_REQUEST;
                        _libssh2_store_str(&s, username, username_len);
                        _libssh2_store_str(&s, "ssh-connection",
                                           sizeof("ssh-connection") - 1);
                        _libssh2_store_str(&s, "password",
                                           sizeof("password") - 1);
                        *s++ = 0x01;
                        _libssh2_store_str(&s, (char *)password, password_len);
                        _libssh2_store_u32(&s,
                                           session->userauth_pswd_newpw_len);
                        /* send session->userauth_pswd_newpw separately */

                        session->userauth_pswd_state = libssh2_NB_state_sent2;
                    }

                    if(session->userauth_pswd_state ==
                       libssh2_NB_state_sent2) {
                        rc = _libssh2_transport_send(session,
                                            session->userauth_pswd_data,
                                            session->userauth_pswd_data_len,
                                            (unsigned char *)
                                            session->userauth_pswd_newpw,
                                            session->userauth_pswd_newpw_len);
                        if(rc == LIBSSH2_ERROR_EAGAIN) {
                            return _libssh2_error(session,
                                                  LIBSSH2_ERROR_EAGAIN,
                                                  "Would block waiting");
                        }

                        /* free the allocated packets again */
                        LIBSSH2_FREE(session, session->userauth_pswd_data);
                        session->userauth_pswd_data = NULL;
                        LIBSSH2_FREE(session, session->userauth_pswd_newpw);
                        session->userauth_pswd_newpw = NULL;

                        if(rc) {
                            return _libssh2_error(session,
                                                  LIBSSH2_ERROR_SOCKET_SEND,
                                                  "Unable to send userauth "
                                                  "password-change request");
                        }

                        /*
                         * Ugliest use of goto ever.  Blame it on the
                         * askN => requirev migration.
                         */
                        session->userauth_pswd_state = libssh2_NB_state_sent;
                        goto password_response;
                    }
                }
            }
            else {
                session->userauth_pswd_state = libssh2_NB_state_idle;
                return _libssh2_error(session, LIBSSH2_ERROR_PASSWORD_EXPIRED,
                                      "Password Expired, and no callback "
                                      "specified");
            }
        }
    }

    /* FAILURE */
    LIBSSH2_FREE(session, session->userauth_pswd_data);
    session->userauth_pswd_data = NULL;
    session->userauth_pswd_state = libssh2_NB_state_idle;

    return _libssh2_error(session, LIBSSH2_ERROR_AUTHENTICATION_FAILED,
                          "Authentication failed");
}
