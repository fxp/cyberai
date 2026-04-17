/* zlib 1.3.1 — gzread.c: gz_look+gz_decomp: gzip header detect + decompress — L90-200 */
        state->size = state->want;

        /* allocate inflate memory */
        state->strm.zalloc = Z_NULL;
        state->strm.zfree = Z_NULL;
        state->strm.opaque = Z_NULL;
        state->strm.avail_in = 0;
        state->strm.next_in = Z_NULL;
        if (inflateInit2(&(state->strm), 15 + 16) != Z_OK) {    /* gunzip */
            free(state->out);
            free(state->in);
            state->size = 0;
            gz_error(state, Z_MEM_ERROR, "out of memory");
            return -1;
        }
    }

    /* get at least the magic bytes in the input buffer */
    if (strm->avail_in < 2) {
        if (gz_avail(state) == -1)
            return -1;
        if (strm->avail_in == 0)
            return 0;
    }

    /* look for gzip magic bytes -- if there, do gzip decoding (note: there is
       a logical dilemma here when considering the case of a partially written
       gzip file, to wit, if a single 31 byte is written, then we cannot tell
       whether this is a single-byte file, or just a partially written gzip
       file -- for here we assume that if a gzip file is being written, then
       the header will be written in a single operation, so that reading a
       single byte is sufficient indication that it is not a gzip file) */
    if (strm->avail_in > 1 &&
            strm->next_in[0] == 31 && strm->next_in[1] == 139) {
        inflateReset(strm);
        state->how = GZIP;
        state->direct = 0;
        return 0;
    }

    /* no gzip header -- if we were decoding gzip before, then this is trailing
       garbage.  Ignore the trailing garbage and finish. */
    if (state->direct == 0) {
        strm->avail_in = 0;
        state->eof = 1;
        state->x.have = 0;
        return 0;
    }

    /* doing raw i/o, copy any leftover input to output -- this assumes that
       the output buffer is larger than the input buffer, which also assures
       space for gzungetc() */
    state->x.next = state->out;
    memcpy(state->x.next, strm->next_in, strm->avail_in);
    state->x.have = strm->avail_in;
    strm->avail_in = 0;
    state->how = COPY;
    state->direct = 1;
    return 0;
}

/* Decompress from input to the provided next_out and avail_out in the state.
   On return, state->x.have and state->x.next point to the just decompressed
   data.  If the gzip stream completes, state->how is reset to LOOK to look for
   the next gzip stream or raw data, once state->x.have is depleted.  Returns 0
   on success, -1 on failure. */
local int gz_decomp(gz_statep state) {
    int ret = Z_OK;
    unsigned had;
    z_streamp strm = &(state->strm);

    /* fill output buffer up to end of deflate stream */
    had = strm->avail_out;
    do {
        /* get more input for inflate() */
        if (strm->avail_in == 0 && gz_avail(state) == -1)
            return -1;
        if (strm->avail_in == 0) {
            gz_error(state, Z_BUF_ERROR, "unexpected end of file");
            break;
        }

        /* decompress and handle errors */
        ret = inflate(strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT) {
            gz_error(state, Z_STREAM_ERROR,
                     "internal error: inflate stream corrupt");
            return -1;
        }
        if (ret == Z_MEM_ERROR) {
            gz_error(state, Z_MEM_ERROR, "out of memory");
            return -1;
        }
        if (ret == Z_DATA_ERROR) {              /* deflate stream invalid */
            gz_error(state, Z_DATA_ERROR,
                     strm->msg == NULL ? "compressed data error" : strm->msg);
            return -1;
        }
    } while (strm->avail_out && ret != Z_STREAM_END);

    /* update available output */
    state->x.have = had - strm->avail_out;
    state->x.next = strm->next_out - state->x.have;

    /* if the gzip stream completed successfully, look for another */
    if (ret == Z_STREAM_END)
        state->how = LOOK;

    /* good decompression */
    return 0;
}
