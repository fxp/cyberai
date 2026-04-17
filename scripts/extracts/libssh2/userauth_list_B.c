                               "Unexpected userauth banner size");
                return NULL;
            }
            session->userauth_banner = LIBSSH2_ALLOC(session, banner_len + 1);
            if(!session->userauth_banner) {
                LIBSSH2_FREE(session, session->userauth_list_data);
                session->userauth_list_data = NULL;
                _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                              "Unable to allocate memory for userauth_banner");
                return NULL;
            }
            memcpy(session->userauth_banner, session->userauth_list_data + 5,
                    banner_len);
            session->userauth_banner[banner_len] = '\0';
            _libssh2_debug((session, LIBSSH2_TRACE_AUTH,
                           "Banner: %s",
                           session->userauth_banner));
            LIBSSH2_FREE(session, session->userauth_list_data);
            session->userauth_list_data = NULL;
            /* SSH_MSG_USERAUTH_BANNER has been handled */
            reply_codes[2] = 0;
            rc = _libssh2_packet_requirev(session, reply_codes,
                                          &session->userauth_list_data,
                                          &session->userauth_list_data_len, 0,
                                          NULL, 0,
                                &session->userauth_list_packet_requirev_state);
            if(rc == LIBSSH2_ERROR_EAGAIN) {
                _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                               "Would block requesting userauth list");
                return NULL;
            }
            else if(rc || (session->userauth_list_data_len < 1)) {
                _libssh2_error(session, rc, "Failed getting response");
                session->userauth_list_state = libssh2_NB_state_idle;
                return NULL;
            }
        }

        if(session->userauth_list_data[0] == SSH_MSG_USERAUTH_SUCCESS) {
            /* Wow, who'dve thought... */
            _libssh2_error(session, LIBSSH2_ERROR_NONE, "No error");
            LIBSSH2_FREE(session, session->userauth_list_data);
            session->userauth_list_data = NULL;
            session->state |= LIBSSH2_STATE_AUTHENTICATED;
            session->userauth_list_state = libssh2_NB_state_idle;
            return NULL;
        }

        if(session->userauth_list_data_len < 5) {
            LIBSSH2_FREE(session, session->userauth_list_data);
            session->userauth_list_data = NULL;
            _libssh2_error(session, LIBSSH2_ERROR_PROTO,
                           "Unexpected packet size");
            return NULL;
        }

        methods_len = _libssh2_ntohu32(session->userauth_list_data + 1);
        if(methods_len >= session->userauth_list_data_len - 5) {
            _libssh2_error(session, LIBSSH2_ERROR_OUT_OF_BOUNDARY,
                           "Unexpected userauth list size");
            return NULL;
        }

        /* Do note that the memory areas overlap! */
        memmove(session->userauth_list_data, session->userauth_list_data + 5,
                methods_len);
        session->userauth_list_data[methods_len] = '\0';
        _libssh2_debug((session, LIBSSH2_TRACE_AUTH,
                       "Permitted auth methods: %s",
                       session->userauth_list_data));
    }

    session->userauth_list_state = libssh2_NB_state_idle;
    return (char *)session->userauth_list_data;
}
