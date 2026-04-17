            LIBSSH2_FREE(session, session->userauth_host_method);
            session->userauth_host_method = NULL;
            LIBSSH2_FREE(session, session->userauth_host_packet);
            session->userauth_host_packet = NULL;
            return rc;
        }

        _libssh2_htonu32(buf, session->session_id_len);
        libssh2_prepare_iovec(datavec, 4);
        datavec[0].iov_base = (void *)buf;
        datavec[0].iov_len = 4;
        datavec[1].iov_base = (void *)session->session_id;
        datavec[1].iov_len = session->session_id_len;
        datavec[2].iov_base = (void *)session->userauth_host_packet;
        datavec[2].iov_len = session->userauth_host_packet_len;

        if(privkeyobj && privkeyobj->signv &&
                         privkeyobj->signv(session, &sig, &sig_len, 3,
                                           datavec, &abstract)) {
            LIBSSH2_FREE(session, session->userauth_host_method);
            session->userauth_host_method = NULL;
            LIBSSH2_FREE(session, session->userauth_host_packet);
            session->userauth_host_packet = NULL;
            if(privkeyobj->dtor) {
                privkeyobj->dtor(session, &abstract);
            }
            return -1;
        }

        if(privkeyobj && privkeyobj->dtor) {
            privkeyobj->dtor(session, &abstract);
        }

        if(sig_len > pubkeydata_len) {
            unsigned char *newpacket;
            /* Should *NEVER* happen, but...well.. better safe than sorry */
            newpacket = LIBSSH2_REALLOC(session, session->userauth_host_packet,
                                        session->userauth_host_packet_len + 4 +
                                        (4 + session->userauth_host_method_len)
                                        + (4 + sig_len)); /* PK sigblob */
            if(!newpacket) {
                LIBSSH2_FREE(session, sig);
                LIBSSH2_FREE(session, session->userauth_host_packet);
                session->userauth_host_packet = NULL;
                LIBSSH2_FREE(session, session->userauth_host_method);
                session->userauth_host_method = NULL;
                return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                      "Failed allocating additional space for "
                                      "userauth-hostbased packet");
            }
            session->userauth_host_packet = newpacket;
        }

        session->userauth_host_s =
            session->userauth_host_packet + session->userauth_host_packet_len;

        _libssh2_store_u32(&session->userauth_host_s,
                           (uint32_t)(4 + session->userauth_host_method_len +
                                      4 + sig_len));
        _libssh2_store_str(&session->userauth_host_s,
                           (const char *)session->userauth_host_method,
                           session->userauth_host_method_len);
        LIBSSH2_FREE(session, session->userauth_host_method);
        session->userauth_host_method = NULL;

        _libssh2_store_str(&session->userauth_host_s, (const char *)sig,
                           sig_len);
        LIBSSH2_FREE(session, sig);

        _libssh2_debug((session, LIBSSH2_TRACE_AUTH,
                       "Attempting hostbased authentication"));

        session->userauth_host_state = libssh2_NB_state_created;
    }

    if(session->userauth_host_state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session, session->userauth_host_packet,
                                     session->userauth_host_s -
                                     session->userauth_host_packet,
                                     NULL, 0);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
