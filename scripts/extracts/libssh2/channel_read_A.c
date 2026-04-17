{
    LIBSSH2_SESSION *session = channel->session;
    int rc;
    size_t bytes_read = 0;
    size_t bytes_want;
    int unlink_packet;
    LIBSSH2_PACKET *read_packet;
    LIBSSH2_PACKET *read_next;

    _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                   "channel_read() wants %ld bytes from channel %u/%u "
                   "stream #%d",
                   (long)buflen, channel->local.id, channel->remote.id,
                   stream_id));

    /* expand the receiving window first if it has become too narrow */
    if((channel->read_state == libssh2_NB_state_jump1) ||
       (channel->remote.window_size <
        channel->remote.window_size_initial / 4 * 3 + buflen)) {

        uint32_t adjustment = (uint32_t)(channel->remote.window_size_initial +
            buflen - channel->remote.window_size);
        if(adjustment < LIBSSH2_CHANNEL_MINADJUST)
            adjustment = LIBSSH2_CHANNEL_MINADJUST;

        /* the actual window adjusting may not finish so we need to deal with
           this special state here */
        channel->read_state = libssh2_NB_state_jump1;
        rc = _libssh2_channel_receive_window_adjust(channel, adjustment,
                                                    0, NULL);
        if(rc)
            return rc;

        channel->read_state = libssh2_NB_state_idle;
    }

    /* Process all pending incoming packets. Tests prove that this way
       produces faster transfers. */
    do {
        rc = _libssh2_transport_read(session);
    } while(rc > 0);

    if((rc < 0) && (rc != LIBSSH2_ERROR_EAGAIN))
        return _libssh2_error(session, rc, "transport read");

    read_packet = _libssh2_list_first(&session->packets);
    while(read_packet && (bytes_read < buflen)) {
        /* previously this loop condition also checked for
           !channel->remote.close but we cannot let it do this:

           We may have a series of packets to read that are still pending even
           if a close has been received. Acknowledging the close too early
           makes us flush buffers prematurely and loose data.
        */

        LIBSSH2_PACKET *readpkt = read_packet;

        /* In case packet gets destroyed during this iteration */
        read_next = _libssh2_list_next(&readpkt->node);

        if(readpkt->data_len < 5) {
            read_packet = read_next;

            if(readpkt->data_len != 1 ||
                readpkt->data[0] != SSH_MSG_REQUEST_FAILURE) {
                _libssh2_debug((channel->session, LIBSSH2_TRACE_ERROR,
                               "Unexpected packet length"));
            }

            continue;
        }

        channel->read_local_id =
            _libssh2_ntohu32(readpkt->data + 1);

        /*
         * Either we asked for a specific extended data stream
         * (and data was available),
         * or the standard stream (and data was available),
         * or the standard stream with extended_data_merge
         * enabled and data was available
         */
