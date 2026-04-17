               Specifically, 'count' means how much data that have or will be
               asked for by the nodes that are already added to the linked
               list. Some of those read requests may not actually have been
               sent off successfully yet.

               If 'already' is very large it should be perfectly fine to have
               count set to 0 as then we don't have to ask for more data
               (right now).

               buffer_size*4 is just picked more or less out of the air. The
               idea is that when reading SFTP from a remote server, we send
               away multiple read requests guessing that the client will read
               more than only this 'buffer_size' amount of memory. So we ask
               for maximum buffer_size*4 amount of data so that we can return
               them very fast in subsequent calls.
            */

            recv_window = libssh2_channel_window_read_ex(sftp->channel,
                                                         NULL, NULL);
            if(max_read_ahead > recv_window) {
                /* more data will be asked for than what the window currently
                   allows, expand it! */

                rc = _libssh2_channel_receive_window_adjust(sftp->channel,
                                               (uint32_t)(max_read_ahead * 8),
                                               1, NULL);
                /* if this returns EAGAIN, we will get back to this function
                   at next call */
                assert(rc != LIBSSH2_ERROR_EAGAIN || !filep->data_left);
                assert(rc != LIBSSH2_ERROR_EAGAIN || !filep->eof);
                if(rc)
                    return rc;
            }
        }

        while(count > 0) {
            unsigned char *s;

            /* 25 = packet_len(4) + packet_type(1) + request_id(4) +
               handle_len(4) + offset(8) + count(4) */
            uint32_t packet_len = (uint32_t)(handle->handle_len + 25);
            uint32_t request_id;

            uint32_t size = (uint32_t)count;
            if(size < buffer_size)
                size = (uint32_t)buffer_size;
            if(size > MAX_SFTP_READ_SIZE)
                size = MAX_SFTP_READ_SIZE;

            chunk = LIBSSH2_ALLOC(session, packet_len +
                                  sizeof(struct sftp_pipeline_chunk));
            if(!chunk)
                return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                      "malloc fail for FXP_WRITE");

            chunk->offset = filep->offset_sent;
            chunk->len = size;
            chunk->lefttosend = packet_len;
            chunk->sent = 0;

            s = chunk->packet;

            _libssh2_store_u32(&s, packet_len - 4);
            *s++ = SSH_FXP_READ;
            request_id = sftp->request_id++;
            chunk->request_id = request_id;
            _libssh2_store_u32(&s, request_id);
            _libssh2_store_str(&s, handle->handle, handle->handle_len);
            _libssh2_store_u64(&s, filep->offset_sent);
            filep->offset_sent += size; /* advance offset at once */
            _libssh2_store_u32(&s, size);

            /* add this new entry LAST in the list */
            _libssh2_list_add(&handle->packet_list, &chunk->node);
            /* deduct the size we used, as we might have to create
               more packets */
            count -= LIBSSH2_MIN(size, count);
            _libssh2_debug((session, LIBSSH2_TRACE_SFTP,
                           "read request id %d sent (offset: %lu, size: %lu)",
                           request_id, (unsigned long)chunk->offset,
                           (unsigned long)chunk->len));
        }
        LIBSSH2_FALLTHROUGH();
    case libssh2_NB_state_sent:

        sftp->read_state = libssh2_NB_state_idle;

        /* move through the READ packets that haven't been sent and send as
           many as possible - remember that we don't block */
