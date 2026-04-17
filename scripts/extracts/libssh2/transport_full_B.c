                if(blocksize > 1) {
                    memcpy(decrypt_buffer, first_block + 1, blocksize - 1);
                }

                /* decrypt all other blocks packet */
                if(blocksize < decrypt_size) {
                    rc = decrypt(session, p->payload + blocksize + 4,
                                 decrypt_buffer + blocksize - 1,
                                 decrypt_size - blocksize, LAST_BLOCK);
                    if(rc) {
                        LIBSSH2_FREE(session, decrypt_buffer);
                        return rc;
                    }
                }

                /* replace encrypted payload with plain text payload */
                LIBSSH2_FREE(session, p->payload);
                p->payload = decrypt_buffer;
            }
        }
        else if(encrypted && CRYPT_FLAG_R(session, REQUIRES_FULL_PACKET)) {
            /* etm trim off padding byte from payload */
            memmove(p->payload, &p->payload[1], p->packet_length - 1);
        }

        session->remote.seqno++;

        /* ignore the padding */
        session->fullpacket_payload_len -= p->padding_length;

        /* Check for and deal with decompression */
        compressed = session->local.comp &&
                     session->local.comp->compress &&
                     ((session->state & LIBSSH2_STATE_AUTHENTICATED) ||
                      session->local.comp->use_in_auth);

        if(compressed && session->remote.comp_abstract) {
            /*
             * The buffer for the decompression (remote.comp_abstract) is
             * initialised in time when it is needed so as long it is NULL we
             * cannot decompress.
             */

            unsigned char *data;
            size_t data_len;
            rc = session->remote.comp->decomp(session,
                                              &data, &data_len,
                                              LIBSSH2_PACKET_MAXDECOMP,
                                              p->payload,
                                              session->fullpacket_payload_len,
                                              &session->remote.comp_abstract);
            LIBSSH2_FREE(session, p->payload);
            if(rc)
                return rc;

            p->payload = data;
            session->fullpacket_payload_len = data_len;
        }

        session->fullpacket_packet_type = p->payload[0];

        debugdump(session, "libssh2_transport_read() plain",
                  p->payload, session->fullpacket_payload_len);

        session->fullpacket_state = libssh2_NB_state_created;
    }

    if(session->fullpacket_state == libssh2_NB_state_created) {
        rc = _libssh2_packet_add(session, p->payload,
                                 session->fullpacket_payload_len,
                                 session->fullpacket_macstate, seq);
        if(rc == LIBSSH2_ERROR_EAGAIN)
            return rc;
        if(rc) {
            session->fullpacket_state = libssh2_NB_state_idle;
            return rc;
        }
    }

    session->fullpacket_state = libssh2_NB_state_idle;

    if(session->kex_strict &&
        session->fullpacket_packet_type == SSH_MSG_NEWKEYS) {
        session->remote.seqno = 0;
    }

    return session->fullpacket_packet_type;
}
