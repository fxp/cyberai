/* ===== EXTRACT: tif_luv.c ===== */
/* Function: LogLuvDecode24 */
/* Lines: 259–307 */

static int LogLuvDecode24(TIFF *tif, uint8_t *op, tmsize_t occ, uint16_t s)
{
    static const char module[] = "LogLuvDecode24";
    LogLuvState *sp = DecoderState(tif);
    tmsize_t cc;
    tmsize_t i;
    tmsize_t npixels;
    unsigned char *bp;
    uint32_t *tp;

    (void)s;
    assert(s == 0);
    assert(sp != NULL);

    npixels = occ / sp->pixel_size;

    if (sp->user_datafmt == SGILOGDATAFMT_RAW)
        tp = (uint32_t *)op;
    else
    {
        if (sp->tbuflen < npixels)
        {
            TIFFErrorExtR(tif, module, "Translation buffer too short");
            return (0);
        }
        tp = (uint32_t *)sp->tbuf;
    }
    /* copy to array of uint32_t */
    bp = (unsigned char *)tif->tif_rawcp;
    cc = tif->tif_rawcc;
    for (i = 0; i < npixels && cc >= 3; i++)
    {
        tp[i] = bp[0] << 16 | bp[1] << 8 | bp[2];
        bp += 3;
        cc -= 3;
    }
    tif->tif_rawcp = (uint8_t *)bp;
    tif->tif_rawcc = cc;
    if (i != npixels)
    {
        TIFFErrorExtR(tif, module,
                      "Not enough data at row %" PRIu32
                      " (short %" TIFF_SSIZE_FORMAT " pixels)",
                      tif->tif_row, npixels - i);
        return (0);
    }
    (*sp->tfunc)(sp, op, npixels);
    return (1);
}
