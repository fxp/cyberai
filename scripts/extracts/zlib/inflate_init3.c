/* zlib 1.3.1 — inflate.c: inflateInit_+inflateGetHeader+inflateSetDictionary — L145-270 */
    /* get the state */
    if (inflateStateCheck(strm)) return Z_STREAM_ERROR;
    state = (struct inflate_state FAR *)strm->state;

    /* extract wrap request from windowBits parameter */
    if (windowBits < 0) {
        if (windowBits < -15)
            return Z_STREAM_ERROR;
        wrap = 0;
        windowBits = -windowBits;
    }
    else {
        wrap = (windowBits >> 4) + 5;
#ifdef GUNZIP
        if (windowBits < 48)
            windowBits &= 15;
#endif
    }

    /* set number of window bits, free window if different */
    if (windowBits && (windowBits < 8 || windowBits > 15))
        return Z_STREAM_ERROR;
    if (state->window != Z_NULL && state->wbits != (unsigned)windowBits) {
        ZFREE(strm, state->window);
        state->window = Z_NULL;
    }

    /* update state and reset the rest of it */
    state->wrap = wrap;
    state->wbits = (unsigned)windowBits;
    return inflateReset(strm);
}

int ZEXPORT inflateInit2_(z_streamp strm, int windowBits,
                          const char *version, int stream_size) {
    int ret;
    struct inflate_state FAR *state;

    if (version == Z_NULL || version[0] != ZLIB_VERSION[0] ||
        stream_size != (int)(sizeof(z_stream)))
        return Z_VERSION_ERROR;
    if (strm == Z_NULL) return Z_STREAM_ERROR;
    strm->msg = Z_NULL;                 /* in case we return an error */
    if (strm->zalloc == (alloc_func)0) {
#ifdef Z_SOLO
        return Z_STREAM_ERROR;
#else
        strm->zalloc = zcalloc;
        strm->opaque = (voidpf)0;
#endif
    }
    if (strm->zfree == (free_func)0)
#ifdef Z_SOLO
        return Z_STREAM_ERROR;
#else
        strm->zfree = zcfree;
#endif
    state = (struct inflate_state FAR *)
            ZALLOC(strm, 1, sizeof(struct inflate_state));
    if (state == Z_NULL) return Z_MEM_ERROR;
    Tracev((stderr, "inflate: allocated\n"));
    strm->state = (struct internal_state FAR *)state;
    state->strm = strm;
    state->window = Z_NULL;
    state->mode = HEAD;     /* to pass state test in inflateReset2() */
    ret = inflateReset2(strm, windowBits);
    if (ret != Z_OK) {
        ZFREE(strm, state);
        strm->state = Z_NULL;
    }
    return ret;
}

int ZEXPORT inflateInit_(z_streamp strm, const char *version,
                         int stream_size) {
    return inflateInit2_(strm, DEF_WBITS, version, stream_size);
}

int ZEXPORT inflatePrime(z_streamp strm, int bits, int value) {
    struct inflate_state FAR *state;

    if (inflateStateCheck(strm)) return Z_STREAM_ERROR;
    if (bits == 0)
        return Z_OK;
    state = (struct inflate_state FAR *)strm->state;
    if (bits < 0) {
        state->hold = 0;
        state->bits = 0;
        return Z_OK;
    }
    if (bits > 16 || state->bits + (uInt)bits > 32) return Z_STREAM_ERROR;
    value &= (1L << bits) - 1;
    state->hold += (unsigned)value << state->bits;
    state->bits += (uInt)bits;
    return Z_OK;
}

/*
   Return state with length and distance decoding tables and index sizes set to
   fixed code decoding.  Normally this returns fixed tables from inffixed.h.
   If BUILDFIXED is defined, then instead this routine builds the tables the
   first time it's called, and returns those tables the first time and
   thereafter.  This reduces the size of the code by about 2K bytes, in
   exchange for a little execution time.  However, BUILDFIXED should not be
   used for threaded applications, since the rewriting of the tables and virgin
   may not be thread-safe.
 */
local void fixedtables(struct inflate_state FAR *state) {
#ifdef BUILDFIXED
    static int virgin = 1;
    static code *lenfix, *distfix;
    static code fixed[544];

    /* build fixed huffman tables if first call (may not be thread safe) */
    if (virgin) {
        unsigned sym, bits;
        static code *next;

        /* literal/length table */
        sym = 0;
        while (sym < 144) state->lens[sym++] = 8;
        while (sym < 256) state->lens[sym++] = 9;
        while (sym < 280) state->lens[sym++] = 7;
        while (sym < 288) state->lens[sym++] = 8;
        next = fixed;
        lenfix = next;
