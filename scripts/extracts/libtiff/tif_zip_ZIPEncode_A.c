/* ===== EXTRACT: tif_zip.c ===== */
/* Function: ZIPEncode (part A) */
/* Lines: 373–495 */

static int ZIPEncode(TIFF *tif, uint8_t *bp, tmsize_t cc, uint16_t s)
{
    static const char module[] = "ZIPEncode";
    ZIPState *sp = ZIPEncoderState(tif);

    assert(sp != NULL);
    assert(sp->state == ZSTATE_INIT_ENCODE);

    (void)s;

#if LIBDEFLATE_SUPPORT
    if (sp->libdeflate_state == 1)
        return 0;

    /* If we have libdeflate support and we are asked to write a whole */
    /* strip/tile, then go for using it */
    do
    {
        TIFFDirectory *td = &tif->tif_dir;

        if (sp->libdeflate_state == 0)
            break;
        if (sp->subcodec == DEFLATE_SUBCODEC_ZLIB)
            break;

        /* Libdeflate does not support the 0-compression level */
        if (sp->zipquality == Z_NO_COMPRESSION)
            break;

        /* Check if we are in the situation where we can use libdeflate */
        if (isTiled(tif))
        {
            if (TIFFTileSize64(tif) != (uint64_t)cc)
                break;
        }
        else
        {
            uint32_t strip_height = td->td_imagelength - tif->tif_row;
            if (strip_height > td->td_rowsperstrip)
                strip_height = td->td_rowsperstrip;
            if (TIFFVStripSize64(tif, strip_height) != (uint64_t)cc)
                break;
        }

        /* Check for overflow */
        if ((size_t)tif->tif_rawdatasize != (uint64_t)tif->tif_rawdatasize)
            break;
        if ((size_t)cc != (uint64_t)cc)
            break;

        /* Go for compression using libdeflate */
        {
            size_t nCompressedBytes;
            if (sp->libdeflate_enc == NULL)
            {
                /* To get results as good as zlib, we asked for an extra */
                /* level of compression */
                sp->libdeflate_enc = libdeflate_alloc_compressor(
                    sp->zipquality == Z_DEFAULT_COMPRESSION ? 7
                    : sp->zipquality >= 6 && sp->zipquality <= 9
                        ? sp->zipquality + 1
                        : sp->zipquality);
                if (sp->libdeflate_enc == NULL)
                {
                    TIFFErrorExtR(tif, module, "Cannot allocate compressor");
                    break;
                }
            }

            /* Make sure the output buffer is large enough for the worse case.
             */
            /* In TIFFWriteBufferSetup(), when libtiff allocates the buffer */
            /* we've taken a 10% margin over the uncompressed size, which should
             */
            /* be large enough even for the the worse case scenario. */
            if (libdeflate_zlib_compress_bound(sp->libdeflate_enc, (size_t)cc) >
                (size_t)tif->tif_rawdatasize)
            {
                break;
            }

            sp->libdeflate_state = 1;
            nCompressedBytes = libdeflate_zlib_compress(
                sp->libdeflate_enc, bp, (size_t)cc, tif->tif_rawdata,
                (size_t)tif->tif_rawdatasize);

            if (nCompressedBytes == 0)
            {
                TIFFErrorExtR(tif, module, "Encoder error at scanline %lu",
                              (unsigned long)tif->tif_row);
                return 0;
            }

            tif->tif_rawcc = nCompressedBytes;

            if (!TIFFFlushData1(tif))
                return 0;

            return 1;
        }
    } while (0);
    sp->libdeflate_state = 0;
#endif /* LIBDEFLATE_SUPPORT */

    sp->stream.next_in = bp;
    assert(sizeof(sp->stream.avail_in) == 4); /* if this assert gets raised,
         we need to simplify this code to reflect a ZLib that is likely updated
         to deal with 8byte memory sizes, though this code will respond
         appropriately even before we simplify it */
    do
    {
        uInt avail_in_before =
            (uint64_t)cc <= 0xFFFFFFFFU ? (uInt)cc : 0xFFFFFFFFU;
        sp->stream.avail_in = avail_in_before;
        /* coverity[overrun-buffer-arg] */
        if (deflate(&sp->stream, Z_NO_FLUSH) != Z_OK)
        {
            TIFFErrorExtR(tif, module, "Encoder error: %s", SAFE_MSG(sp));
            return (0);
        }
        if (sp->stream.avail_out == 0)
        {
            tif->tif_rawcc = tif->tif_rawdatasize;
