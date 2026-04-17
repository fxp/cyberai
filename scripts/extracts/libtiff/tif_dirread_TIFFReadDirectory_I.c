/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFReadDirectory (part I) */
/* Lines: 5044–5146 */

        else if (tif->tif_dir.td_nstrips == 1 &&
                 !(tif->tif_flags & TIFF_ISTILED) && ByteCountLooksBad(tif))
        {
            /*
             * XXX: Plexus (and others) sometimes give a value of
             * zero for a tag when they don't know what the
             * correct value is!  Try and handle the simple case
             * of estimating the size of a one strip image.
             */
            TIFFWarningExtR(tif, module,
                            "Bogus \"StripByteCounts\" field, ignoring and "
                            "calculating from imagelength");
            if (EstimateStripByteCounts(tif, dir, dircount) < 0)
                goto bad;
        }
        else if (!(tif->tif_flags & TIFF_DEFERSTRILELOAD) &&
                 tif->tif_dir.td_planarconfig == PLANARCONFIG_CONTIG &&
                 tif->tif_dir.td_nstrips > 2 &&
                 tif->tif_dir.td_compression == COMPRESSION_NONE &&
                 TIFFGetStrileByteCount(tif, 0) !=
                     TIFFGetStrileByteCount(tif, 1) &&
                 TIFFGetStrileByteCount(tif, 0) != 0 &&
                 TIFFGetStrileByteCount(tif, 1) != 0)
        {
            /*
             * XXX: Some vendors fill StripByteCount array with
             * absolutely wrong values (it can be equal to
             * StripOffset array, for example). Catch this case
             * here.
             *
             * We avoid this check if deferring strile loading
             * as it would always force us to load the strip/tile
             * information.
             */
            TIFFWarningExtR(tif, module,
                            "Wrong \"StripByteCounts\" field, ignoring and "
                            "calculating from imagelength");
            if (EstimateStripByteCounts(tif, dir, dircount) < 0)
                goto bad;
        }
    }
    if (dir)
    {
        _TIFFfreeExt(tif, dir);
        dir = NULL;
    }
    if (!TIFFFieldSet(tif, FIELD_MAXSAMPLEVALUE))
    {
        if (tif->tif_dir.td_bitspersample >= 16)
            tif->tif_dir.td_maxsamplevalue = 0xFFFF;
        else
            tif->tif_dir.td_maxsamplevalue =
                (uint16_t)((1L << tif->tif_dir.td_bitspersample) - 1);
    }

#ifdef STRIPBYTECOUNTSORTED_UNUSED
    /*
     * XXX: We can optimize checking for the strip bounds using the sorted
     * bytecounts array. See also comments for TIFFAppendToStrip()
     * function in tif_write.c.
     */
    if (!(tif->tif_flags & TIFF_DEFERSTRILELOAD) && tif->tif_dir.td_nstrips > 1)
    {
        uint32_t strip;

        tif->tif_dir.td_stripbytecountsorted = 1;
        for (strip = 1; strip < tif->tif_dir.td_nstrips; strip++)
        {
            if (TIFFGetStrileOffset(tif, strip - 1) >
                TIFFGetStrileOffset(tif, strip))
            {
                tif->tif_dir.td_stripbytecountsorted = 0;
                break;
            }
        }
    }
#endif

    /*
     * An opportunity for compression mode dependent tag fixup
     */
    (*tif->tif_fixuptags)(tif);

    /*
     * Some manufacturers make life difficult by writing
     * large amounts of uncompressed data as a single strip.
     * This is contrary to the recommendations of the spec.
     * The following makes an attempt at breaking such images
     * into strips closer to the recommended 8k bytes.  A
     * side effect, however, is that the RowsPerStrip tag
     * value may be changed.
     */
    if ((tif->tif_dir.td_planarconfig == PLANARCONFIG_CONTIG) &&
        (tif->tif_dir.td_nstrips == 1) &&
        (tif->tif_dir.td_compression == COMPRESSION_NONE) &&
        ((tif->tif_flags & (TIFF_STRIPCHOP | TIFF_ISTILED)) == TIFF_STRIPCHOP))
    {
        ChopUpSingleUncompressedStrip(tif);
    }

    /* There are also uncompressed striped files with strips larger than */
    /* 2 GB, which make them unfriendly with a lot of code. If possible, */
    /* try to expose smaller "virtual" strips. */
