                }
            }

            /*
             * _libssh2_debug() will actually truncate this for us so
             * that it's not an inordinate about of data
             */
            _libssh2_debug((session, LIBSSH2_TRACE_TRANS,
                           "Debug Packet: %s", message));
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return 0;

            /*
              byte      SSH_MSG_EXT_INFO
              uint32    nr-extensions
              [repeat   "nr-extensions" times]
              string    extension-name  [RFC8308]
              string    extension-value (binary)
            */

        case SSH_MSG_EXT_INFO:
            if(datalen >= 5) {
                uint32_t nr_extensions = 0;
                struct string_buf buf;
                buf.data = (unsigned char *)data;
                buf.dataptr = buf.data;
                buf.len = datalen;
                buf.dataptr += 1; /* advance past type */

                if(_libssh2_get_u32(&buf, &nr_extensions) != 0) {
                    rc = _libssh2_error(session, LIBSSH2_ERROR_PROTO,
                                        "Invalid extension info received");
                }

                while(rc == 0 && nr_extensions > 0) {

                    size_t name_len = 0;
                    size_t value_len = 0;
                    unsigned char *name = NULL;
                    unsigned char *value = NULL;

                    nr_extensions -= 1;

                    _libssh2_get_string(&buf, &name, &name_len);
                    _libssh2_get_string(&buf, &value, &value_len);

                    if(name && value) {
                        _libssh2_debug((session,
                                       LIBSSH2_TRACE_KEX,
                                       "Server to Client extension %.*s: %.*s",
                                       (int)name_len, name,
                                       (int)value_len, value));
                    }

                    if(name_len == 15 &&
                        memcmp(name, "server-sig-algs", 15) == 0) {
                        if(session->server_sign_algorithms) {
                            LIBSSH2_FREE(session,
                                         session->server_sign_algorithms);
                        }

                        session->server_sign_algorithms =
                                                LIBSSH2_ALLOC(session,
                                                              value_len + 1);

                        if(session->server_sign_algorithms) {
                            memcpy(session->server_sign_algorithms,
                                   value, value_len);
                            session->server_sign_algorithms[value_len] = '\0';
                        }
                        else {
                            rc = _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                                "memory for server sign algo");
                        }
                    }
                }
            }

            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return rc;

            /*
              byte      SSH_MSG_GLOBAL_REQUEST
              string    request name in US-ASCII only
              boolean   want reply
              ....      request-specific data follows
            */

        case SSH_MSG_GLOBAL_REQUEST:
            if(datalen >= 5) {
                uint32_t len = 0;
                unsigned char want_reply = 0;
                len = _libssh2_ntohu32(data + 1);
                if((len <= (UINT_MAX - 6)) && (datalen >= (6 + len))) {
                    want_reply = data[5 + len];
                    _libssh2_debug((session,
