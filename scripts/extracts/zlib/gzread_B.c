/* zlib 1.3.1 — gzread.c: gz_fetch+gz_skip: fetch buffer + line skip — L200-330 */
}

/* Fetch data and put it in the output buffer.  Assumes state->x.have is 0.
   Data is either copied from the input file or decompressed from the input
   file depending on state->how.  If state->how is LOOK, then a gzip header is
   looked for to determine whether to copy or decompress.  Returns -1 on error,
   otherwise 0.  gz_fetch() will leave state->how as COPY or GZIP unless the
   end of the input file has been reached and all data has been processed.  */
local int gz_fetch(gz_statep state) {
    z_streamp strm = &(state->strm);

    do {
        switch(state->how) {
        case LOOK:      /* -> LOOK, COPY (only if never GZIP), or GZIP */
            if (gz_look(state) == -1)
                return -1;
            if (state->how == LOOK)
                return 0;
            break;
        case COPY:      /* -> COPY */
            if (gz_load(state, state->out, state->size << 1, &(state->x.have))
                    == -1)
                return -1;
            state->x.next = state->out;
            return 0;
        case GZIP:      /* -> GZIP or LOOK (if end of gzip stream) */
            strm->avail_out = state->size << 1;
            strm->next_out = state->out;
            if (gz_decomp(state) == -1)
                return -1;
        }
    } while (state->x.have == 0 && (!state->eof || strm->avail_in));
    return 0;
}

/* Skip len uncompressed bytes of output.  Return -1 on error, 0 on success. */
local int gz_skip(gz_statep state, z_off64_t len) {
    unsigned n;

    /* skip over len bytes or reach end-of-file, whichever comes first */
    while (len)
        /* skip over whatever is in output buffer */
        if (state->x.have) {
            n = GT_OFF(state->x.have) || (z_off64_t)state->x.have > len ?
                (unsigned)len : state->x.have;
            state->x.have -= n;
            state->x.next += n;
            state->x.pos += n;
            len -= n;
        }

        /* output buffer empty -- return if we're at the end of the input */
        else if (state->eof && state->strm.avail_in == 0)
            break;

        /* need more data to skip -- load up output buffer */
        else {
            /* get more output, looking for header if required */
            if (gz_fetch(state) == -1)
                return -1;
        }
    return 0;
}

/* Read len bytes into buf from file, or less than len up to the end of the
   input.  Return the number of bytes read.  If zero is returned, either the
   end of file was reached, or there was an error.  state->err must be
   consulted in that case to determine which. */
local z_size_t gz_read(gz_statep state, voidp buf, z_size_t len) {
    z_size_t got;
    unsigned n;

    /* if len is zero, avoid unnecessary operations */
    if (len == 0)
        return 0;

    /* process a skip request */
    if (state->seek) {
        state->seek = 0;
        if (gz_skip(state, state->skip) == -1)
            return 0;
    }

    /* get len bytes to buf, or less than len if at the end */
    got = 0;
    do {
        /* set n to the maximum amount of len that fits in an unsigned int */
        n = (unsigned)-1;
        if (n > len)
            n = (unsigned)len;

        /* first just try copying data from the output buffer */
        if (state->x.have) {
            if (state->x.have < n)
                n = state->x.have;
            memcpy(buf, state->x.next, n);
            state->x.next += n;
            state->x.have -= n;
        }

        /* output buffer empty -- return if we're at the end of the input */
        else if (state->eof && state->strm.avail_in == 0) {
            state->past = 1;        /* tried to read past end */
            break;
        }

        /* need output data -- for small len or new stream load up our output
           buffer */
        else if (state->how == LOOK || n < (state->size << 1)) {
            /* get more output, looking for header if required */
            if (gz_fetch(state) == -1)
                return 0;
            continue;       /* no progress yet -- go back to copy above */
            /* the copy above assures that we will leave with space in the
               output buffer, allowing at least one gzungetc() to succeed */
        }

        /* large len -- read directly into user buffer */
        else if (state->how == COPY) {      /* read directly */
            if (gz_load(state, (unsigned char *)buf, n, &n) == -1)
                return 0;
        }

        /* large len -- decompress directly into user buffer */
        else {  /* state->how == GZIP */
            state->strm.avail_out = n;
            state->strm.next_out = (unsigned char *)buf;
            if (gz_decomp(state) == -1)
                return 0;
            n = state->x.have;
            state->x.have = 0;
