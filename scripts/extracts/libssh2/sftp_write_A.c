{
    LIBSSH2_SFTP *sftp = handle->sftp;
    LIBSSH2_CHANNEL *channel = sftp->channel;
    LIBSSH2_SESSION *session = channel->session;
    size_t data_len = 0;
    uint32_t retcode;
    uint32_t packet_len;
    unsigned char *s, *data = NULL;
    ssize_t rc;
    struct sftp_pipeline_chunk *chunk;
    struct sftp_pipeline_chunk *next;
    size_t acked = 0;
    size_t org_count = count;
    size_t already;

    switch(sftp->write_state) {
    default:
    case libssh2_NB_state_idle:
        sftp->last_errno = LIBSSH2_FX_OK;

        /* Number of bytes sent off that haven't been acked and therefore we
           will get passed in here again.

           Also, add up the number of bytes that actually already have been
           acked but we haven't been able to return as such yet, so we will
           get that data as well passed in here again.
        */
        already = (size_t)(handle->u.file.offset_sent -
                           handle->u.file.offset)+
            handle->u.file.acked;

        if(count >= already) {
            /* skip the part already made into packets */
            buffer += already;
            count -= already;
        }
        else
            /* there is more data already fine than what we got in this call */
            count = 0;

        sftp->write_state = libssh2_NB_state_idle;
        while(count) {
            /* TODO: Possibly this should have some logic to prevent a very
               very small fraction to be left but lets ignore that for now */
            uint32_t size =
                (uint32_t)(LIBSSH2_MIN(MAX_SFTP_OUTGOING_SIZE, count));
            uint32_t request_id;

            /* 25 = packet_len(4) + packet_type(1) + request_id(4) +
               handle_len(4) + offset(8) + count(4) */
            packet_len = (uint32_t)(handle->handle_len + size + 25);

            chunk = LIBSSH2_ALLOC(session, packet_len +
                                  sizeof(struct sftp_pipeline_chunk));
            if(!chunk)
                return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                      "malloc fail for FXP_WRITE");

            chunk->len = size;
            chunk->sent = 0;
            chunk->lefttosend = packet_len;

            s = chunk->packet;
            _libssh2_store_u32(&s, packet_len - 4);

            *(s++) = SSH_FXP_WRITE;
            request_id = sftp->request_id++;
            chunk->request_id = request_id;
            _libssh2_store_u32(&s, request_id);
            _libssh2_store_str(&s, handle->handle, handle->handle_len);
            _libssh2_store_u64(&s, handle->u.file.offset_sent);
            handle->u.file.offset_sent += size; /* advance offset at once */
            _libssh2_store_str(&s, buffer, size);

            /* add this new entry LAST in the list */
            _libssh2_list_add(&handle->packet_list, &chunk->node);

            buffer += size;
            count -= size; /* deduct the size we used, as we might have
                              to create more packets */
        }

        /* move through the WRITE packets that haven't been sent and send as
           many as possible - remember that we don't block */
        chunk = _libssh2_list_first(&handle->packet_list);

        while(chunk) {
            if(chunk->lefttosend) {
                rc = _libssh2_channel_write(channel, 0,
                                            &chunk->packet[chunk->sent],
                                            chunk->lefttosend);
                if(rc < 0)
                    /* remain in idle state */
                    return rc;

                /* remember where to continue sending the next time */
                chunk->lefttosend -= rc;
                chunk->sent += rc;

                if(chunk->lefttosend)
                    /* data left to send, get out of loop */
                    break;
            }

            /* move on to the next chunk with data to send */
