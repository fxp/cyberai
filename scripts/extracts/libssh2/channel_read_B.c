        if((stream_id
             && (readpkt->data[0] == SSH_MSG_CHANNEL_EXTENDED_DATA)
             && (channel->local.id == channel->read_local_id)
             && (readpkt->data_len >= 9)
             && (stream_id == (int) _libssh2_ntohu32(readpkt->data + 5)))
            || (!stream_id && (readpkt->data[0] == SSH_MSG_CHANNEL_DATA)
                && (channel->local.id == channel->read_local_id))
            || (!stream_id
                && (readpkt->data[0] == SSH_MSG_CHANNEL_EXTENDED_DATA)
                && (channel->local.id == channel->read_local_id)
                && (channel->remote.extended_data_ignore_mode ==
                    LIBSSH2_CHANNEL_EXTENDED_DATA_MERGE))) {

            /* figure out much more data we want to read */
            bytes_want = buflen - bytes_read;
            unlink_packet = FALSE;

            if(bytes_want >= (readpkt->data_len - readpkt->data_head)) {
                /* we want more than this node keeps, so adjust the number and
                   delete this node after the copy */
                bytes_want = readpkt->data_len - readpkt->data_head;
                unlink_packet = TRUE;
            }

            _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                           "channel_read() got %ld of data from %u/%u/%d%s",
                           (long)bytes_want, channel->local.id,
                           channel->remote.id, stream_id,
                           unlink_packet ? " [ul]" : ""));

            /* copy data from this struct to the target buffer */
            memcpy(&buf[bytes_read],
                   &readpkt->data[readpkt->data_head], bytes_want);

            /* advance pointer and counter */
            readpkt->data_head += bytes_want;
            bytes_read += bytes_want;

            /* if drained, remove from list */
            if(unlink_packet) {
                /* detach readpkt from session->packets list */
                _libssh2_list_remove(&readpkt->node);

                LIBSSH2_FREE(session, readpkt->data);
                LIBSSH2_FREE(session, readpkt);
            }
        }

        /* check the next struct in the chain */
        read_packet = read_next;
    }

    if(!bytes_read) {
        /* If the channel is already at EOF or even closed, we need to signal
           that back. We may have gotten that info while draining the incoming
           transport layer until EAGAIN so we must not be fooled by that
           return code. */
        if(channel->remote.eof || channel->remote.close)
            return 0;
        else if(rc != LIBSSH2_ERROR_EAGAIN)
            return 0;

        /* if the transport layer said EAGAIN then we say so as well */
        return _libssh2_error(session, rc, "would block");
    }

    channel->read_avail -= bytes_read;
    channel->remote.window_size -= (uint32_t)bytes_read;

    return bytes_read;
}
