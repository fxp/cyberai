/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFReadDirectory (part H) */
/* Lines: 4932–5043 */

            if (!TIFFFetchStripThing(tif, &(tif->tif_dir.td_stripoffset_entry),
                                     tif->tif_dir.td_nstrips,
                                     &tif->tif_dir.td_stripoffset_p))
            {
                goto bad;
            }
        }
        if (tif->tif_dir.td_stripbytecount_entry.tdir_tag != 0)
        {
            if (!TIFFFetchStripThing(
                    tif, &(tif->tif_dir.td_stripbytecount_entry),
                    tif->tif_dir.td_nstrips, &tif->tif_dir.td_stripbytecount_p))
            {
                goto bad;
            }
        }
    }

    /*
     * Make sure all non-color channels are extrasamples.
     * If it's not the case, define them as such.
     */
    color_channels = _TIFFGetMaxColorChannels(tif->tif_dir.td_photometric);
    if (color_channels &&
        tif->tif_dir.td_samplesperpixel - tif->tif_dir.td_extrasamples >
            color_channels)
    {
        uint16_t old_extrasamples;
        uint16_t *new_sampleinfo;

        TIFFWarningExtR(
            tif, module,
            "Sum of Photometric type-related "
            "color channels and ExtraSamples doesn't match SamplesPerPixel. "
            "Defining non-color channels as ExtraSamples.");

        old_extrasamples = tif->tif_dir.td_extrasamples;
        tif->tif_dir.td_extrasamples =
            (uint16_t)(tif->tif_dir.td_samplesperpixel - color_channels);

        // sampleinfo should contain information relative to these new extra
        // samples
        new_sampleinfo = (uint16_t *)_TIFFcallocExt(
            tif, tif->tif_dir.td_extrasamples, sizeof(uint16_t));
        if (!new_sampleinfo)
        {
            TIFFErrorExtR(tif, module,
                          "Failed to allocate memory for "
                          "temporary new sampleinfo array "
                          "(%" PRIu16 " 16 bit elements)",
                          tif->tif_dir.td_extrasamples);
            goto bad;
        }

        if (old_extrasamples > 0)
            memcpy(new_sampleinfo, tif->tif_dir.td_sampleinfo,
                   old_extrasamples * sizeof(uint16_t));
        _TIFFsetShortArrayExt(tif, &tif->tif_dir.td_sampleinfo, new_sampleinfo,
                              tif->tif_dir.td_extrasamples);
        _TIFFfreeExt(tif, new_sampleinfo);
    }

    /*
     * Verify Palette image has a Colormap.
     */
    if (tif->tif_dir.td_photometric == PHOTOMETRIC_PALETTE &&
        !TIFFFieldSet(tif, FIELD_COLORMAP))
    {
        if (tif->tif_dir.td_bitspersample >= 8 &&
            tif->tif_dir.td_samplesperpixel == 3)
            tif->tif_dir.td_photometric = PHOTOMETRIC_RGB;
        else if (tif->tif_dir.td_bitspersample >= 8)
            tif->tif_dir.td_photometric = PHOTOMETRIC_MINISBLACK;
        else
        {
            MissingRequired(tif, "Colormap");
            goto bad;
        }
    }
    /*
     * OJPEG hack:
     * We do no further messing with strip/tile offsets/bytecounts in OJPEG
     * TIFFs
     */
    if (tif->tif_dir.td_compression != COMPRESSION_OJPEG)
    {
        /*
         * Attempt to deal with a missing StripByteCounts tag.
         */
        if (!TIFFFieldSet(tif, FIELD_STRIPBYTECOUNTS))
        {
            /*
             * Some manufacturers violate the spec by not giving
             * the size of the strips.  In this case, assume there
             * is one uncompressed strip of data.
             */
            if ((tif->tif_dir.td_planarconfig == PLANARCONFIG_CONTIG &&
                 tif->tif_dir.td_nstrips > 1) ||
                (tif->tif_dir.td_planarconfig == PLANARCONFIG_SEPARATE &&
                 tif->tif_dir.td_nstrips !=
                     (uint32_t)tif->tif_dir.td_samplesperpixel))
            {
                MissingRequired(tif, "StripByteCounts");
                goto bad;
            }
            TIFFWarningExtR(
                tif, module,
                "TIFF directory is missing required "
                "\"StripByteCounts\" field, calculating from imagelength");
            if (EstimateStripByteCounts(tif, dir, dircount) < 0)
                goto bad;
        }
