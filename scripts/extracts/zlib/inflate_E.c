/* zlib 1.3.1 — inflate.c: case CODELENS+LENS+DIST: dynamic Huffman decode — L975-1090 */
                        DROPBITS(7);
                    }
                    if (state->have + copy > state->nlen + state->ndist) {
                        strm->msg = (char *)"invalid bit length repeat";
                        state->mode = BAD;
                        break;
                    }
                    while (copy--)
                        state->lens[state->have++] = (unsigned short)len;
                }
            }

            /* handle error breaks in while */
            if (state->mode == BAD) break;

            /* check for end-of-block code (better have one) */
            if (state->lens[256] == 0) {
                strm->msg = (char *)"invalid code -- missing end-of-block";
                state->mode = BAD;
                break;
            }

            /* build code tables -- note: do not change the lenbits or distbits
               values here (9 and 6) without reading the comments in inftrees.h
               concerning the ENOUGH constants, which depend on those values */
            state->next = state->codes;
            state->lencode = (const code FAR *)(state->next);
            state->lenbits = 9;
            ret = inflate_table(LENS, state->lens, state->nlen, &(state->next),
                                &(state->lenbits), state->work);
            if (ret) {
                strm->msg = (char *)"invalid literal/lengths set";
                state->mode = BAD;
                break;
            }
            state->distcode = (const code FAR *)(state->next);
            state->distbits = 6;
            ret = inflate_table(DISTS, state->lens + state->nlen, state->ndist,
                            &(state->next), &(state->distbits), state->work);
            if (ret) {
                strm->msg = (char *)"invalid distances set";
                state->mode = BAD;
                break;
            }
            Tracev((stderr, "inflate:       codes ok\n"));
            state->mode = LEN_;
            if (flush == Z_TREES) goto inf_leave;
                /* fallthrough */
        case LEN_:
            state->mode = LEN;
                /* fallthrough */
        case LEN:
            if (have >= 6 && left >= 258) {
                RESTORE();
                inflate_fast(strm, out);
                LOAD();
                if (state->mode == TYPE)
                    state->back = -1;
                break;
            }
            state->back = 0;
            for (;;) {
                here = state->lencode[BITS(state->lenbits)];
                if ((unsigned)(here.bits) <= bits) break;
                PULLBYTE();
            }
            if (here.op && (here.op & 0xf0) == 0) {
                last = here;
                for (;;) {
                    here = state->lencode[last.val +
                            (BITS(last.bits + last.op) >> last.bits)];
                    if ((unsigned)(last.bits + here.bits) <= bits) break;
                    PULLBYTE();
                }
                DROPBITS(last.bits);
                state->back += last.bits;
            }
            DROPBITS(here.bits);
            state->back += here.bits;
            state->length = (unsigned)here.val;
            if ((int)(here.op) == 0) {
                Tracevv((stderr, here.val >= 0x20 && here.val < 0x7f ?
                        "inflate:         literal '%c'\n" :
                        "inflate:         literal 0x%02x\n", here.val));
                state->mode = LIT;
                break;
            }
            if (here.op & 32) {
                Tracevv((stderr, "inflate:         end of block\n"));
                state->back = -1;
                state->mode = TYPE;
                break;
            }
            if (here.op & 64) {
                strm->msg = (char *)"invalid literal/length code";
                state->mode = BAD;
                break;
            }
            state->extra = (unsigned)(here.op) & 15;
            state->mode = LENEXT;
                /* fallthrough */
        case LENEXT:
            if (state->extra) {
                NEEDBITS(state->extra);
                state->length += BITS(state->extra);
                DROPBITS(state->extra);
                state->back += state->extra;
            }
            Tracevv((stderr, "inflate:         length %u\n", state->length));
            state->was = state->length;
            state->mode = DIST;
                /* fallthrough */
        case DIST:
            for (;;) {
                here = state->distcode[BITS(state->distbits)];
                if ((unsigned)(here.bits) <= bits) break;
