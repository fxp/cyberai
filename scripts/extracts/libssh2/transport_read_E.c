                     * block large enough to hold the entire integrated MAC */
                    numdecrypt = LIBSSH2_MIN(numdecrypt,
                        (int)(p->total_num - skip - blocksize - p->data_num));
                    numbytes = 0;
                }
                firstlast = MIDDLE_BLOCK;
            }
        }
        else {
            /* unencrypted data should not be decrypted at all */
            numdecrypt = 0;
        }
        assert(numdecrypt >= 0);

        /* if there are bytes to decrypt, do that */
        if(numdecrypt > 0) {
            /* now decrypt the lot */
            if(CRYPT_FLAG_R(session, REQUIRES_FULL_PACKET)) {
                rc = session->remote.crypt->crypt(session,
                                               session->remote.seqno,
                                               &p->buf[p->readidx],
                                               numdecrypt,
                                               &session->remote.crypt_abstract,
                                               0);

                if(rc != LIBSSH2_ERROR_NONE) {
                    p->total_num = 0;   /* no packet buffer available */
                    return rc;
                }

                memcpy(p->wptr, &p->buf[p->readidx], numbytes);

                /* advance read index past size field now that we've decrypted
                   full packet */
                p->readidx += 4;

                /* include auth tag in bytes decrypted */
                numdecrypt += auth_len;

                /* set padding now that the packet has been verified and
                   decrypted */
                p->padding_length = p->wptr[0];

                if(p->padding_length > p->packet_length - 1) {
                    return LIBSSH2_ERROR_DECRYPT;
                }
            }
            else {
                rc = decrypt(session, &p->buf[p->readidx], p->wptr, numdecrypt,
                             firstlast);

                if(rc != LIBSSH2_ERROR_NONE) {
                    p->total_num = 0;   /* no packet buffer available */
                    return rc;
                }
            }

            /* advance the read pointer */
            p->readidx += numdecrypt;
            /* advance write pointer */
            p->wptr += numdecrypt;
            /* increase data_num */
            p->data_num += numdecrypt;

            /* bytes left to take care of without decryption */
            numbytes -= numdecrypt;
        }

        /* if there are bytes to copy that aren't decrypted,
           copy them as-is to the target buffer */
        if(numbytes > 0) {

            if((size_t)numbytes <= (p->total_num - (p->wptr - p->payload))) {
                memcpy(p->wptr, &p->buf[p->readidx], numbytes);
            }
            else {
                if(p->payload)
                    LIBSSH2_FREE(session, p->payload);
                return LIBSSH2_ERROR_OUT_OF_BOUNDARY;
            }

            /* advance the read pointer */
            p->readidx += numbytes;
            /* advance write pointer */
            p->wptr += numbytes;
            /* increase data_num */
            p->data_num += numbytes;
        }

        /* now check how much data there's left to read to finish the
           current packet */
        remainpack = p->total_num - p->data_num;

        if(!remainpack) {
            /* we have a full packet */
libssh2_transport_read_point1:
            rc = fullpacket(session, encrypted);
            if(rc == LIBSSH2_ERROR_EAGAIN) {

                if(session->packAdd_state != libssh2_NB_state_idle) {
                    /* fullpacket only returns LIBSSH2_ERROR_EAGAIN if
                     * libssh2_packet_add() returns LIBSSH2_ERROR_EAGAIN. If
                     * that returns LIBSSH2_ERROR_EAGAIN but the packAdd_state
                     * is idle, then the packet has been added to the brigade,
                     * but some immediate action that was taken based on the
                     * packet type (such as key re-exchange) is not yet
                     * complete.  Clear the way for a new packet to be read
                     * in.
                     */
                    session->readPack_encrypted = encrypted;
                    session->readPack_state = libssh2_NB_state_jump1;
                }

                return rc;
            }

            p->total_num = 0;   /* no packet buffer available */

            return rc;
        }
    } while(1);                /* loop */

    return LIBSSH2_ERROR_SOCKET_RECV; /* we never reach this point */
}
