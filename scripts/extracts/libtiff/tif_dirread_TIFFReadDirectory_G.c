/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFReadDirectory (part G) */
/* Lines: 4825–4931 */

        if (!TIFFFieldSet(tif, FIELD_BITSPERSAMPLE))
        {
            TIFFWarningExtR(
                tif, module,
                "BitsPerSample tag is missing, assuming 8 bits per sample");
            if (!TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8))
                goto bad;
        }
        if (!TIFFFieldSet(tif, FIELD_SAMPLESPERPIXEL))
        {
            if (tif->tif_dir.td_photometric == PHOTOMETRIC_RGB)
            {
                TIFFWarningExtR(tif, module,
                                "SamplesPerPixel tag is missing, "
                                "assuming correct SamplesPerPixel value is 3");
                if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3))
                    goto bad;
            }
            if (tif->tif_dir.td_photometric == PHOTOMETRIC_YCBCR)
            {
                TIFFWarningExtR(tif, module,
                                "SamplesPerPixel tag is missing, "
                                "applying correct SamplesPerPixel value of 3");
                if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3))
                    goto bad;
            }
            else if ((tif->tif_dir.td_photometric == PHOTOMETRIC_MINISWHITE) ||
                     (tif->tif_dir.td_photometric == PHOTOMETRIC_MINISBLACK))
            {
                /*
                 * SamplesPerPixel tag is missing, but is not required
                 * by spec.  Assume correct SamplesPerPixel value of 1.
                 */
                if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1))
                    goto bad;
            }
        }
    }

    /*
     * Setup appropriate structures (by strip or by tile)
     * We do that only after the above OJPEG hack which alters SamplesPerPixel
     * and thus influences the number of strips in the separate planarconfig.
     */
    if (!TIFFFieldSet(tif, FIELD_TILEDIMENSIONS))
    {
        tif->tif_dir.td_nstrips = TIFFNumberOfStrips(tif);
        tif->tif_dir.td_tilewidth = tif->tif_dir.td_imagewidth;
        tif->tif_dir.td_tilelength = tif->tif_dir.td_rowsperstrip;
        tif->tif_dir.td_tiledepth = tif->tif_dir.td_imagedepth;
        tif->tif_flags &= ~TIFF_ISTILED;
    }
    else
    {
        tif->tif_dir.td_nstrips = TIFFNumberOfTiles(tif);
        tif->tif_flags |= TIFF_ISTILED;
    }
    if (!tif->tif_dir.td_nstrips)
    {
        TIFFErrorExtR(tif, module, "Cannot handle zero number of %s",
                      isTiled(tif) ? "tiles" : "strips");
        goto bad;
    }
    tif->tif_dir.td_stripsperimage = tif->tif_dir.td_nstrips;
    if (tif->tif_dir.td_planarconfig == PLANARCONFIG_SEPARATE)
        tif->tif_dir.td_stripsperimage /= tif->tif_dir.td_samplesperpixel;
    if (!TIFFFieldSet(tif, FIELD_STRIPOFFSETS))
    {
#ifdef OJPEG_SUPPORT
        if ((tif->tif_dir.td_compression == COMPRESSION_OJPEG) &&
            (isTiled(tif) == 0) && (tif->tif_dir.td_nstrips == 1))
        {
            /*
             * XXX: OJPEG hack.
             * If a) compression is OJPEG, b) it's not a tiled TIFF,
             * and c) the number of strips is 1,
             * then we tolerate the absence of stripoffsets tag,
             * because, presumably, all required data is in the
             * JpegInterchangeFormat stream.
             */
            TIFFSetFieldBit(tif, FIELD_STRIPOFFSETS);
        }
        else
#endif
        {
            MissingRequired(tif, isTiled(tif) ? "TileOffsets" : "StripOffsets");
            goto bad;
        }
    }

    if (tif->tif_mode == O_RDWR &&
        tif->tif_dir.td_stripoffset_entry.tdir_tag != 0 &&
        tif->tif_dir.td_stripoffset_entry.tdir_count == 0 &&
        tif->tif_dir.td_stripoffset_entry.tdir_type == 0 &&
        tif->tif_dir.td_stripoffset_entry.tdir_offset.toff_long8 == 0 &&
        tif->tif_dir.td_stripbytecount_entry.tdir_tag != 0 &&
        tif->tif_dir.td_stripbytecount_entry.tdir_count == 0 &&
        tif->tif_dir.td_stripbytecount_entry.tdir_type == 0 &&
        tif->tif_dir.td_stripbytecount_entry.tdir_offset.toff_long8 == 0)
    {
        /* Directory typically created with TIFFDeferStrileArrayWriting() */
        TIFFSetupStrips(tif);
    }
    else if (!(tif->tif_flags & TIFF_DEFERSTRILELOAD))
    {
        if (tif->tif_dir.td_stripoffset_entry.tdir_tag != 0)
        {
