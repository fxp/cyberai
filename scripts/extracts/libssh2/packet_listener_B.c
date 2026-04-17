                    channel->channel_type = LIBSSH2_ALLOC(session,
                                                          channel->
                                                          channel_type_len +
                                                          1);
                    if(!channel->channel_type) {
                        _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                       "Unable to allocate a channel for new"
                                       " connection");
                        LIBSSH2_FREE(session, channel);
                        failure_code = SSH_OPEN_RESOURCE_SHORTAGE;
                        listen_state->state = libssh2_NB_state_sent;
                        break;
                    }
                    memcpy(channel->channel_type, "forwarded-tcpip",
                           channel->channel_type_len + 1);

                    channel->remote.id = listen_state->sender_channel;
                    channel->remote.window_size_initial =
                        LIBSSH2_CHANNEL_WINDOW_DEFAULT;
                    channel->remote.window_size =
                        LIBSSH2_CHANNEL_WINDOW_DEFAULT;
                    channel->remote.packet_size =
                        LIBSSH2_CHANNEL_PACKET_DEFAULT;

                    channel->local.id = _libssh2_channel_nextid(session);
                    channel->local.window_size_initial =
                        listen_state->initial_window_size;
                    channel->local.window_size =
                        listen_state->initial_window_size;
                    channel->local.packet_size = listen_state->packet_size;

                    _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                                   "Connection queued: channel %u/%u "
                                   "win %u/%u packet %u/%u",
                                   channel->local.id, channel->remote.id,
                                   channel->local.window_size,
                                   channel->remote.window_size,
                                   channel->local.packet_size,
                                   channel->remote.packet_size));

                    p = listen_state->packet;
                    *(p++) = SSH_MSG_CHANNEL_OPEN_CONFIRMATION;
                    _libssh2_store_u32(&p, channel->remote.id);
                    _libssh2_store_u32(&p, channel->local.id);
                    _libssh2_store_u32(&p,
                                       channel->remote.window_size_initial);
                    _libssh2_store_u32(&p, channel->remote.packet_size);

                    listen_state->state = libssh2_NB_state_created;
                }

                if(listen_state->state == libssh2_NB_state_created) {
                    rc = _libssh2_transport_send(session, listen_state->packet,
                                                 17, NULL, 0);
                    if(rc == LIBSSH2_ERROR_EAGAIN)
                        return rc;
                    else if(rc) {
                        listen_state->state = libssh2_NB_state_idle;
                        return _libssh2_error(session, rc,
                                              "Unable to send channel "
                                              "open confirmation");
                    }

                    /* Link the channel into the end of the queue list */
                    if(listen_state->channel) {
                        _libssh2_list_add(&listn->queue,
                                          &listen_state->channel->node);
                        listn->queue_size++;
                    }

                    listen_state->state = libssh2_NB_state_idle;
                    return 0;
                }
            }

            listn = _libssh2_list_next(&listn->node);
        }

        listen_state->state = libssh2_NB_state_sent;
    }

    /* We're not listening to you */
    p = listen_state->packet;
    *(p++) = SSH_MSG_CHANNEL_OPEN_FAILURE;
    _libssh2_store_u32(&p, listen_state->sender_channel);
    _libssh2_store_u32(&p, failure_code);
    _libssh2_store_str(&p, FwdNotReq, strlen(FwdNotReq));
    _libssh2_htonu32(p, 0);

    rc = _libssh2_transport_send(session, listen_state->packet,
                                 packet_len, NULL, 0);
    if(rc == LIBSSH2_ERROR_EAGAIN) {
        return rc;
    }
    else if(rc) {
        listen_state->state = libssh2_NB_state_idle;
        return _libssh2_error(session, rc, "Unable to send open failure");

    }
    listen_state->state = libssh2_NB_state_idle;
    return 0;
