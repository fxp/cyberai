/* ===== EXTRACT: tif_luv.c ===== */
/* Function: LogLuvEncode24 */
/* Lines: 544–594 */

static int LogLuvEncode24(TIFF *tif, uint8_t *bp, tmsize_t cc, uint16_t s)
{
    static const char module[] = "LogLuvEncode24";
    LogLuvState *sp = EncoderState(tif);
    tmsize_t i;
    tmsize_t npixels;
    tmsize_t occ;
    uint8_t *op;
    uint32_t *tp;

    (void)s;
    assert(s == 0);
    assert(sp != NULL);
    npixels = cc / sp->pixel_size;

    if (sp->user_datafmt == SGILOGDATAFMT_RAW)
        tp = (uint32_t *)bp;
    else
    {
        tp = (uint32_t *)sp->tbuf;
        if (sp->tbuflen < npixels)
        {
            TIFFErrorExtR(tif, module, "Translation buffer too short");
            return (0);
        }
        (*sp->tfunc)(sp, bp, npixels);
    }
    /* write out encoded pixels */
    op = tif->tif_rawcp;
    occ = tif->tif_rawdatasize - tif->tif_rawcc;
    for (i = npixels; i--;)
    {
        if (occ < 3)
        {
            tif->tif_rawcp = op;
            tif->tif_rawcc = tif->tif_rawdatasize - occ;
            if (!TIFFFlushData1(tif))
                return (0);
            op = tif->tif_rawcp;
            occ = tif->tif_rawdatasize - tif->tif_rawcc;
        }
        *op++ = (uint8_t)(*tp >> 16);
        *op++ = (uint8_t)(*tp >> 8 & 0xff);
        *op++ = (uint8_t)(*tp++ & 0xff);
        occ -= 3;
    }
    tif->tif_rawcp = op;
    tif->tif_rawcc = tif->tif_rawdatasize - occ;

    return (1);
}
