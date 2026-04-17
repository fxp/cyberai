/* zlib 1.3.1 — inflate.c: case LEN_+MATCH+LIT: back-reference copy — L1090-1200 */
                if ((unsigned)(here.bits) <= bits) break;
                PULLBYTE();
            }
            if ((here.op & 0xf0) == 0) {
                last = here;
                for (;;) {
                    here = state->distcode[last.val +
                            (BITS(last.bits + last.op) >> last.bits)];
                    if ((unsigned)(last.bits + here.bits) <= bits) break;
                    PULLBYTE();
                }
                DROPBITS(last.bits);
                state->back += last.bits;
            }
            DROPBITS(here.bits);
            state->back += here.bits;
            if (here.op & 64) {
                strm->msg = (char *)"invalid distance code";
                state->mode = BAD;
                break;
            }
            state->offset = (unsigned)here.val;
            state->extra = (unsigned)(here.op) & 15;
            state->mode = DISTEXT;
                /* fallthrough */
        case DISTEXT:
            if (state->extra) {
                NEEDBITS(state->extra);
                state->offset += BITS(state->extra);
                DROPBITS(state->extra);
                state->back += state->extra;
            }
#ifdef INFLATE_STRICT
            if (state->offset > state->dmax) {
                strm->msg = (char *)"invalid distance too far back";
                state->mode = BAD;
                break;
            }
#endif
            Tracevv((stderr, "inflate:         distance %u\n", state->offset));
            state->mode = MATCH;
                /* fallthrough */
        case MATCH:
            if (left == 0) goto inf_leave;
            copy = out - left;
            if (state->offset > copy) {         /* copy from window */
                copy = state->offset - copy;
                if (copy > state->whave) {
                    if (state->sane) {
                        strm->msg = (char *)"invalid distance too far back";
                        state->mode = BAD;
                        break;
                    }
#ifdef INFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR
                    Trace((stderr, "inflate.c too far\n"));
                    copy -= state->whave;
                    if (copy > state->length) copy = state->length;
                    if (copy > left) copy = left;
                    left -= copy;
                    state->length -= copy;
                    do {
                        *put++ = 0;
                    } while (--copy);
                    if (state->length == 0) state->mode = LEN;
                    break;
#endif
                }
                if (copy > state->wnext) {
                    copy -= state->wnext;
                    from = state->window + (state->wsize - copy);
                }
                else
                    from = state->window + (state->wnext - copy);
                if (copy > state->length) copy = state->length;
            }
            else {                              /* copy from output */
                from = put - state->offset;
                copy = state->length;
            }
            if (copy > left) copy = left;
            left -= copy;
            state->length -= copy;
            do {
                *put++ = *from++;
            } while (--copy);
            if (state->length == 0) state->mode = LEN;
            break;
        case LIT:
            if (left == 0) goto inf_leave;
            *put++ = (unsigned char)(state->length);
            left--;
            state->mode = LEN;
            break;
        case CHECK:
            if (state->wrap) {
                NEEDBITS(32);
                out -= left;
                strm->total_out += out;
                state->total += out;
                if ((state->wrap & 4) && out)
                    strm->adler = state->check =
                        UPDATE_CHECK(state->check, put - out, out);
                out = left;
                if ((state->wrap & 4) && (
#ifdef GUNZIP
                     state->flags ? hold :
#endif
                     ZSWAP32(hold)) != state->check) {
                    strm->msg = (char *)"incorrect data check";
                    state->mode = BAD;
                    break;
