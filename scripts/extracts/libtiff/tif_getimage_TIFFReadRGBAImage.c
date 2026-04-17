/* ===== EXTRACT: tif_getimage.c ===== */
/* Function: TIFFReadRGBAImage */
/* Lines: 637–642 */

int TIFFReadRGBAImage(TIFF *tif, uint32_t rwidth, uint32_t rheight,
                      uint32_t *raster, int stop)
{
    return TIFFReadRGBAImageOriented(tif, rwidth, rheight, raster,
                                     ORIENTATION_BOTLEFT, stop);
}
