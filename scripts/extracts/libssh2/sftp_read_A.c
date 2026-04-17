{
    LIBSSH2_SFTP *sftp = handle->sftp;
    LIBSSH2_CHANNEL *channel = sftp->channel;
    LIBSSH2_SESSION *session = channel->session;
    size_t count = 0;
    struct sftp_pipeline_chunk *chunk;
    struct sftp_pipeline_chunk *next;
    ssize_t rc;
    struct _libssh2_sftp_handle_file_data *filep =
        &handle->u.file;
    size_t bytes_in_buffer = 0;
    char *sliding_bufferp = buffer;

    /* This function can be interrupted in three different places where it
       might need to wait for data from the network.  It returns EAGAIN to
       allow non-blocking clients to do other work but these client are
       expected to call this function again (possibly many times) to finish
       the operation.

       The tricky part is that if we previously aborted a sftp_read due to
       EAGAIN, we must continue at the same spot to continue the previously
       interrupted operation.  This is done using a state machine to record
       what phase of execution we were at.  The state is stored in
       sftp->read_state.

       libssh2_NB_state_idle: The first phase is where we prepare multiple
       FXP_READ packets to do optimistic read-ahead.  We send off as many as
       possible in the second phase without waiting for a response to each
       one; this is the key to fast reads. But we may have to adjust the
       channel window size to do this which may interrupt this function while
       waiting.  The state machine saves the phase as libssh2_NB_state_idle so
       it returns here on the next call.

       libssh2_NB_state_sent: The second phase is where we send the FXP_READ
       packets.  Writing them to the channel can be interrupted with EAGAIN
       but the state machine ensures we skip the first phase on the next call
       and resume sending.

       libssh2_NB_state_sent2: In the third phase (indicated by ) we read the
       data from the responses that have arrived so far.  Reading can be
       interrupted with EAGAIN but the state machine ensures we skip the first
       and second phases on the next call and resume sending.
    */

    switch(sftp->read_state) {
    case libssh2_NB_state_idle:
        sftp->last_errno = LIBSSH2_FX_OK;

        /* Some data may already have been read from the server in the
           previous call but didn't fit in the buffer at the time.  If so, we
           return that now as we can't risk being interrupted later with data
           partially written to the buffer. */
        if(filep->data_left) {
            size_t copy = LIBSSH2_MIN(buffer_size, filep->data_left);

            memcpy(buffer, &filep->data[ filep->data_len - filep->data_left],
                   copy);

            filep->data_left -= copy;
            filep->offset += copy;

            if(!filep->data_left) {
                LIBSSH2_FREE(session, filep->data);
                filep->data = NULL;
            }

            return copy;
        }

        if(filep->eof) {
            return 0;
        }
        else {
            /* We allow a number of bytes being requested at any given time
               without having been acked - until we reach EOF. */

            /* Number of bytes asked for that haven't been acked yet */
            size_t already = (size_t)(filep->offset_sent - filep->offset);

            size_t max_read_ahead = buffer_size*4;
            unsigned long recv_window;

            if(max_read_ahead > LIBSSH2_CHANNEL_WINDOW_DEFAULT*4)
                max_read_ahead = LIBSSH2_CHANNEL_WINDOW_DEFAULT*4;

            /* if the buffer_size passed in now is smaller than what has
               already been sent, we risk getting count become a very large
               number */
            if(max_read_ahead > already)
                count = max_read_ahead - already;

            /* 'count' is how much more data to ask for, and 'already' is how
               much data that already has been asked for but not yet returned.
