                                   LIBSSH2_TRACE_CONN,
                                   "Received global request type %.*s (wr %X)",
                                   (int)len, data + 5, want_reply));
                }


                if(want_reply) {
                    static const unsigned char packet =
                        SSH_MSG_REQUEST_FAILURE;
libssh2_packet_add_jump_point5:
                    session->packAdd_state = libssh2_NB_state_jump5;
                    rc = _libssh2_transport_send(session, &packet, 1, NULL, 0);
                    if(rc == LIBSSH2_ERROR_EAGAIN)
                        return rc;
                }
            }
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return 0;

            /*
              byte      SSH_MSG_CHANNEL_EXTENDED_DATA
              uint32    recipient channel
              uint32    data_type_code
              string    data
            */

        case SSH_MSG_CHANNEL_EXTENDED_DATA:
            /* streamid(4) */
            data_head += 4;

            LIBSSH2_FALLTHROUGH();

            /*
              byte      SSH_MSG_CHANNEL_DATA
              uint32    recipient channel
              string    data
            */

        case SSH_MSG_CHANNEL_DATA:
            /* packet_type(1) + channelno(4) + datalen(4) */
            data_head += 9;

            if(datalen >= data_head)
                channelp =
                    _libssh2_channel_locate(session,
                                            _libssh2_ntohu32(data + 1));

            if(!channelp) {
                _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_UNKNOWN,
                               "Packet received for unknown channel");
                LIBSSH2_FREE(session, data);
                session->packAdd_state = libssh2_NB_state_idle;
                return 0;
            }
#ifdef LIBSSH2DEBUG
            {
                uint32_t stream_id = 0;
                if(msg == SSH_MSG_CHANNEL_EXTENDED_DATA)
                    stream_id = _libssh2_ntohu32(data + 5);

                _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                               "%ld bytes packet_add() for %u/%u/%u",
                               (long) (datalen - data_head),
                               channelp->local.id,
                               channelp->remote.id,
                               stream_id));
            }
#endif
            if((channelp->remote.extended_data_ignore_mode ==
                 LIBSSH2_CHANNEL_EXTENDED_DATA_IGNORE) &&
                (msg == SSH_MSG_CHANNEL_EXTENDED_DATA)) {
                /* Pretend we didn't receive this */
                LIBSSH2_FREE(session, data);

                _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                              "Ignoring extended data and refunding %ld bytes",
                               (long) (datalen - 13)));
                if(channelp->read_avail + datalen - data_head >=
                    channelp->remote.window_size)
                    datalen = channelp->remote.window_size -
                        channelp->read_avail + data_head;

                channelp->remote.window_size -= (uint32_t)(datalen -
                                                           data_head);
                _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                               "shrinking window size by %ld bytes to %u, "
                               "read_avail %ld",
                               (long) (datalen - data_head),
                               channelp->remote.window_size,
                               (long) channelp->read_avail));

                session->packAdd_channelp = channelp;

                /* Adjust the window based on the block we just freed */
libssh2_packet_add_jump_point1:
