                                       (char *)&sftp->packet_header[
                                           sftp->packet_header_len],
                                       sizeof(sftp->packet_header) -
                                       sftp->packet_header_len);
            if(rc == LIBSSH2_ERROR_EAGAIN)
                return (int)rc;
            else if(rc < 0)
                return _libssh2_error(session, (int)rc, "channel read");

            sftp->packet_header_len += rc;

            if(sftp->packet_header_len != sizeof(sftp->packet_header))
                /* we got a short read for the header part */
                return LIBSSH2_ERROR_EAGAIN;

            /* parse SFTP packet header */
            sftp->partial_len = _libssh2_ntohu32(sftp->packet_header);
            packet_type = sftp->packet_header[4];
            request_id = _libssh2_ntohu32(sftp->packet_header + 5);

            /* make sure we don't proceed if the packet size is unreasonably
               large */
            if(sftp->partial_len > LIBSSH2_SFTP_PACKET_MAXLEN &&
               /* exception: response to SSH_FXP_READDIR request */
               !(sftp->readdir_state != libssh2_NB_state_idle &&
                 sftp->readdir_request_id == request_id &&
                 packet_type == SSH_FXP_NAME)) {
                libssh2_channel_flush(channel);
                sftp->packet_header_len = 0;
                return _libssh2_error(session,
                                      LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED,
                                      "SFTP packet too large");
            }

            if(sftp->partial_len < 5)
                return _libssh2_error(session,
                                      LIBSSH2_ERROR_ALLOC,
                                      "Invalid SFTP packet size");

            _libssh2_debug((session, LIBSSH2_TRACE_SFTP,
                           "Data begin - Packet Length: %lu",
                           (unsigned long)sftp->partial_len));
            packet = LIBSSH2_ALLOC(session, sftp->partial_len);
            if(!packet)
                return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                      "Unable to allocate SFTP packet");
            sftp->packet_header_len = 0;
            sftp->partial_packet = packet;
            /* copy over packet type(4) and request id(1) */
            sftp->partial_received = 5;
            memcpy(packet, sftp->packet_header + 4, 5);

window_adjust:
            recv_window = libssh2_channel_window_read_ex(channel, NULL, NULL);

            if(sftp->partial_len > recv_window) {
                /* ask for twice the data amount we need at once */
                rc = _libssh2_channel_receive_window_adjust(channel,
                                                            sftp->partial_len
                                                            * 2,
                                                            1, NULL);
                /* store the state so that we continue with the correct
                   operation at next invoke */
                sftp->packet_state = (rc == LIBSSH2_ERROR_EAGAIN) ?
                    libssh2_NB_state_sent :
                    libssh2_NB_state_idle;

                if(rc == LIBSSH2_ERROR_EAGAIN)
                    return (int)rc;
            }
        }

        /* Read as much of the packet as we can */
        while(sftp->partial_len > sftp->partial_received) {
            rc = _libssh2_channel_read(channel, 0,
                                       (char *)&packet[sftp->partial_received],
                                       sftp->partial_len -
                                       sftp->partial_received);

            if(rc == LIBSSH2_ERROR_EAGAIN) {
                /*
                 * We received EAGAIN, save what we have and return EAGAIN to
                 * the caller. Set 'partial_packet' so that this function
                 * knows how to continue on the next invoke.
                 */
                sftp->packet_state = libssh2_NB_state_sent1;
                return (int)rc;
            }
            else if(rc < 0) {
                LIBSSH2_FREE(session, packet);
                sftp->partial_packet = NULL;
                return _libssh2_error(session, (int)rc,
                                      "Error waiting for SFTP packet");
            }
            sftp->partial_received += rc;
        }

        sftp->partial_packet = NULL;

        /* sftp_packet_add takes ownership of the packet and might free it
           so we take a copy of the packet type before we call it. */
        packet_type = packet[0];
        rc = sftp_packet_add(sftp, packet, sftp->partial_len);
        if(rc) {
            LIBSSH2_FREE(session, packet);
            return (int)rc;
        }
        else {
            return packet_type;
        }
    }
    /* WON'T REACH */
}
