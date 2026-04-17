/* zlib 1.3.1 — inffast.c: inflate_fast A: literal/length decode entry — L1-115 */
/* inffast.c -- fast decoding
 * Copyright (C) 1995-2017 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zutil.h"
#include "inftrees.h"
#include "inflate.h"
#include "inffast.h"

#ifdef ASMINF
#  pragma message("Assembler code may have bugs -- use at your own risk")
#else

/*
   Decode literal, length, and distance codes and write out the resulting
   literal and match bytes until either not enough input or output is
   available, an end-of-block is encountered, or a data error is encountered.
   When large enough input and output buffers are supplied to inflate(), for
   example, a 16K input buffer and a 64K output buffer, more than 95% of the
   inflate execution time is spent in this routine.

   Entry assumptions:

        state->mode == LEN
        strm->avail_in >= 6
        strm->avail_out >= 258
        start >= strm->avail_out
        state->bits < 8

   On return, state->mode is one of:

        LEN -- ran out of enough output space or enough available input
        TYPE -- reached end of block code, inflate() to interpret next block
        BAD -- error in block data

   Notes:

    - The maximum input bits used by a length/distance pair is 15 bits for the
      length code, 5 bits for the length extra, 15 bits for the distance code,
      and 13 bits for the distance extra.  This totals 48 bits, or six bytes.
      Therefore if strm->avail_in >= 6, then there is enough input to avoid
      checking for available input while decoding.

    - The maximum bytes that a single length/distance pair can output is 258
      bytes, which is the maximum length that can be coded.  inflate_fast()
      requires strm->avail_out >= 258 for each loop to avoid checking for
      output space.
 */
void ZLIB_INTERNAL inflate_fast(z_streamp strm, unsigned start) {
    struct inflate_state FAR *state;
    z_const unsigned char FAR *in;      /* local strm->next_in */
    z_const unsigned char FAR *last;    /* have enough input while in < last */
    unsigned char FAR *out;     /* local strm->next_out */
    unsigned char FAR *beg;     /* inflate()'s initial strm->next_out */
    unsigned char FAR *end;     /* while out < end, enough space available */
#ifdef INFLATE_STRICT
    unsigned dmax;              /* maximum distance from zlib header */
#endif
    unsigned wsize;             /* window size or zero if not using window */
    unsigned whave;             /* valid bytes in the window */
    unsigned wnext;             /* window write index */
    unsigned char FAR *window;  /* allocated sliding window, if wsize != 0 */
    unsigned long hold;         /* local strm->hold */
    unsigned bits;              /* local strm->bits */
    code const FAR *lcode;      /* local strm->lencode */
    code const FAR *dcode;      /* local strm->distcode */
    unsigned lmask;             /* mask for first level of length codes */
    unsigned dmask;             /* mask for first level of distance codes */
    code const *here;           /* retrieved table entry */
    unsigned op;                /* code bits, operation, extra bits, or */
                                /*  window position, window bytes to copy */
    unsigned len;               /* match length, unused bytes */
    unsigned dist;              /* match distance */
    unsigned char FAR *from;    /* where to copy match from */

    /* copy state to local variables */
    state = (struct inflate_state FAR *)strm->state;
    in = strm->next_in;
    last = in + (strm->avail_in - 5);
    out = strm->next_out;
    beg = out - (start - strm->avail_out);
    end = out + (strm->avail_out - 257);
#ifdef INFLATE_STRICT
    dmax = state->dmax;
#endif
    wsize = state->wsize;
    whave = state->whave;
    wnext = state->wnext;
    window = state->window;
    hold = state->hold;
    bits = state->bits;
    lcode = state->lencode;
    dcode = state->distcode;
    lmask = (1U << state->lenbits) - 1;
    dmask = (1U << state->distbits) - 1;

    /* decode literals and length/distances until end-of-block or not enough
       input data or output space */
    do {
        if (bits < 15) {
            hold += (unsigned long)(*in++) << bits;
            bits += 8;
            hold += (unsigned long)(*in++) << bits;
            bits += 8;
        }
        here = lcode + (hold & lmask);
      dolen:
        op = (unsigned)(here->bits);
        hold >>= op;
        bits -= op;
        op = (unsigned)(here->op);
        if (op == 0) {                          /* literal */
            Tracevv((stderr, here->val >= 0x20 && here->val < 0x7f ?
                    "inflate:         literal '%c'\n" :
