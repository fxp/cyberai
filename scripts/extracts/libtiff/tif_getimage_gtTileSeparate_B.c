/* ===== EXTRACT: tif_getimage.c ===== */
/* Function: gtTileSeparate (part B) */
/* Lines: 994–1054 */

            if (alpha &&
                TIFFReadTile(tif, pa, col, row + img->row_offset, 0,
                             colorchannels) == (tmsize_t)(-1) &&
                img->stoponerr)
            {
                ret = 0;
                break;
            }

            pos = ((row + img->row_offset) % th) * TIFFTileRowSize(tif) +
                  ((tmsize_t)fromskew * img->samplesperpixel);
            if (tocol + this_tw > w)
            {
                /*
                 * Rightmost tile is clipped on right side.
                 */
                fromskew = tw - (w - tocol);
                this_tw = tw - fromskew;
                this_toskew = toskew + fromskew;
            }
            tmsize_t roffset = (tmsize_t)y * w + tocol;
            (*put)(img, raster + roffset, tocol, y, this_tw, nrow, fromskew,
                   this_toskew, p0 + pos, p1 + pos, p2 + pos,
                   (alpha ? (pa + pos) : NULL));
            tocol += this_tw;
            col += this_tw;
            /*
             * After the leftmost tile, tiles are no longer clipped on left
             * side.
             */
            fromskew = 0;
            this_tw = tw;
            this_toskew = toskew;
        }

        y += ((flip & FLIP_VERTICALLY) ? -(int32_t)nrow : (int32_t)nrow);
    }

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
