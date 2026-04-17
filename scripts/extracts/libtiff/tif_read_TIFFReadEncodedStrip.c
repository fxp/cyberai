/* ===== EXTRACT: tif_read.c ===== */
/* Function: TIFFReadEncodedStrip */
/* Lines: 525–564 */

tmsize_t TIFFReadEncodedStrip(TIFF *tif, uint32_t strip, void *buf,
                              tmsize_t size)
{
    static const char module[] = "TIFFReadEncodedStrip";
    TIFFDirectory *td = &tif->tif_dir;
    tmsize_t stripsize;
    uint16_t plane;

    stripsize = TIFFReadEncodedStripGetStripSize(tif, strip, &plane);
    if (stripsize == ((tmsize_t)(-1)))
        return ((tmsize_t)(-1));

    /* shortcut to avoid an extra memcpy() */
    if (td->td_compression == COMPRESSION_NONE && size != (tmsize_t)(-1) &&
        size >= stripsize && !isMapped(tif) &&
        ((tif->tif_flags & TIFF_NOREADRAW) == 0))
    {
        if (TIFFReadRawStrip1(tif, strip, buf, stripsize, module) != stripsize)
            return ((tmsize_t)(-1));

        if (!isFillOrder(tif, td->td_fillorder) &&
            (tif->tif_flags & TIFF_NOBITREV) == 0)
            TIFFReverseBits(buf, stripsize);

        (*tif->tif_postdecode)(tif, buf, stripsize);
        return (stripsize);
    }

    if ((size != (tmsize_t)(-1)) && (size < stripsize))
        stripsize = size;
    if (!TIFFFillStrip(tif, strip))
    {
        memset(buf, 0, (size_t)stripsize);
        return ((tmsize_t)(-1));
    }
    if ((*tif->tif_decodestrip)(tif, buf, stripsize, plane) <= 0)
        return ((tmsize_t)(-1));
    (*tif->tif_postdecode)(tif, buf, stripsize);
    return (stripsize);
}
