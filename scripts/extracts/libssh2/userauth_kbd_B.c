                session->userauth_pswd_state = libssh2_NB_state_idle;
                return _libssh2_error(session, LIBSSH2_ERROR_PROTO,
                                      "Unexpected packet size");
            }

            if(session->userauth_pswd_data[0] == SSH_MSG_USERAUTH_SUCCESS) {
                _libssh2_debug((session, LIBSSH2_TRACE_AUTH,
                               "Password authentication successful"));
                LIBSSH2_FREE(session, session->userauth_pswd_data);
                session->userauth_pswd_data = NULL;
                session->state |= LIBSSH2_STATE_AUTHENTICATED;
                session->userauth_pswd_state = libssh2_NB_state_idle;
                return 0;
            }
            else if(session->userauth_pswd_data[0] ==
                    SSH_MSG_USERAUTH_FAILURE) {
                _libssh2_debug((session, LIBSSH2_TRACE_AUTH,
                               "Password authentication failed"));
                LIBSSH2_FREE(session, session->userauth_pswd_data);
                session->userauth_pswd_data = NULL;
                session->userauth_pswd_state = libssh2_NB_state_idle;
                return _libssh2_error(session,
                                      LIBSSH2_ERROR_AUTHENTICATION_FAILED,
                                      "Authentication failed "
                                      "(username/password)");
            }

            session->userauth_pswd_newpw = NULL;
            session->userauth_pswd_newpw_len = 0;

            session->userauth_pswd_state = libssh2_NB_state_sent1;
        }

        if(session->userauth_pswd_data_len < 1) {
            session->userauth_pswd_state = libssh2_NB_state_idle;
            return _libssh2_error(session, LIBSSH2_ERROR_PROTO,
                                  "Unexpected packet size");
        }

        if((session->userauth_pswd_data[0] ==
             SSH_MSG_USERAUTH_PASSWD_CHANGEREQ)
            || (session->userauth_pswd_data0 ==
                SSH_MSG_USERAUTH_PASSWD_CHANGEREQ)) {
            session->userauth_pswd_data0 = SSH_MSG_USERAUTH_PASSWD_CHANGEREQ;

            if((session->userauth_pswd_state == libssh2_NB_state_sent1) ||
                (session->userauth_pswd_state == libssh2_NB_state_sent2)) {
                if(session->userauth_pswd_state == libssh2_NB_state_sent1) {
                    _libssh2_debug((session, LIBSSH2_TRACE_AUTH,
                                   "Password change required"));
                    LIBSSH2_FREE(session, session->userauth_pswd_data);
                    session->userauth_pswd_data = NULL;
                }
                if(passwd_change_cb) {
                    if(session->userauth_pswd_state ==
                       libssh2_NB_state_sent1) {
                        passwd_change_cb(session,
                                         &session->userauth_pswd_newpw,
                                         &session->userauth_pswd_newpw_len,
                                         &session->abstract);
                        if(!session->userauth_pswd_newpw) {
                            return _libssh2_error(session,
                                                LIBSSH2_ERROR_PASSWORD_EXPIRED,
                                                  "Password expired, and "
                                                  "callback failed");
                        }

                        /* basic data_len + newpw_len(4) */
                        if(username_len + password_len + 44 <= UINT_MAX) {
                            session->userauth_pswd_data_len =
                                username_len + password_len + 44;
                            s = session->userauth_pswd_data =
                                LIBSSH2_ALLOC(session,
