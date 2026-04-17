{
    /*
     * Look for a matching listener
     */
    /* 17 = packet_type(1) + channel(4) + reason(4) + descr(4) + lang(4) */
    size_t packet_len = 17 + strlen(FwdNotReq);
    unsigned char *p;
    LIBSSH2_LISTENER *listn = _libssh2_list_first(&session->listeners);
    char failure_code = SSH_OPEN_ADMINISTRATIVELY_PROHIBITED;
    int rc;

    if(listen_state->state == libssh2_NB_state_idle) {
        size_t offset = strlen("forwarded-tcpip") + 5;
        size_t temp_len = 0;
        struct string_buf buf;
        buf.data = data;
        buf.dataptr = buf.data;
        buf.len = datalen;

        if(datalen < offset) {
            return _libssh2_error(session, LIBSSH2_ERROR_OUT_OF_BOUNDARY,
                                  "Unexpected packet size");
        }

        buf.dataptr += offset;

        if(_libssh2_get_u32(&buf, &(listen_state->sender_channel))) {
            return _libssh2_error(session, LIBSSH2_ERROR_BUFFER_TOO_SMALL,
                                  "Data too short extracting channel");
        }
        if(_libssh2_get_u32(&buf, &(listen_state->initial_window_size))) {
            return _libssh2_error(session, LIBSSH2_ERROR_BUFFER_TOO_SMALL,
                                  "Data too short extracting window size");
        }
        if(_libssh2_get_u32(&buf, &(listen_state->packet_size))) {
            return _libssh2_error(session, LIBSSH2_ERROR_BUFFER_TOO_SMALL,
                                  "Data too short extracting packet");
        }
        if(_libssh2_get_string(&buf, &(listen_state->host), &temp_len)) {
            return _libssh2_error(session, LIBSSH2_ERROR_BUFFER_TOO_SMALL,
                                  "Data too short extracting host");
        }
        listen_state->host_len = (uint32_t)temp_len;

        if(_libssh2_get_u32(&buf, &(listen_state->port))) {
            return _libssh2_error(session, LIBSSH2_ERROR_BUFFER_TOO_SMALL,
                                  "Data too short extracting port");
        }
        if(_libssh2_get_string(&buf, &(listen_state->shost), &temp_len)) {
            return _libssh2_error(session, LIBSSH2_ERROR_BUFFER_TOO_SMALL,
                                  "Data too short extracting shost");
        }
        listen_state->shost_len = (uint32_t)temp_len;

        if(_libssh2_get_u32(&buf, &(listen_state->sport))) {
            return _libssh2_error(session, LIBSSH2_ERROR_BUFFER_TOO_SMALL,
                                  "Data too short extracting sport");
        }

        _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                       "Remote received connection from %s:%u to %s:%u",
                       listen_state->shost, listen_state->sport,
                       listen_state->host, listen_state->port));

        listen_state->state = libssh2_NB_state_allocated;
    }

    if(listen_state->state != libssh2_NB_state_sent) {
        while(listn) {
            if((listn->port == (int) listen_state->port) &&
                (strlen(listn->host) == listen_state->host_len) &&
                (memcmp(listn->host, listen_state->host,
                        listen_state->host_len) == 0)) {
                /* This is our listener */
                LIBSSH2_CHANNEL *channel = NULL;
                listen_state->channel = NULL;

                if(listen_state->state == libssh2_NB_state_allocated) {
                    if(listn->queue_maxsize &&
                        (listn->queue_maxsize <= listn->queue_size)) {
                        /* Queue is full */
                        failure_code = SSH_OPEN_RESOURCE_SHORTAGE;
                        _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                                       "Listener queue full, ignoring"));
                        listen_state->state = libssh2_NB_state_sent;
                        break;
                    }

                    channel = LIBSSH2_CALLOC(session, sizeof(LIBSSH2_CHANNEL));
                    if(!channel) {
                        _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                       "Unable to allocate a channel for "
                                       "new connection");
                        failure_code = SSH_OPEN_RESOURCE_SHORTAGE;
                        listen_state->state = libssh2_NB_state_sent;
                        break;
                    }
                    listen_state->channel = channel;

                    channel->session = session;
                    channel->channel_type_len = strlen("forwarded-tcpip");
