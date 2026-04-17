/* ===== EXTRACT: tif_getimage.c ===== */
/* Function: gtTileSeparate (part A) */
/* Lines: 852–993 */

static int gtTileSeparate(TIFFRGBAImage *img, uint32_t *raster, uint32_t w,
                          uint32_t h)
{
    TIFF *tif = img->tif;
    tileSeparateRoutine put = img->put.separate;
    uint32_t col, row, y, rowstoread;
    tmsize_t pos;
    uint32_t tw, th;
    unsigned char *buf = NULL;
    unsigned char *p0 = NULL;
    unsigned char *p1 = NULL;
    unsigned char *p2 = NULL;
    unsigned char *pa = NULL;
    tmsize_t tilesize;
    tmsize_t bufsize;
    int32_t fromskew, toskew;
    int alpha = img->alpha;
    uint32_t nrow;
    int ret = 1, flip;
    uint16_t colorchannels;
    uint32_t this_tw, tocol;
    int32_t this_toskew, leftmost_toskew;
    int32_t leftmost_fromskew;
    uint32_t leftmost_tw;

    tilesize = TIFFTileSize(tif);
    bufsize =
        _TIFFMultiplySSize(tif, alpha ? 4 : 3, tilesize, "gtTileSeparate");
    if (bufsize == 0)
    {
        return (0);
    }

    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tw);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &th);

    flip = setorientation(img);
    if (flip & FLIP_VERTICALLY)
    {
        if ((tw + w) > INT_MAX)
        {
            TIFFErrorExtR(tif, TIFFFileName(tif), "%s",
                          "unsupported tile size (too wide)");
            return (0);
        }
        y = h - 1;
        toskew = -(int32_t)(tw + w);
    }
    else
    {
        if (tw > (INT_MAX + w))
        {
            TIFFErrorExtR(tif, TIFFFileName(tif), "%s",
                          "unsupported tile size (too wide)");
            return (0);
        }
        y = 0;
        toskew = -(int32_t)(tw - w);
    }

    switch (img->photometric)
    {
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_MINISBLACK:
        case PHOTOMETRIC_PALETTE:
            colorchannels = 1;
            break;

        default:
            colorchannels = 3;
            break;
    }

    if (tw == 0 || th == 0)
    {
        TIFFErrorExtR(tif, TIFFFileName(tif), "tile width or height is zero");
        return (0);
    }

    /*
     *	Leftmost tile is clipped on left side if col_offset > 0.
     */
    leftmost_fromskew = img->col_offset % tw;
    leftmost_tw = tw - leftmost_fromskew;
    leftmost_toskew = toskew + leftmost_fromskew;
    for (row = 0; ret != 0 && row < h; row += nrow)
    {
        rowstoread = th - (row + img->row_offset) % th;
        nrow = (row + rowstoread > h ? h - row : rowstoread);
        fromskew = leftmost_fromskew;
        this_tw = leftmost_tw;
        this_toskew = leftmost_toskew;
        tocol = 0;
        col = img->col_offset;
        while (tocol < w)
        {
            if (buf == NULL)
            {
                if (_TIFFReadTileAndAllocBuffer(tif, (void **)&buf, bufsize,
                                                col, row + img->row_offset, 0,
                                                0) == (tmsize_t)(-1) &&
                    (buf == NULL || img->stoponerr))
                {
                    ret = 0;
                    break;
                }
                p0 = buf;
                if (colorchannels == 1)
                {
                    p2 = p1 = p0;
                    pa = (alpha ? (p0 + 3 * tilesize) : NULL);
                }
                else
                {
                    p1 = p0 + tilesize;
                    p2 = p1 + tilesize;
                    pa = (alpha ? (p2 + tilesize) : NULL);
                }
            }
            else if (TIFFReadTile(tif, p0, col, row + img->row_offset, 0, 0) ==
                         (tmsize_t)(-1) &&
                     img->stoponerr)
            {
                ret = 0;
                break;
            }
            if (colorchannels > 1 &&
                TIFFReadTile(tif, p1, col, row + img->row_offset, 0, 1) ==
                    (tmsize_t)(-1) &&
                img->stoponerr)
            {
                ret = 0;
                break;
            }
            if (colorchannels > 1 &&
                TIFFReadTile(tif, p2, col, row + img->row_offset, 0, 2) ==
                    (tmsize_t)(-1) &&
                img->stoponerr)
            {
                ret = 0;
                break;
            }
