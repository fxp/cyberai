/* ===== EXTRACT: tif_zip.c ===== */
/* Function: ZIPDecode (part A) */
/* Lines: 163–290 */

static int ZIPDecode(TIFF *tif, uint8_t *op, tmsize_t occ, uint16_t s)
{
    static const char module[] = "ZIPDecode";
    ZIPState *sp = ZIPDecoderState(tif);

    (void)s;
    assert(sp != NULL);
    assert(sp->state == ZSTATE_INIT_DECODE);

    if (sp->read_error)
    {
        memset(op, 0, (size_t)occ);
        TIFFErrorExtR(tif, module,
                      "ZIPDecode: Scanline %" PRIu32 " cannot be read due to "
                      "previous error",
                      tif->tif_row);
        return 0;
    }

#if LIBDEFLATE_SUPPORT
    if (sp->libdeflate_state == 1)
        return 0;

    /* If we have libdeflate support and we are asked to read a whole */
    /* strip/tile, then go for using it */
    do
    {
        TIFFDirectory *td = &tif->tif_dir;

        if (sp->libdeflate_state == 0)
            break;
        if (sp->subcodec == DEFLATE_SUBCODEC_ZLIB)
            break;

        /* Check if we are in the situation where we can use libdeflate */
        if (isTiled(tif))
        {
            if (TIFFTileSize64(tif) != (uint64_t)occ)
                break;
        }
        else
        {
            uint32_t strip_height = td->td_imagelength - tif->tif_row;
            if (strip_height > td->td_rowsperstrip)
                strip_height = td->td_rowsperstrip;
            if (TIFFVStripSize64(tif, strip_height) != (uint64_t)occ)
                break;
        }

        /* Check for overflow */
        if ((size_t)tif->tif_rawcc != (uint64_t)tif->tif_rawcc)
            break;
        if ((size_t)occ != (uint64_t)occ)
            break;

        /* Go for decompression using libdeflate */
        {
            enum libdeflate_result res;
            if (sp->libdeflate_dec == NULL)
            {
                sp->libdeflate_dec = libdeflate_alloc_decompressor();
                if (sp->libdeflate_dec == NULL)
                {
                    break;
                }
            }

            sp->libdeflate_state = 1;

            res = libdeflate_zlib_decompress(sp->libdeflate_dec, tif->tif_rawcp,
                                             (size_t)tif->tif_rawcc, op,
                                             (size_t)occ, NULL);

            tif->tif_rawcp += tif->tif_rawcc;
            tif->tif_rawcc = 0;

            /* We accept LIBDEFLATE_INSUFFICIENT_SPACE has a return */
            /* There are odd files in the wild where the last strip, when */
            /* it is smaller in height than td_rowsperstrip, actually contains
             */
            /* data for td_rowsperstrip lines. Just ignore that silently. */
            if (res != LIBDEFLATE_SUCCESS &&
                res != LIBDEFLATE_INSUFFICIENT_SPACE)
            {
                memset(op, 0, (size_t)occ);
                TIFFErrorExtR(tif, module, "Decoding error at scanline %lu",
                              (unsigned long)tif->tif_row);
                sp->read_error = 1;
                return 0;
            }

            return 1;
        }
    } while (0);
    sp->libdeflate_state = 0;
#endif /* LIBDEFLATE_SUPPORT */

    sp->stream.next_in = tif->tif_rawcp;

    sp->stream.next_out = op;
    assert(sizeof(sp->stream.avail_out) == 4); /* if this assert gets raised,
         we need to simplify this code to reflect a ZLib that is likely updated
         to deal with 8byte memory sizes, though this code will respond
         appropriately even before we simplify it */
    do
    {
        int state;
        uInt avail_in_before = (uint64_t)tif->tif_rawcc <= 0xFFFFFFFFU
                                   ? (uInt)tif->tif_rawcc
                                   : 0xFFFFFFFFU;
        uInt avail_out_before =
            (uint64_t)occ < 0xFFFFFFFFU ? (uInt)occ : 0xFFFFFFFFU;
        sp->stream.avail_in = avail_in_before;
        sp->stream.avail_out = avail_out_before;
        /* coverity[overrun-buffer-arg] */
        state = inflate(&sp->stream, Z_PARTIAL_FLUSH);
        tif->tif_rawcc -= (avail_in_before - sp->stream.avail_in);
        occ -= (avail_out_before - sp->stream.avail_out);
        if (state == Z_STREAM_END)
            break;
        if (state == Z_DATA_ERROR)
        {
            memset(sp->stream.next_out, 0, sp->stream.avail_out);
            TIFFErrorExtR(tif, module, "Decoding error at scanline %lu, %s",
                          (unsigned long)tif->tif_row, SAFE_MSG(sp));
            sp->read_error = 1;
            return (0);
        }
