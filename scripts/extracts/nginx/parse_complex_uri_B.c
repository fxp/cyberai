/* parse_complex_uri B: dot-segments */
                ch = *p++;
                break;
            }

            switch (ch) {
#if (NGX_WIN32)
            case '\\':
                break;
#endif
            case '/':
                if (!merge_slashes) {
                    *u++ = ch;
                }
                break;
            case '.':
                state = sw_dot;
                *u++ = ch;
                break;
            case '%':
                quoted_state = state;
                state = sw_quoted;
                break;
            case '?':
                r->args_start = p;
                goto args;
            case '#':
                goto done;
            case '+':
                r->plus_in_uri = 1;
                /* fall through */
            default:
                state = sw_usual;
                *u++ = ch;
                break;
            }

            ch = *p++;
            break;

        case sw_dot:

            if (usual[ch >> 5] & (1U << (ch & 0x1f))) {
                state = sw_usual;
                *u++ = ch;
                ch = *p++;
                break;
            }

            switch (ch) {
#if (NGX_WIN32)
            case '\\':
#endif
            case '/':
                state = sw_slash;
                u--;
                break;
            case '.':
                state = sw_dot_dot;
                *u++ = ch;
                break;
            case '%':
                quoted_state = state;
                state = sw_quoted;
                break;
            case '?':
                u--;
                r->args_start = p;
                goto args;
            case '#':
                u--;
                goto done;
            case '+':
                r->plus_in_uri = 1;
                /* fall through */
            default:
                state = sw_usual;
                *u++ = ch;
                break;
            }

            ch = *p++;
            break;

        case sw_dot_dot:

            if (usual[ch >> 5] & (1U << (ch & 0x1f))) {
                state = sw_usual;
                *u++ = ch;
                ch = *p++;
                break;
            }

            switch (ch) {
#if (NGX_WIN32)
            case '\\':
#endif
            case '/':
            case '?':
            case '#':
                u -= 4;
                for ( ;; ) {
                    if (u < r->uri.data) {
                        return NGX_HTTP_PARSE_INVALID_REQUEST;
                    }
                    if (*u == '/') {
                        u++;
                        break;
                    }
                    u--;
                }
                if (ch == '?') {
                    r->args_start = p;
                    goto args;
                }
                if (ch == '#') {
                    goto done;
                }
                state = sw_slash;
                break;
            case '%':
                quoted_state = state;
                state = sw_quoted;
                break;
            case '+':
                r->plus_in_uri = 1;
                /* fall through */
            default:
                state = sw_usual;
                *u++ = ch;
                break;
