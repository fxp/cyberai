/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFReadDirectory (part B) */
/* Lines: 4327–4436 */

    if (tif->tif_dir.td_dirdatasize_offsets == NULL)
    {
        TIFFErrorExtR(
            tif, module,
            "Failed to allocate memory for counting IFD data size at reading");
        goto bad;
    }
    /*
     * Electronic Arts writes gray-scale TIFF files
     * without a PlanarConfiguration directory entry.
     * Thus we setup a default value here, even though
     * the TIFF spec says there is no default value.
     * After PlanarConfiguration is preset in TIFFDefaultDirectory()
     * the following setting is not needed, but does not harm either.
     */
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    /*
     * Setup default value and then make a pass over
     * the fields to check type and tag information,
     * and to extract info required to size data
     * structures.  A second pass is made afterwards
     * to read in everything not taken in the first pass.
     * But we must process the Compression tag first
     * in order to merge in codec-private tag definitions (otherwise
     * we may get complaints about unknown tags).  However, the
     * Compression tag may be dependent on the SamplesPerPixel
     * tag value because older TIFF specs permitted Compression
     * to be written as a SamplesPerPixel-count tag entry.
     * Thus if we don't first figure out the correct SamplesPerPixel
     * tag value then we may end up ignoring the Compression tag
     * value because it has an incorrect count value (if the
     * true value of SamplesPerPixel is not 1).
     */
    dp =
        TIFFReadDirectoryFindEntry(tif, dir, dircount, TIFFTAG_SAMPLESPERPIXEL);
    if (dp)
    {
        if (!TIFFFetchNormalTag(tif, dp, 0))
            goto bad;
        dp->tdir_ignore = TRUE;
    }
    dp = TIFFReadDirectoryFindEntry(tif, dir, dircount, TIFFTAG_COMPRESSION);
    if (dp)
    {
        /*
         * The 5.0 spec says the Compression tag has one value, while
         * earlier specs say it has one value per sample.  Because of
         * this, we accept the tag if one value is supplied with either
         * count.
         */
        uint16_t value;
        enum TIFFReadDirEntryErr err;
        err = TIFFReadDirEntryShort(tif, dp, &value);
        if (err == TIFFReadDirEntryErrCount)
            err = TIFFReadDirEntryPersampleShort(tif, dp, &value);
        if (err != TIFFReadDirEntryErrOk)
        {
            TIFFReadDirEntryOutputErr(tif, err, module, "Compression", 0);
            goto bad;
        }
        if (!TIFFSetField(tif, TIFFTAG_COMPRESSION, value))
            goto bad;
        dp->tdir_ignore = TRUE;
    }
    else
    {
        if (!TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE))
            goto bad;
    }
    /*
     * First real pass over the directory.
     */
    for (di = 0, dp = dir; di < dircount; di++, dp++)
    {
        if (!dp->tdir_ignore)
        {
            TIFFReadDirectoryFindFieldInfo(tif, dp->tdir_tag, &fii);
            if (fii == FAILED_FII)
            {
                TIFFWarningExtR(tif, module,
                                "Unknown field with tag %" PRIu16 " (0x%" PRIx16
                                ") encountered",
                                dp->tdir_tag, dp->tdir_tag);
                /* the following knowingly leaks the
                   anonymous field structure */
                const TIFFField *fld = _TIFFCreateAnonField(
                    tif, dp->tdir_tag, (TIFFDataType)dp->tdir_type);
                if (fld == NULL || !_TIFFMergeFields(tif, fld, 1))
                {
                    TIFFWarningExtR(
                        tif, module,
                        "Registering anonymous field with tag %" PRIu16
                        " (0x%" PRIx16 ") failed",
                        dp->tdir_tag, dp->tdir_tag);
                    dp->tdir_ignore = TRUE;
                }
                else
                {
                    TIFFReadDirectoryFindFieldInfo(tif, dp->tdir_tag, &fii);
                    assert(fii != FAILED_FII);
                }
            }
        }
        if (!dp->tdir_ignore)
        {
            fip = tif->tif_fields[fii];
            if (fip->field_bit == FIELD_IGNORE)
                dp->tdir_ignore = TRUE;
            else
            {
