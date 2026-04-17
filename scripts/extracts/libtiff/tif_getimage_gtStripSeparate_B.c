/* ===== EXTRACT: tif_getimage.c ===== */
/* Function: gtStripSeparate (part B) */
/* Lines: 1323–1345 */

    if (flip & FLIP_HORIZONTALLY)
    {
        uint32_t line;

        for (line = 0; line < h; line++)
        {
            uint32_t *left = raster + (line * w);
            uint32_t *right = left + w - 1;

            while (left < right)
            {
                uint32_t temp = *left;
                *left = *right;
                *right = temp;
                left++;
                right--;
            }
        }
    }

    _TIFFfreeExt(img->tif, buf);
    return (ret);
}
