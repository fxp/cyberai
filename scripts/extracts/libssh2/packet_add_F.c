              string    request type in US-ASCII characters only
              boolean   want reply
              ....      type-specific data follows
            */

        case SSH_MSG_CHANNEL_REQUEST:
            if(datalen >= 9) {
                uint32_t channel = _libssh2_ntohu32(data + 1);
                uint32_t len = _libssh2_ntohu32(data + 5);
                unsigned char want_reply = 1;

                if((len + 9) < datalen)
                    want_reply = data[len + 9];

                _libssh2_debug((session,
                               LIBSSH2_TRACE_CONN,
                               "Channel %u received request type %.*s (wr %X)",
                               channel, (int)len, data + 9, want_reply));

                if(len == strlen("exit-status")
                    && (strlen("exit-status") + 9) <= datalen
                    && !memcmp("exit-status", data + 9,
                               strlen("exit-status"))) {

                    /* we've got "exit-status" packet. Set the session value */
                    if(datalen >= 20)
                        channelp =
                            _libssh2_channel_locate(session, channel);

                    if(channelp && (strlen("exit-status") + 14) <= datalen) {
                        channelp->exit_status =
                            _libssh2_ntohu32(data + 10 +
                                             strlen("exit-status"));
                        _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                                       "Exit status %d received for "
                                       "channel %u/%u",
                                       channelp->exit_status,
                                       channelp->local.id,
                                       channelp->remote.id));
                    }

                }
                else if(len == strlen("exit-signal")
                         && (strlen("exit-signal") + 9) <= datalen
                         && !memcmp("exit-signal", data + 9,
                                    strlen("exit-signal"))) {
                    /* command terminated due to signal */
                    if(datalen >= 20)
                        channelp = _libssh2_channel_locate(session, channel);

                    if(channelp && (strlen("exit-signal") + 14) <= datalen) {
                        /* set signal name (without SIG prefix) */
                        uint32_t namelen =
                            _libssh2_ntohu32(data + 10 +
                                             strlen("exit-signal"));

                        if(namelen <= UINT_MAX - 1) {
                            channelp->exit_signal =
                                LIBSSH2_ALLOC(session, namelen + 1);
                        }
                        else {
                            channelp->exit_signal = NULL;
                        }

                        if(!channelp->exit_signal)
                            rc = _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                                "memory for signal name");
                        else if((strlen("exit-signal") + 14 + namelen <=
                                 datalen)) {
                            memcpy(channelp->exit_signal,
                                   data + 14 + strlen("exit-signal"), namelen);
                            channelp->exit_signal[namelen] = '\0';
                            /* TODO: save error message and language tag */
                            _libssh2_debug((session, LIBSSH2_TRACE_CONN,
                                           "Exit signal %s received for "
                                           "channel %u/%u",
                                           channelp->exit_signal,
                                           channelp->local.id,
