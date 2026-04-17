            /* If we have less than a blocksize left, it is too
               little data to deal with, read more */
            ssize_t nread;

            /* move any remainder to the start of the buffer so
               that we can do a full refill */
            if(remainbuf) {
                memmove(p->buf, &p->buf[p->readidx], remainbuf);
                p->readidx = 0;
                p->writeidx = remainbuf;
            }
            else {
                /* nothing to move, just zero the indexes */
                p->readidx = p->writeidx = 0;
            }

            /* now read a big chunk from the network into the temp buffer */
            nread = LIBSSH2_RECV(session, &p->buf[remainbuf],
                                 PACKETBUFSIZE - remainbuf,
                                 LIBSSH2_SOCKET_RECV_FLAGS(session));
            if(nread <= 0) {
                /* check if this is due to EAGAIN and return the special
                   return code if so, error out normally otherwise */
                if((nread < 0) && (nread == -EAGAIN)) {
                    session->socket_block_directions |=
                        LIBSSH2_SESSION_BLOCK_INBOUND;
                    return LIBSSH2_ERROR_EAGAIN;
                }
                _libssh2_debug((session, LIBSSH2_TRACE_SOCKET,
                               "Error recving %ld bytes (got %ld)",
                               (long)(PACKETBUFSIZE - remainbuf),
                               (long)-nread));
                return LIBSSH2_ERROR_SOCKET_RECV;
            }
            _libssh2_debug((session, LIBSSH2_TRACE_SOCKET,
                           "Recved %ld/%ld bytes to %p+%ld", (long)nread,
                           (long)(PACKETBUFSIZE - remainbuf), (void *)p->buf,
                           (long)remainbuf));

            debugdump(session, "libssh2_transport_read() raw",
                      &p->buf[remainbuf], nread);
            /* advance write pointer */
            p->writeidx += nread;

            /* update remainbuf counter */
            remainbuf = p->writeidx - p->readidx;
        }

        /* how much data to deal with from the buffer */
        numbytes = remainbuf;

        if(!p->total_num) {
            size_t total_num; /* the number of bytes following the initial
                                 (5 bytes) packet length and padding length
                                 fields */

            /* packet length is not encrypted in encode-then-mac mode
               and we donøt need to decrypt first block */
            ssize_t required_size = etm ? 4 : blocksize;

            /* No payload package area allocated yet. To know the
               size of this payload, we need enough to decrypt the first
               blocksize data. */

            if(numbytes < required_size) {
                /* we can't act on anything less than blocksize, but this
                   check is only done for the initial block since once we have
                   got the start of a block we can in fact deal with fractions
                */
                session->socket_block_directions |=
                    LIBSSH2_SESSION_BLOCK_INBOUND;
                return LIBSSH2_ERROR_EAGAIN;
            }

            if(etm) {
                /* etm size field is not encrypted */
                memcpy(block, &p->buf[p->readidx], 4);
                memcpy(p->init, &p->buf[p->readidx], 4);
            }
            else if(encrypted && session->remote.crypt->get_len) {
                unsigned int len = 0;
                unsigned char *ptr = NULL;

                rc = session->remote.crypt->get_len(session,
                                            session->remote.seqno,
                                            &p->buf[p->readidx],
                                            numbytes,
                                            &len,
                                            &session->remote.crypt_abstract);

                if(rc != LIBSSH2_ERROR_NONE) {
                    p->total_num = 0;   /* no packet buffer available */
                    if(p->payload)
                        LIBSSH2_FREE(session, p->payload);
                    p->payload = NULL;
                    return rc;
                }

                /* store size in buffers for use below */
                ptr = &block[0];
                _libssh2_store_u32(&ptr, len);

