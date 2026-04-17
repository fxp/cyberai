{
    int rc = 0;
    LIBSSH2_SESSION *session = channel->session;
    ssize_t wrote = 0; /* counter for this specific this call */

    /* In theory we could split larger buffers into several smaller packets
     * but it turns out to be really hard and nasty to do while still offering
     * the API/prototype.
     *
     * Instead we only deal with the first 32K in this call and for the parent
     * function to call it again with the remainder! 32K is a conservative
     * limit based on the text in RFC4253 section 6.1.
     */
    if(buflen > 32700)
        buflen = 32700;

    if(channel->write_state == libssh2_NB_state_idle) {
        unsigned char *s = channel->write_packet;

        _libssh2_debug((channel->session, LIBSSH2_TRACE_CONN,
                       "Writing %ld bytes on channel %u/%u, stream #%d",
                       (long)buflen, channel->local.id, channel->remote.id,
                       stream_id));

        if(channel->local.close)
            return _libssh2_error(channel->session,
                                  LIBSSH2_ERROR_CHANNEL_CLOSED,
                                  "We have already closed this channel");
        else if(channel->local.eof)
            return _libssh2_error(channel->session,
                                  LIBSSH2_ERROR_CHANNEL_EOF_SENT,
                                  "EOF has already been received, "
                                  "data might be ignored");

        /* drain the incoming flow first, mostly to make sure we get all
         * pending window adjust packets */
        do
            rc = _libssh2_transport_read(session);
        while(rc > 0);

        if((rc < 0) && (rc != LIBSSH2_ERROR_EAGAIN)) {
            return _libssh2_error(channel->session, rc,
                                  "Failure while draining incoming flow");
        }

        if(channel->local.window_size <= 0) {
            /* there's no room for data so we stop */

            /* Waiting on the socket to be writable would be wrong because we
             * would be back here immediately, but a readable socket might
             * herald an incoming window adjustment.
             */
            session->socket_block_directions = LIBSSH2_SESSION_BLOCK_INBOUND;

            return rc == LIBSSH2_ERROR_EAGAIN ? rc : 0;
        }

        channel->write_bufwrite = buflen;

        *(s++) = stream_id ? SSH_MSG_CHANNEL_EXTENDED_DATA :
            SSH_MSG_CHANNEL_DATA;
        _libssh2_store_u32(&s, channel->remote.id);
        if(stream_id)
            _libssh2_store_u32(&s, stream_id);

        /* Don't exceed the remote end's limits */
        /* REMEMBER local means local as the SOURCE of the data */
        if(channel->write_bufwrite > channel->local.window_size) {
            _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                           "Splitting write block due to %u byte "
                           "window_size on %u/%u/%d",
