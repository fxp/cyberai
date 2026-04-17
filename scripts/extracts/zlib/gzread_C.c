/* zlib 1.3.1 — gzread.c: gzread+gzfread+gzgetc: public read API — L320-480 */
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
        }

        /* update progress */
        len -= n;
        buf = (char *)buf + n;
        got += n;
        state->x.pos += n;
    } while (len);

    /* return number of bytes read into user buffer */
    return got;
}

/* -- see zlib.h -- */
int ZEXPORT gzread(gzFile file, voidp buf, unsigned len) {
    gz_statep state;

    /* get internal structure */
    if (file == NULL)
        return -1;
    state = (gz_statep)file;

    /* check that we're reading and that there's no (serious) error */
    if (state->mode != GZ_READ ||
            (state->err != Z_OK && state->err != Z_BUF_ERROR))
        return -1;

    /* since an int is returned, make sure len fits in one, otherwise return
       with an error (this avoids a flaw in the interface) */
    if ((int)len < 0) {
        gz_error(state, Z_STREAM_ERROR, "request does not fit in an int");
        return -1;
    }

    /* read len or fewer bytes to buf */
    len = (unsigned)gz_read(state, buf, len);

    /* check for an error */
    if (len == 0 && state->err != Z_OK && state->err != Z_BUF_ERROR)
        return -1;

    /* return the number of bytes read (this is assured to fit in an int) */
    return (int)len;
}

/* -- see zlib.h -- */
z_size_t ZEXPORT gzfread(voidp buf, z_size_t size, z_size_t nitems, gzFile file) {
    z_size_t len;
    gz_statep state;

    /* get internal structure */
    if (file == NULL)
        return 0;
    state = (gz_statep)file;

    /* check that we're reading and that there's no (serious) error */
    if (state->mode != GZ_READ ||
            (state->err != Z_OK && state->err != Z_BUF_ERROR))
        return 0;

    /* compute bytes to read -- error on overflow */
    len = nitems * size;
    if (size && len / size != nitems) {
        gz_error(state, Z_STREAM_ERROR, "request does not fit in a size_t");
        return 0;
    }

    /* read len or fewer bytes to buf, return the number of full items read */
    return len ? gz_read(state, buf, len) / size : 0;
}

/* -- see zlib.h -- */
#ifdef Z_PREFIX_SET
#  undef z_gzgetc
#else
#  undef gzgetc
#endif
int ZEXPORT gzgetc(gzFile file) {
    unsigned char buf[1];
    gz_statep state;

    /* get internal structure */
    if (file == NULL)
        return -1;
    state = (gz_statep)file;

    /* check that we're reading and that there's no (serious) error */
    if (state->mode != GZ_READ ||
        (state->err != Z_OK && state->err != Z_BUF_ERROR))
        return -1;

    /* try output buffer (no need to check for skip request) */
    if (state->x.have) {
        state->x.have--;
        state->x.pos++;
        return *(state->x.next)++;
    }

    /* nothing there -- try gz_read() */
    return gz_read(state, buf, 1) < 1 ? -1 : buf[0];
}

int ZEXPORT gzgetc_(gzFile file) {
    return gzgetc(file);
}

/* -- see zlib.h -- */
int ZEXPORT gzungetc(int c, gzFile file) {
    gz_statep state;

    /* get internal structure */
    if (file == NULL)
        return -1;
    state = (gz_statep)file;

    /* in case this was just opened, set up the input buffer */
    if (state->mode == GZ_READ && state->how == LOOK && state->x.have == 0)
        (void)gz_look(state);

    /* check that we're reading and that there's no (serious) error */
    if (state->mode != GZ_READ ||
        (state->err != Z_OK && state->err != Z_BUF_ERROR))
        return -1;

    /* process a skip request */
    if (state->seek) {
        state->seek = 0;
        if (gz_skip(state, state->skip) == -1)
            return -1;
    }

    /* can't push EOF */
    if (c < 0)
        return -1;

    /* if output buffer empty, put byte at end (allows more pushing) */
    if (state->x.have == 0) {
        state->x.have = 1;
        state->x.next = state->out + (state->size << 1) - 1;
        state->x.next[0] = (unsigned char)c;
        state->x.pos--;
        state->past = 0;
        return c;
    }

    /* if no room, give up (must have already done a gzungetc()) */
    if (state->x.have == (state->size << 1)) {
        gz_error(state, Z_DATA_ERROR, "out of room to push characters");
        return -1;
    }
