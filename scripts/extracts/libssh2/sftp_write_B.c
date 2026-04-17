            chunk = _libssh2_list_next(&chunk->node);
        }

        LIBSSH2_FALLTHROUGH();

    case libssh2_NB_state_sent:

        sftp->write_state = libssh2_NB_state_idle;
        /*
         * Count all ACKed packets
         */
        chunk = _libssh2_list_first(&handle->packet_list);

        while(chunk) {
            if(chunk->lefttosend)
                /* if the chunk still has data left to send, we shouldn't wait
                   for an ACK for it just yet */
                break;

            else if(acked)
                /* if we have sent data that is acked, we must return that
                   info before we call a function that might return EAGAIN */
                break;

            /* we check the packets in order */
            rc = sftp_packet_require(sftp, SSH_FXP_STATUS,
                                     chunk->request_id, &data, &data_len, 9);
            if(rc == LIBSSH2_ERROR_BUFFER_TOO_SMALL) {
                if(data_len > 0) {
                    LIBSSH2_FREE(session, data);
                }
                return _libssh2_error(session, LIBSSH2_ERROR_SFTP_PROTOCOL,
                                      "FXP write packet too short");
            }
            else if(rc < 0) {
                if(rc == LIBSSH2_ERROR_EAGAIN)
                    sftp->write_state = libssh2_NB_state_sent;
                return rc;
            }

            retcode = _libssh2_ntohu32(data + 5);
            LIBSSH2_FREE(session, data);

            sftp->last_errno = retcode;
            if(retcode == LIBSSH2_FX_OK) {
                acked += chunk->len; /* number of payload data that was acked
                                        here */

                /* we increase the offset value for all acks */
                handle->u.file.offset += chunk->len;

                next = _libssh2_list_next(&chunk->node);

                _libssh2_list_remove(&chunk->node); /* remove from list */
                LIBSSH2_FREE(session, chunk); /* free memory */

                chunk = next;
            }
            else {
                /* flush all pending packets from the outgoing list */
                sftp_packetlist_flush(handle);

                /* since we return error now, the application will not get any
                   outstanding data acked, so we need to rewind the offset to
                   where the application knows it has reached with acked
                   data */
                handle->u.file.offset -= handle->u.file.acked;

                /* then reset the offset_sent to be the same as the offset */
                handle->u.file.offset_sent = handle->u.file.offset;

                /* clear the acked counter since we can have no pending data to
                   ack after an error */
                handle->u.file.acked = 0;

                /* the server returned an error for that written chunk,
                   propagate this back to our parent function */
                return _libssh2_error(session, LIBSSH2_ERROR_SFTP_PROTOCOL,
                                      "FXP write failed");
            }
        }
        break;
    }

    /* if there were acked data in a previous call that wasn't returned then,
       add that up and try to return it all now. This can happen if the app
       first sends a huge buffer of data, and then in a second call it sends a
       smaller one. */
    acked += handle->u.file.acked;

    if(acked) {
        ssize_t ret = LIBSSH2_MIN(acked, org_count);
        /* we got data acked so return that amount, but no more than what
           was asked to get sent! */

        /* store the remainder. 'ret' is always equal to or less than 'acked'
           here */
        handle->u.file.acked = acked - ret;

        return ret;
    }

    else
        return 0; /* nothing was acked, and no EAGAIN was received! */
}
