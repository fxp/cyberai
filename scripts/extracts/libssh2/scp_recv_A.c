{
    size_t cmd_len;
    int rc;
    int tmp_err_code;
    const char *tmp_err_msg;

    if(session->scpRecv_state == libssh2_NB_state_idle) {
        session->scpRecv_mode = 0;
        session->scpRecv_size = 0;
        session->scpRecv_mtime = 0;
        session->scpRecv_atime = 0;

        session->scpRecv_command_len =
            _libssh2_shell_quotedsize(path) + sizeof("scp -f ") + (sb ? 1 : 0);

        session->scpRecv_command =
            LIBSSH2_ALLOC(session, session->scpRecv_command_len);

        if(!session->scpRecv_command) {
            _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                           "Unable to allocate a command buffer for "
                           "SCP session");
            return NULL;
        }

        snprintf((char *)session->scpRecv_command,
                 session->scpRecv_command_len,
                 "scp -%sf ", sb ? "p" : "");

        cmd_len = strlen((char *)session->scpRecv_command);

        if(!session->flag.quote_paths) {
            size_t path_len;

            path_len = strlen(path);

            /* no NUL-termination needed, so memcpy will do */
            memcpy(&session->scpRecv_command[cmd_len], path, path_len);
            cmd_len += path_len;
        }
        else {
            cmd_len += shell_quotearg(path,
                                      &session->scpRecv_command[cmd_len],
                                      session->scpRecv_command_len - cmd_len);
        }

        /* the command to exec should _not_ be NUL-terminated */
        session->scpRecv_command_len = cmd_len;

        _libssh2_debug((session, LIBSSH2_TRACE_SCP,
                       "Opening channel for SCP receive"));

        session->scpRecv_state = libssh2_NB_state_created;
    }

    if(session->scpRecv_state == libssh2_NB_state_created) {
        /* Allocate a channel */
        session->scpRecv_channel =
            _libssh2_channel_open(session, "session",
                                  sizeof("session") - 1,
                                  LIBSSH2_CHANNEL_WINDOW_DEFAULT,
                                  LIBSSH2_CHANNEL_PACKET_DEFAULT, NULL,
                                  0);
        if(!session->scpRecv_channel) {
            if(libssh2_session_last_errno(session) !=
                LIBSSH2_ERROR_EAGAIN) {
                LIBSSH2_FREE(session, session->scpRecv_command);
                session->scpRecv_command = NULL;
                session->scpRecv_state = libssh2_NB_state_idle;
            }
            else {
                _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                               "Would block starting up channel");
            }
            return NULL;
        }

        session->scpRecv_state = libssh2_NB_state_sent;
    }

    if(session->scpRecv_state == libssh2_NB_state_sent) {
        /* Request SCP for the desired file */
        rc = _libssh2_channel_process_startup(session->scpRecv_channel, "exec",
                                              sizeof("exec") - 1,
                                              (char *)session->scpRecv_command,
                                              session->scpRecv_command_len);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                           "Would block requesting SCP startup");
            return NULL;
        }
        else if(rc) {
            LIBSSH2_FREE(session, session->scpRecv_command);
            session->scpRecv_command = NULL;
            goto scp_recv_error;
        }
        LIBSSH2_FREE(session, session->scpRecv_command);
        session->scpRecv_command = NULL;

        _libssh2_debug((session, LIBSSH2_TRACE_SCP, "Sending initial wakeup"));
        /* SCP ACK */
        session->scpRecv_response[0] = '\0';

        session->scpRecv_state = libssh2_NB_state_sent1;
    }

    if(session->scpRecv_state == libssh2_NB_state_sent1) {
        rc = (int)_libssh2_channel_write(session->scpRecv_channel, 0,
