                       sizeof(session->packAdd_authagent_state));

libssh2_packet_add_jump_authagent:
                session->packAdd_state = libssh2_NB_state_jumpauthagent;
                rc = packet_authagent_open(session, data, datalen,
                                           &session->packAdd_authagent_state);
            }
            if(rc == LIBSSH2_ERROR_EAGAIN)
                return rc;

            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return rc;

            /*
              byte      SSH_MSG_CHANNEL_WINDOW_ADJUST
              uint32    recipient channel
              uint32    bytes to add
            */
        case SSH_MSG_CHANNEL_WINDOW_ADJUST:
            if(datalen < 9)
                ;
            else {
                uint32_t bytestoadd = _libssh2_ntohu32(data + 5);
                channelp =
                    _libssh2_channel_locate(session,
                                            _libssh2_ntohu32(data + 1));
                if(channelp) {
                    channelp->local.window_size += bytestoadd;

                    _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                                   "Window adjust for channel %u/%u, "
                                   "adding %u bytes, new window_size=%u",
                                   channelp->local.id,
                                   channelp->remote.id,
                                   bytestoadd,
                                   channelp->local.window_size));
                }
            }
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return 0;
        default:
            break;
        }

        session->packAdd_state = libssh2_NB_state_sent;
    }

    if(session->packAdd_state == libssh2_NB_state_sent) {
        LIBSSH2_PACKET *packetp =
            LIBSSH2_ALLOC(session, sizeof(LIBSSH2_PACKET));
        if(!packetp) {
            _libssh2_debug((session, LIBSSH2_ERROR_ALLOC,
                           "memory for packet"));
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return LIBSSH2_ERROR_ALLOC;
        }
        packetp->data = data;
        packetp->data_len = datalen;
        packetp->data_head = data_head;

        _libssh2_list_add(&session->packets, &packetp->node);

        session->packAdd_state = libssh2_NB_state_sent1;
    }

    if((msg == SSH_MSG_KEXINIT &&
         !(session->state & LIBSSH2_STATE_EXCHANGING_KEYS)) ||
        (session->packAdd_state == libssh2_NB_state_sent2)) {
        if(session->packAdd_state == libssh2_NB_state_sent1) {
            /*
             * Remote wants new keys
             * Well, it's already in the brigade,
             * let's just call back into ourselves
             */
            _libssh2_debug((session, LIBSSH2_TRACE_TRANS,
                           "Renegotiating Keys"));

            session->packAdd_state = libssh2_NB_state_sent2;
        }

        /*
         * The KEXINIT message has been added to the queue.  The packAdd and
         * readPack states need to be reset because _libssh2_kex_exchange
         * (eventually) calls upon _libssh2_transport_read to read the rest of
         * the key exchange conversation.
         */
        session->readPack_state = libssh2_NB_state_idle;
        session->packet.total_num = 0;
        session->packAdd_state = libssh2_NB_state_idle;
        session->fullpacket_state = libssh2_NB_state_idle;

        memset(&session->startup_key_state, 0, sizeof(key_exchange_state_t));

        /*
         * If there was a key reexchange failure, let's just hope we didn't
         * send NEWKEYS yet, otherwise remote will drop us like a rock
         */
        rc = _libssh2_kex_exchange(session, 1, &session->startup_key_state);
        if(rc == LIBSSH2_ERROR_EAGAIN)
            return rc;
    }

    session->packAdd_state = libssh2_NB_state_idle;
    return 0;
}
