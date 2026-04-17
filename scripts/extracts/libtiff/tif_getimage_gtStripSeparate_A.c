/* ===== EXTRACT: tif_getimage.c ===== */
/* Function: gtStripSeparate (part A) */
/* Lines: 1178–1322 */

static int gtStripSeparate(TIFFRGBAImage *img, uint32_t *raster, uint32_t w,
                           uint32_t h)
{
    TIFF *tif = img->tif;
    tileSeparateRoutine put = img->put.separate;
    unsigned char *buf = NULL;
    unsigned char *p0 = NULL, *p1 = NULL, *p2 = NULL, *pa = NULL;
    uint32_t row, y, nrow, rowstoread;
    tmsize_t pos;
    tmsize_t scanline;
    uint32_t rowsperstrip, offset_row;
    uint32_t imagewidth = img->width;
    tmsize_t stripsize;
    tmsize_t bufsize;
    int32_t fromskew, toskew;
    int alpha = img->alpha;
    int ret = 1, flip;
    uint16_t colorchannels;

    stripsize = TIFFStripSize(tif);
    bufsize =
        _TIFFMultiplySSize(tif, alpha ? 4 : 3, stripsize, "gtStripSeparate");
    if (bufsize == 0)
    {
        return (0);
    }

    flip = setorientation(img);
    if (flip & FLIP_VERTICALLY)
    {
        if (w > INT_MAX)
        {
            TIFFErrorExtR(tif, TIFFFileName(tif), "Width overflow");
            return (0);
        }
        y = h - 1;
        toskew = -(int32_t)(w + w);
    }
    else
    {
        y = 0;
        toskew = -(int32_t)(w - w);
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

    TIFFGetFieldDefaulted(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
    if (rowsperstrip == 0)
    {
        TIFFErrorExtR(tif, TIFFFileName(tif), "rowsperstrip is zero");
        return (0);
    }

    scanline = TIFFScanlineSize(tif);
    fromskew = (w < imagewidth ? imagewidth - w : 0);
    for (row = 0; row < h; row += nrow)
    {
        uint32_t temp;
        rowstoread = rowsperstrip - (row + img->row_offset) % rowsperstrip;
        nrow = (row + rowstoread > h ? h - row : rowstoread);
        offset_row = row + img->row_offset;
        temp = (row + img->row_offset) % rowsperstrip + nrow;
        if (scanline > 0 && temp > (size_t)(TIFF_TMSIZE_T_MAX / scanline))
        {
            TIFFErrorExtR(tif, TIFFFileName(tif),
                          "Integer overflow in gtStripSeparate");
            return 0;
        }
        if (buf == NULL)
        {
            if (_TIFFReadEncodedStripAndAllocBuffer(
                    tif, TIFFComputeStrip(tif, offset_row, 0), (void **)&buf,
                    bufsize, temp * scanline) == (tmsize_t)(-1) &&
                (buf == NULL || img->stoponerr))
            {
                ret = 0;
                break;
            }
            p0 = buf;
            if (colorchannels == 1)
            {
                p2 = p1 = p0;
                pa = (alpha ? (p0 + 3 * stripsize) : NULL);
            }
            else
            {
                p1 = p0 + stripsize;
                p2 = p1 + stripsize;
                pa = (alpha ? (p2 + stripsize) : NULL);
            }
        }
        else if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, offset_row, 0),
                                      p0, temp * scanline) == (tmsize_t)(-1) &&
                 img->stoponerr)
        {
            ret = 0;
            break;
        }
        if (colorchannels > 1 &&
            TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, offset_row, 1), p1,
                                 temp * scanline) == (tmsize_t)(-1) &&
            img->stoponerr)
        {
            ret = 0;
            break;
        }
        if (colorchannels > 1 &&
            TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, offset_row, 2), p2,
                                 temp * scanline) == (tmsize_t)(-1) &&
            img->stoponerr)
        {
            ret = 0;
            break;
        }
        if (alpha)
        {
            if (TIFFReadEncodedStrip(
                    tif, TIFFComputeStrip(tif, offset_row, colorchannels), pa,
                    temp * scanline) == (tmsize_t)(-1) &&
                img->stoponerr)
            {
                ret = 0;
                break;
            }
        }

        pos = ((row + img->row_offset) % rowsperstrip) * scanline +
              ((tmsize_t)img->col_offset * img->samplesperpixel);
        tmsize_t roffset = (tmsize_t)y * w;
        (*put)(img, raster + roffset, 0, y, w, nrow, fromskew, toskew, p0 + pos,
               p1 + pos, p2 + pos, (alpha ? (pa + pos) : NULL));
        y += ((flip & FLIP_VERTICALLY) ? -(int32_t)nrow : (int32_t)nrow);
    }

