{
    int rc;
    struct transportpacket *p = &session->packet;
    ssize_t remainpack; /* how much there is left to add to the current payload
                           package */
    ssize_t remainbuf;  /* how much data there is remaining in the buffer to
                           deal with before we should read more from the
                           network */
    ssize_t numbytes;   /* how much data to deal with from the buffer on this
                           iteration through the loop */
    ssize_t numdecrypt; /* number of bytes to decrypt this iteration */
    unsigned char block[MAX_BLOCKSIZE]; /* working block buffer */
    int blocksize;  /* minimum number of bytes we need before we can
                       use them */
    int encrypted = 1; /* whether the packet is encrypted or not */
    int firstlast = FIRST_BLOCK; /* if the first or last block to decrypt */
    unsigned int auth_len = 0; /* length of the authentication tag */
    const LIBSSH2_MAC_METHOD *remote_mac = NULL; /* The remote MAC, if used */

    /* default clear the bit */
    session->socket_block_directions &= ~LIBSSH2_SESSION_BLOCK_INBOUND;

    /*
     * All channels, systems, subsystems, etc eventually make it down here
     * when looking for more incoming data. If a key exchange is going on
     * (LIBSSH2_STATE_EXCHANGING_KEYS bit is set) then the remote end will
     * ONLY send key exchange related traffic. In non-blocking mode, there is
     * a chance to break out of the kex_exchange function with an EAGAIN
     * status, and never come back to it. If LIBSSH2_STATE_EXCHANGING_KEYS is
     * active, then we must redirect to the key exchange. However, if
     * kex_exchange is active (as in it is the one that calls this execution
     * of packet_read, then don't redirect, as that would be an infinite loop!
     */

    if(session->state & LIBSSH2_STATE_EXCHANGING_KEYS &&
        !(session->state & LIBSSH2_STATE_KEX_ACTIVE)) {

        /* Whoever wants a packet won't get anything until the key re-exchange
         * is done!
         */
        _libssh2_debug((session, LIBSSH2_TRACE_TRANS, "Redirecting into the"
                       " key re-exchange from _libssh2_transport_read"));
        rc = _libssh2_kex_exchange(session, 1, &session->startup_key_state);
        if(rc)
            return rc;
    }

    /*
     * =============================== NOTE ===============================
     * I know this is very ugly and not a really good use of "goto", but
     * this case statement would be even uglier to do it any other way
     */
    if(session->readPack_state == libssh2_NB_state_jump1) {
        session->readPack_state = libssh2_NB_state_idle;
        encrypted = session->readPack_encrypted;
        goto libssh2_transport_read_point1;
    }

    do {
        int etm;
        if(session->socket_state == LIBSSH2_SOCKET_DISCONNECTED) {
            return LIBSSH2_ERROR_SOCKET_DISCONNECT;
        }

        if(session->state & LIBSSH2_STATE_NEWKEYS) {
            blocksize = session->remote.crypt->blocksize;
        }
        else {
            encrypted = 0;      /* not encrypted */
            blocksize = 5;      /* not strictly true, but we can use 5 here to
                                   make the checks below work fine still */
        }

        if(encrypted) {
            if(CRYPT_FLAG_R(session, REQUIRES_FULL_PACKET)) {
                auth_len = session->remote.crypt->auth_len;
            }
            else {
                remote_mac = session->remote.mac;
            }
        }

        etm = encrypted && remote_mac ? remote_mac->etm : 0;

        /* read/use a whole big chunk into a temporary area stored in
           the LIBSSH2_SESSION struct. We will decrypt data from that
           buffer into the packet buffer so this temp one doesn't have
           to be able to keep a whole SSH packet, just be large enough
           so that we can read big chunks from the network layer. */

        /* how much data there is remaining in the buffer to deal with
           before we should read more from the network */
        remainbuf = p->writeidx - p->readidx;

        /* if remainbuf turns negative we have a bad internal error */
        assert(remainbuf >= 0);

        if(remainbuf < blocksize ||
           (CRYPT_FLAG_R(session, REQUIRES_FULL_PACKET)
            && ((ssize_t)p->total_num) > remainbuf)) {
