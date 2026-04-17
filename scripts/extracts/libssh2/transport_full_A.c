{
    unsigned char macbuf[MAX_MACSIZE];
    struct transportpacket *p = &session->packet;
    int rc;
    int compressed;
    const LIBSSH2_MAC_METHOD *remote_mac = NULL;
    uint32_t seq = session->remote.seqno;

    if(!encrypted || (!CRYPT_FLAG_R(session, REQUIRES_FULL_PACKET) &&
                      !CRYPT_FLAG_R(session, INTEGRATED_MAC))) {
        remote_mac = session->remote.mac;
    }

    if(session->fullpacket_state == libssh2_NB_state_idle) {
        session->fullpacket_macstate = LIBSSH2_MAC_CONFIRMED;
        session->fullpacket_payload_len = p->packet_length - 1;

        if(encrypted && remote_mac) {

            /* Calculate MAC hash */
            int etm = remote_mac->etm;
            size_t mac_len = remote_mac->mac_len;
            if(etm) {
                /* store hash here */
                remote_mac->hash(session, macbuf,
                                 session->remote.seqno,
                                 p->payload, p->total_num - mac_len,
                                 NULL, 0,
                                 &session->remote.mac_abstract);
            }
            else {
                /* store hash here */
                remote_mac->hash(session, macbuf,
                                 session->remote.seqno,
                                 p->init, 5,
                                 p->payload,
                                 session->fullpacket_payload_len,
                                 &session->remote.mac_abstract);
            }

            /* Compare the calculated hash with the MAC we just read from
             * the network. The read one is at the very end of the payload
             * buffer. Note that 'payload_len' here is the packet_length
             * field which includes the padding but not the MAC.
             */
            if(memcmp(macbuf, p->payload + p->total_num - mac_len, mac_len)) {
                _libssh2_debug((session, LIBSSH2_TRACE_SOCKET,
                               "Failed MAC check"));
                session->fullpacket_macstate = LIBSSH2_MAC_INVALID;

            }
            else if(etm) {
                /* MAC was ok and we start by decrypting the first block that
                   contains padding length since this allows us to decrypt
                   all other blocks to the right location in memory
                   avoiding moving a larger block of memory one byte. */
                unsigned char first_block[MAX_BLOCKSIZE];
                ssize_t decrypt_size;
                unsigned char *decrypt_buffer;
                int blocksize = session->remote.crypt->blocksize;

                rc = decrypt(session, p->payload + 4,
                             first_block, blocksize, FIRST_BLOCK);
                if(rc) {
                    return rc;
                }

                /* we need buffer for decrypt */
                decrypt_size = p->total_num - mac_len - 4;
                decrypt_buffer = LIBSSH2_ALLOC(session, decrypt_size);
                if(!decrypt_buffer) {
                    return LIBSSH2_ERROR_ALLOC;
                }

                /* grab padding length and copy anything else
                   into target buffer */
                p->padding_length = first_block[0];
