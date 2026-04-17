/* ===== EXTRACT: tif_read.c ===== */
/* Function: TIFFReadScanline */
/* Lines: 447–472 */

int TIFFReadScanline(TIFF *tif, void *buf, uint32_t row, uint16_t sample)
{
    int e;

    if (!TIFFCheckRead(tif, 0))
        return (-1);
    if ((e = TIFFSeek(tif, row, sample)) != 0)
    {
        /*
         * Decompress desired row into user buffer.
         */
        e = (*tif->tif_decoderow)(tif, (uint8_t *)buf, tif->tif_scanlinesize,
                                  sample);

        /* we are now poised at the beginning of the next row */
        tif->tif_row = row + 1;

        if (e)
            (*tif->tif_postdecode)(tif, (uint8_t *)buf, tif->tif_scanlinesize);
    }
    else
    {
        memset(buf, 0, (size_t)tif->tif_scanlinesize);
    }
    return (e > 0 ? 1 : -1);
}
