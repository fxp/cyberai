            p->payload = LIBSSH2_ALLOC(session, total_num);
            if(!p->payload) {
                return LIBSSH2_ERROR_ALLOC;
            }
            p->total_num = total_num;
            /* init write pointer to start of payload buffer */
            p->wptr = p->payload;

            if(!encrypted || !CRYPT_FLAG_R(session, REQUIRES_FULL_PACKET)) {
                if(!etm && blocksize > 5) {
                    /* copy the data from index 5 to the end of
                     the blocksize from the temporary buffer to
                     the start of the decrypted buffer */
                    if(blocksize - 5 <= (int) total_num) {
                        memcpy(p->wptr, &block[5], blocksize - 5);
                        p->wptr += blocksize - 5; /* advance write pointer */
                        if(etm) {
                            /* advance past unencrypted packet length */
                            p->wptr += 4;
                        }
                    }
                    else {
                        if(p->payload)
                            LIBSSH2_FREE(session, p->payload);
                        return LIBSSH2_ERROR_OUT_OF_BOUNDARY;
                    }
                }

                /* init the data_num field to the number of bytes of
                 the package read so far */
                p->data_num = p->wptr - p->payload;

                /* we already dealt with a blocksize worth of data */
                if(!etm)
                    numbytes -= blocksize;
            }
            else {
                /* haven't started reading payload yet */
                p->data_num = 0;

                /* we already dealt with packet size worth of data */
                if(!encrypted)
                    numbytes -= 4;
            }
        }

        /* how much there is left to add to the current payload
           package */
        remainpack = p->total_num - p->data_num;

        if(numbytes > remainpack) {
            /* if we have more data in the buffer than what is going into this
               particular packet, we limit this round to this packet only */
            numbytes = remainpack;
        }

        if(encrypted && CRYPT_FLAG_R(session, REQUIRES_FULL_PACKET)) {
            if(numbytes < remainpack) {
                /* need a full packet before checking MAC */
                session->socket_block_directions |=
                LIBSSH2_SESSION_BLOCK_INBOUND;
                return LIBSSH2_ERROR_EAGAIN;
            }

            /* we have a full packet, now remove the size field from numbytes
               and total_num to process only the packet data */
            numbytes -= 4;
            p->total_num -= 4;
        }

        if(encrypted && !etm) {
            /* At the end of the incoming stream, there is a MAC,
               and we don't want to decrypt that since we need it
               "raw". We MUST however decrypt the padding data
               since it is used for the hash later on. */
            int skip = (remote_mac ? remote_mac->mac_len : 0) + auth_len;

            if(CRYPT_FLAG_R(session, INTEGRATED_MAC))
                /* This crypto method DOES need the MAC to go through
                   decryption so it can be authenticated. */
                skip = 0;

            /* if what we have plus numbytes is bigger than the
               total minus the skip margin, we should lower the
               amount to decrypt even more */
            if((p->data_num + numbytes) >= (p->total_num - skip)) {
                /* decrypt the entire rest of the package */
                numdecrypt = LIBSSH2_MAX(0,
                    (int)(p->total_num - skip) - (int)p->data_num);
                firstlast = LAST_BLOCK;
            }
            else {
                ssize_t frac;
                numdecrypt = numbytes;
                frac = numdecrypt % blocksize;
                if(frac) {
                    /* not an aligned amount of blocks, align it by reducing
                       the number of bytes processed this loop */
                    numdecrypt -= frac;
                    /* and make it no unencrypted data
                       after it */
                    numbytes = 0;
                }
                if(CRYPT_FLAG_R(session, INTEGRATED_MAC)) {
                    /* Make sure that we save enough bytes to make the last
