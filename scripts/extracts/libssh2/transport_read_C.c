                ptr = &p->init[0];
                _libssh2_store_u32(&ptr, len);
            }
            else {
                if(encrypted) {
                    /* first decrypted block */
                    rc = decrypt(session, &p->buf[p->readidx],
                                 block, blocksize, FIRST_BLOCK);
                    if(rc != LIBSSH2_ERROR_NONE) {
                        return rc;
                    }
                    /* Save the first 5 bytes of the decrypted package, to be
                       used in the hash calculation later down.
                       This is ignored in the INTEGRATED_MAC case. */
                    memcpy(p->init, block, 5);
                }
                else {
                    /* the data is plain, just copy it verbatim to
                       the working block buffer */
                    memcpy(block, &p->buf[p->readidx], blocksize);
                }

                /* advance the read pointer */
                p->readidx += blocksize;

                /* we now have the initial blocksize bytes decrypted,
                 * and we can extract packet and padding length from it
                 */
                p->packet_length = _libssh2_ntohu32(block);
            }

            if(!encrypted || !CRYPT_FLAG_R(session, REQUIRES_FULL_PACKET)) {
                if(p->packet_length < 1) {
                    return LIBSSH2_ERROR_DECRYPT;
                }
                else if(p->packet_length > LIBSSH2_PACKET_MAXPAYLOAD) {
                    return LIBSSH2_ERROR_OUT_OF_BOUNDARY;
                }

                if(etm) {
                    /* we collect entire undecrypted packet including the
                     packet length field that we run MAC over */
                    p->packet_length = _libssh2_ntohu32(block);
                    total_num = 4 + p->packet_length +
                    remote_mac->mac_len;
                }
                else {
                    /* padding_length has not been authenticated yet, but it
                     won't actually be used (except for the sanity check
                     immediately following) until after the entire packet is
                     authenticated, so this is safe. */
                    p->padding_length = block[4];
                    if(p->padding_length > p->packet_length - 1) {
                        return LIBSSH2_ERROR_DECRYPT;
                    }

                    /* total_num is the number of bytes following the initial
                     (5 bytes) packet length and padding length fields */
                    total_num = p->packet_length - 1 +
                    (encrypted ? remote_mac->mac_len : 0);
                }
            }
            else {
                /* advance the read pointer past size field if the packet
                 length is not required for decryption */

                /* add size field to be included in total packet size
                 * calculation so it doesn't get dropped off on subsequent
                 * partial reads
                 */
                total_num = 4;

                p->packet_length = _libssh2_ntohu32(block);
                if(p->packet_length < 1)
                    return LIBSSH2_ERROR_DECRYPT;

                /* total_num may include size field, however due to existing
                 * logic it needs to be removed after the entire packet is read
                 */

                total_num += p->packet_length +
                    (remote_mac ? remote_mac->mac_len : 0) + auth_len;

                /* don't know what padding is until we decrypt the full
                   packet */
                p->padding_length = 0;
            }

            /* RFC4253 section 6.1 Maximum Packet Length says:
             *
             * "All implementations MUST be able to process
             * packets with uncompressed payload length of 32768
             * bytes or less and total packet size of 35000 bytes
             * or less (including length, padding length, payload,
             * padding, and MAC.)."
             */
            if(total_num > LIBSSH2_PACKET_MAXPAYLOAD || total_num == 0) {
                return LIBSSH2_ERROR_OUT_OF_BOUNDARY;
            }

            /* Get a packet handle put data into. We get one to
               hold all data, including padding and MAC. */
