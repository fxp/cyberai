/* ===== EXTRACT: tif_pixarlog.c ===== */
/* Function: PixarLogEncode */
/* Lines: 1300–1401 */

static int PixarLogEncode(TIFF *tif, uint8_t *bp, tmsize_t cc, uint16_t s)
{
    static const char module[] = "PixarLogEncode";
    TIFFDirectory *td = &tif->tif_dir;
    PixarLogState *sp = PixarLogEncoderState(tif);
    tmsize_t i;
    tmsize_t n;
    int llen;
    unsigned short *up;

    (void)s;

    switch (sp->user_datafmt)
    {
        case PIXARLOGDATAFMT_FLOAT:
            n = cc / sizeof(float); /* XXX float == 32 bits */
            break;
        case PIXARLOGDATAFMT_16BIT:
        case PIXARLOGDATAFMT_12BITPICIO:
        case PIXARLOGDATAFMT_11BITLOG:
            n = cc / sizeof(uint16_t); /* XXX uint16_t == 16 bits */
            break;
        case PIXARLOGDATAFMT_8BIT:
        case PIXARLOGDATAFMT_8BITABGR:
            n = cc;
            break;
        default:
            TIFFErrorExtR(tif, module,
                          "%" PRIu16 " bit input not supported in PixarLog",
                          td->td_bitspersample);
            return 0;
    }

    llen = sp->stride * td->td_imagewidth;
    /* Check against the number of elements (of size uint16_t) of sp->tbuf */
    if (n > ((tmsize_t)td->td_rowsperstrip * llen))
    {
        TIFFErrorExtR(tif, module, "Too many input bytes provided");
        return 0;
    }

    for (i = 0, up = sp->tbuf; i < n; i += llen, up += llen)
    {
        switch (sp->user_datafmt)
        {
            case PIXARLOGDATAFMT_FLOAT:
                horizontalDifferenceF((float *)bp, llen, sp->stride, up,
                                      sp->FromLT2);
                bp += llen * sizeof(float);
                break;
            case PIXARLOGDATAFMT_16BIT:
                horizontalDifference16((uint16_t *)bp, llen, sp->stride, up,
                                       sp->From14);
                bp += llen * sizeof(uint16_t);
                break;
            case PIXARLOGDATAFMT_8BIT:
                horizontalDifference8((unsigned char *)bp, llen, sp->stride, up,
                                      sp->From8);
                bp += llen * sizeof(unsigned char);
                break;
            default:
                TIFFErrorExtR(tif, module,
                              "%" PRIu16 " bit input not supported in PixarLog",
                              td->td_bitspersample);
                return 0;
        }
    }

    sp->stream.next_in = (unsigned char *)sp->tbuf;
    assert(sizeof(sp->stream.avail_in) == 4); /* if this assert gets raised,
         we need to simplify this code to reflect a ZLib that is likely updated
         to deal with 8byte memory sizes, though this code will respond
         appropriately even before we simplify it */
    sp->stream.avail_in = (uInt)(n * sizeof(uint16_t));
    if ((sp->stream.avail_in / sizeof(uint16_t)) != (uInt)n)
    {
        TIFFErrorExtR(tif, module, "ZLib cannot deal with buffers this size");
        return (0);
    }

    do
    {
        if (deflate(&sp->stream, Z_NO_FLUSH) != Z_OK)
        {
            TIFFErrorExtR(tif, module, "Encoder error: %s",
                          sp->stream.msg ? sp->stream.msg : "(null)");
            return (0);
        }
        if (sp->stream.avail_out == 0)
        {
            tif->tif_rawcc = tif->tif_rawdatasize;
            if (!TIFFFlushData1(tif))
                return 0;
            sp->stream.next_out = tif->tif_rawdata;
            sp->stream.avail_out =
                (uInt)tif
                    ->tif_rawdatasize; /* this is a safe typecast, as check is
                                          made already in PixarLogPreEncode */
        }
    } while (sp->stream.avail_in > 0);
    return (1);
}
