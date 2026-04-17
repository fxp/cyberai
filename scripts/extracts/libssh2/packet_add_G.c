                                           channelp->remote.id));
                        }
                    }
                }


                if(want_reply) {
                    unsigned char packet[5];
libssh2_packet_add_jump_point4:
                    session->packAdd_state = libssh2_NB_state_jump4;
                    packet[0] = SSH_MSG_CHANNEL_FAILURE;
                    memcpy(&packet[1], data + 1, 4);
                    rc = _libssh2_transport_send(session, packet, 5, NULL, 0);
                    if(rc == LIBSSH2_ERROR_EAGAIN)
                        return rc;
                }
            }
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return rc;

            /*
              byte      SSH_MSG_CHANNEL_CLOSE
              uint32    recipient channel
            */

        case SSH_MSG_CHANNEL_CLOSE:
            if(datalen >= 5)
                channelp =
                    _libssh2_channel_locate(session,
                                            _libssh2_ntohu32(data + 1));
            if(!channelp) {
                /* We may have freed already, just quietly ignore this... */
                LIBSSH2_FREE(session, data);
                session->packAdd_state = libssh2_NB_state_idle;
                return 0;
            }
            _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                           "Close received for channel %u/%u",
                           channelp->local.id,
                           channelp->remote.id));

            channelp->remote.close = 1;
            channelp->remote.eof = 1;

            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return 0;

            /*
              byte      SSH_MSG_CHANNEL_OPEN
              string    "session"
              uint32    sender channel
              uint32    initial window size
              uint32    maximum packet size
            */

        case SSH_MSG_CHANNEL_OPEN:
            if(datalen < 17)
                ;
            else if((datalen >= (strlen("forwarded-tcpip") + 5)) &&
                     (strlen("forwarded-tcpip") ==
                      _libssh2_ntohu32(data + 1))
                     &&
                     (memcmp(data + 5, "forwarded-tcpip",
                             strlen("forwarded-tcpip")) == 0)) {

                /* init the state struct */
                memset(&session->packAdd_Qlstn_state, 0,
                       sizeof(session->packAdd_Qlstn_state));

libssh2_packet_add_jump_point2:
                session->packAdd_state = libssh2_NB_state_jump2;
                rc = packet_queue_listener(session, data, datalen,
                                           &session->packAdd_Qlstn_state);
            }
            else if((datalen >= (strlen("x11") + 5)) &&
                     ((strlen("x11")) == _libssh2_ntohu32(data + 1)) &&
                     (memcmp(data + 5, "x11", strlen("x11")) == 0)) {

                /* init the state struct */
                memset(&session->packAdd_x11open_state, 0,
                       sizeof(session->packAdd_x11open_state));

libssh2_packet_add_jump_point3:
                session->packAdd_state = libssh2_NB_state_jump3;
                rc = packet_x11_open(session, data, datalen,
                                     &session->packAdd_x11open_state);
            }
            else if((datalen >= (strlen("auth-agent@openssh.com") + 5)) &&
                    (strlen("auth-agent@openssh.com") ==
                      _libssh2_ntohu32(data + 1)) &&
                    (memcmp(data + 5, "auth-agent@openssh.com",
                            strlen("auth-agent@openssh.com")) == 0)) {

                /* init the state struct */
                memset(&session->packAdd_authagent_state, 0,
