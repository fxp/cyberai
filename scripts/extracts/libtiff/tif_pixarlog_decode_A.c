/* ===== EXTRACT: tif_pixarlog.c ===== */
/* Function: PixarLogDecode (part A) */
/* Lines: 834–951 */

static int PixarLogDecode(TIFF *tif, uint8_t *op, tmsize_t occ, uint16_t s)
{
    static const char module[] = "PixarLogDecode";
    TIFFDirectory *td = &tif->tif_dir;
    PixarLogState *sp = PixarLogDecoderState(tif);
    tmsize_t i;
    tmsize_t nsamples;
    int llen;
    uint16_t *up;

    switch (sp->user_datafmt)
    {
        case PIXARLOGDATAFMT_FLOAT:
            nsamples = occ / sizeof(float); /* XXX float == 32 bits */
            break;
        case PIXARLOGDATAFMT_16BIT:
        case PIXARLOGDATAFMT_12BITPICIO:
        case PIXARLOGDATAFMT_11BITLOG:
            nsamples = occ / sizeof(uint16_t); /* XXX uint16_t == 16 bits */
            break;
        case PIXARLOGDATAFMT_8BIT:
        case PIXARLOGDATAFMT_8BITABGR:
            nsamples = occ;
            break;
        default:
            TIFFErrorExtR(tif, module,
                          "%" PRIu16 " bit input not supported in PixarLog",
                          td->td_bitspersample);
            memset(op, 0, (size_t)occ);
            return 0;
    }

    llen = sp->stride * td->td_imagewidth;

    (void)s;
    assert(sp != NULL);

    sp->stream.next_in = tif->tif_rawcp;
    sp->stream.avail_in = (uInt)tif->tif_rawcc;

    sp->stream.next_out = (unsigned char *)sp->tbuf;
    assert(sizeof(sp->stream.avail_out) == 4); /* if this assert gets raised,
         we need to simplify this code to reflect a ZLib that is likely updated
         to deal with 8byte memory sizes, though this code will respond
         appropriately even before we simplify it */
    sp->stream.avail_out = (uInt)(nsamples * sizeof(uint16_t));
    if (sp->stream.avail_out != nsamples * sizeof(uint16_t))
    {
        TIFFErrorExtR(tif, module, "ZLib cannot deal with buffers this size");
        memset(op, 0, (size_t)occ);
        return (0);
    }
    /* Check that we will not fill more than what was allocated */
    if ((tmsize_t)sp->stream.avail_out > sp->tbuf_size)
    {
        TIFFErrorExtR(tif, module, "sp->stream.avail_out > sp->tbuf_size");
        memset(op, 0, (size_t)occ);
        return (0);
    }
    do
    {
        int state = inflate(&sp->stream, Z_PARTIAL_FLUSH);
        if (state == Z_STREAM_END)
        {
            break; /* XXX */
        }
        if (state == Z_DATA_ERROR)
        {
            TIFFErrorExtR(
                tif, module, "Decoding error at scanline %" PRIu32 ", %s",
                tif->tif_row, sp->stream.msg ? sp->stream.msg : "(null)");
            memset(op, 0, (size_t)occ);
            return (0);
        }
        if (state != Z_OK)
        {
            TIFFErrorExtR(tif, module, "ZLib error: %s",
                          sp->stream.msg ? sp->stream.msg : "(null)");
            memset(op, 0, (size_t)occ);
            return (0);
        }
    } while (sp->stream.avail_out > 0);

    /* hopefully, we got all the bytes we needed */
    if (sp->stream.avail_out != 0)
    {
        TIFFErrorExtR(tif, module,
                      "Not enough data at scanline %" PRIu32
                      " (short %u bytes)",
                      tif->tif_row, sp->stream.avail_out);
        memset(op, 0, (size_t)occ);
        return (0);
    }

    tif->tif_rawcp = sp->stream.next_in;
    tif->tif_rawcc = sp->stream.avail_in;

    up = sp->tbuf;
    /* Swap bytes in the data if from a different endian machine. */
    if (tif->tif_flags & TIFF_SWAB)
        TIFFSwabArrayOfShort(up, nsamples);

    /*
     * if llen is not an exact multiple of nsamples, the decode operation
     * may overflow the output buffer, so truncate it enough to prevent
     * that but still salvage as much data as possible.
     */
    if (nsamples % llen)
    {
        TIFFWarningExtR(tif, module,
                        "stride %d is not a multiple of sample count, "
                        "%" TIFF_SSIZE_FORMAT ", data truncated.",
                        llen, nsamples);
        nsamples -= nsamples % llen;
    }

    for (i = 0; i < nsamples; i += llen, up += llen)
    {
