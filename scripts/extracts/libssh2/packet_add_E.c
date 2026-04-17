                session->packAdd_state = libssh2_NB_state_jump1;
                rc = _libssh2_channel_receive_window_adjust(session->
                                                            packAdd_channelp,
                                                    (uint32_t)(datalen - 13),
                                                            1, NULL);
                if(rc == LIBSSH2_ERROR_EAGAIN)
                    return rc;

                session->packAdd_state = libssh2_NB_state_idle;
                return 0;
            }

            /*
             * REMEMBER! remote means remote as source of data,
             * NOT remote window!
             */
            if(channelp->remote.packet_size < (datalen - data_head)) {
                /*
                 * Spec says we MAY ignore bytes sent beyond
                 * packet_size
                 */
                _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED,
                               "Packet contains more data than we offered"
                               " to receive, truncating");
                datalen = channelp->remote.packet_size + data_head;
            }
            if(channelp->remote.window_size <= channelp->read_avail) {
                /*
                 * Spec says we MAY ignore bytes sent beyond
                 * window_size
                 */
                _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED,
                               "The current receive window is full,"
                               " data ignored");
                LIBSSH2_FREE(session, data);
                session->packAdd_state = libssh2_NB_state_idle;
                return 0;
            }
            /* Reset EOF status */
            channelp->remote.eof = 0;

            if(channelp->read_avail + datalen - data_head >
                channelp->remote.window_size) {
                _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED,
                               "Remote sent more data than current "
                               "window allows, truncating");
                datalen = channelp->remote.window_size -
                    channelp->read_avail + data_head;
            }

            /* Update the read_avail counter. The window size will be
             * updated once the data is actually read from the queue
             * from an upper layer */
            channelp->read_avail += datalen - data_head;

            _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                           "increasing read_avail by %ld bytes to %ld/%u",
                           (long)(datalen - data_head),
                           (long)channelp->read_avail,
                           channelp->remote.window_size));

            break;

            /*
              byte      SSH_MSG_CHANNEL_EOF
              uint32    recipient channel
            */

        case SSH_MSG_CHANNEL_EOF:
            if(datalen >= 5)
                channelp =
                    _libssh2_channel_locate(session,
                                            _libssh2_ntohu32(data + 1));
            if(!channelp)
                /* We may have freed already, just quietly ignore this... */
                ;
            else {
                _libssh2_debug((session,
                               LIBSSH2_TRACE_CONN,
                               "EOF received for channel %u/%u",
                               channelp->local.id,
                               channelp->remote.id));
                channelp->remote.eof = 1;
            }
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return 0;

            /*
              byte      SSH_MSG_CHANNEL_REQUEST
              uint32    recipient channel
