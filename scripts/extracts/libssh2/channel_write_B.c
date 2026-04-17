                           channel->local.window_size, channel->local.id,
                           channel->remote.id, stream_id));
            channel->write_bufwrite = channel->local.window_size;
        }
        if(channel->write_bufwrite > channel->local.packet_size) {
            _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                           "Splitting write block due to %u byte "
                           "packet_size on %u/%u/%d",
                           channel->local.packet_size, channel->local.id,
                           channel->remote.id, stream_id));
            channel->write_bufwrite = channel->local.packet_size;
        }
        /* store the size here only, the buffer is passed in as-is to
           _libssh2_transport_send() */
        _libssh2_store_u32(&s, (uint32_t)channel->write_bufwrite);
        channel->write_packet_len = s - channel->write_packet;

        _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                       "Sending %ld bytes on channel %u/%u, stream_id=%d",
                       (long)channel->write_bufwrite, channel->local.id,
                       channel->remote.id, stream_id));

        channel->write_state = libssh2_NB_state_created;
    }

    if(channel->write_state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session, channel->write_packet,
                                     channel->write_packet_len,
                                     buf, channel->write_bufwrite);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return _libssh2_error(session, rc,
                                  "Unable to send channel data");
        }
        else if(rc) {
            channel->write_state = libssh2_NB_state_idle;
            return _libssh2_error(session, rc,
                                  "Unable to send channel data");
        }
        /* Shrink local window size */
        channel->local.window_size -= (uint32_t)channel->write_bufwrite;

        wrote += channel->write_bufwrite;

        /* Since _libssh2_transport_write() succeeded, we must return
           now to allow the caller to provide the next chunk of data.

           We cannot move on to send the next piece of data that may
           already have been provided in this same function call, as we
           risk getting EAGAIN for that and we can't return information
           both about sent data as well as EAGAIN. So, by returning short
           now, the caller will call this function again with new data to
           send */

        channel->write_state = libssh2_NB_state_idle;

        return wrote;
    }

    return LIBSSH2_ERROR_INVAL; /* reaching this point is really bad */
}
