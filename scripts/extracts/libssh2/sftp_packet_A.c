{
    LIBSSH2_SESSION *session = sftp->channel->session;
    LIBSSH2_SFTP_PACKET *packet;
    uint32_t request_id;

    if(data_len < 5) {
        return LIBSSH2_ERROR_OUT_OF_BOUNDARY;
    }

    _libssh2_debug((session, LIBSSH2_TRACE_SFTP,
                   "Received packet type %u (len %lu)",
                   (unsigned int)data[0], (unsigned long)data_len));

    /*
     * Experience shows that if we mess up EAGAIN handling somewhere or
     * otherwise get out of sync with the channel, this is where we first get
     * a wrong byte and if so we need to bail out at once to aid tracking the
     * problem better.
     */

    switch(data[0]) {
    case SSH_FXP_INIT:
    case SSH_FXP_VERSION:
    case SSH_FXP_OPEN:
    case SSH_FXP_CLOSE:
    case SSH_FXP_READ:
    case SSH_FXP_WRITE:
    case SSH_FXP_LSTAT:
    case SSH_FXP_FSTAT:
    case SSH_FXP_SETSTAT:
    case SSH_FXP_FSETSTAT:
    case SSH_FXP_OPENDIR:
    case SSH_FXP_READDIR:
    case SSH_FXP_REMOVE:
    case SSH_FXP_MKDIR:
    case SSH_FXP_RMDIR:
    case SSH_FXP_REALPATH:
    case SSH_FXP_STAT:
    case SSH_FXP_RENAME:
    case SSH_FXP_READLINK:
    case SSH_FXP_SYMLINK:
    case SSH_FXP_STATUS:
    case SSH_FXP_HANDLE:
    case SSH_FXP_DATA:
    case SSH_FXP_NAME:
    case SSH_FXP_ATTRS:
    case SSH_FXP_EXTENDED:
    case SSH_FXP_EXTENDED_REPLY:
        break;
    default:
        sftp->last_errno = LIBSSH2_FX_OK;
        return _libssh2_error(session, LIBSSH2_ERROR_SFTP_PROTOCOL,
                              "Out of sync with the world");
    }

    request_id = _libssh2_ntohu32(&data[1]);

    _libssh2_debug((session, LIBSSH2_TRACE_SFTP, "Received packet id %d",
                   request_id));

    /* Don't add the packet if it answers a request we've given up on. */
    if((data[0] == SSH_FXP_STATUS || data[0] == SSH_FXP_DATA)
       && find_zombie_request(sftp, request_id)) {

        /* If we get here, the file ended before the response arrived. We
           are no longer interested in the request so we discard it */

        LIBSSH2_FREE(session, data);

        remove_zombie_request(sftp, request_id);
        return LIBSSH2_ERROR_NONE;
    }

    packet = LIBSSH2_ALLOC(session, sizeof(LIBSSH2_SFTP_PACKET));
    if(!packet) {
        return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                              "Unable to allocate datablock for SFTP packet");
    }

    packet->data = data;
    packet->data_len = data_len;
    packet->request_id = request_id;

    _libssh2_list_add(&sftp->packets, &packet->node);

    return LIBSSH2_ERROR_NONE;
}

/* sftp_packet_read
 * Frame an SFTP packet off the channel
 */
static int
sftp_packet_read(LIBSSH2_SFTP *sftp)
{
    LIBSSH2_CHANNEL *channel = sftp->channel;
    LIBSSH2_SESSION *session = channel->session;
    unsigned char *packet = NULL;
    ssize_t rc;
    unsigned long recv_window;
    int packet_type;
    uint32_t request_id;

    _libssh2_debug((session, LIBSSH2_TRACE_SFTP, "recv packet"));

    switch(sftp->packet_state) {
    case libssh2_NB_state_sent: /* EAGAIN from window adjusting */
        sftp->packet_state = libssh2_NB_state_idle;

        packet = sftp->partial_packet;
        goto window_adjust;

    case libssh2_NB_state_sent1: /* EAGAIN from channel read */
        sftp->packet_state = libssh2_NB_state_idle;

        packet = sftp->partial_packet;

        _libssh2_debug((session, LIBSSH2_TRACE_SFTP,
                       "partial read cont, len: %u", sftp->partial_len));
        _libssh2_debug((session, LIBSSH2_TRACE_SFTP,
                       "partial read cont, already recvd: %lu",
                       (unsigned long)sftp->partial_received));
        LIBSSH2_FALLTHROUGH();
    default:
        if(!packet) {
            /* only do this if there's not already a packet buffer allocated
               to use */

            /* each packet starts with a 32 bit length field */
            rc = _libssh2_channel_read(channel, 0,
