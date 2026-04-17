/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFReadDirectory (part C) */
/* Lines: 4437–4543 */

                switch (dp->tdir_tag)
                {
                    case TIFFTAG_STRIPOFFSETS:
                    case TIFFTAG_STRIPBYTECOUNTS:
                    case TIFFTAG_TILEOFFSETS:
                    case TIFFTAG_TILEBYTECOUNTS:
                        TIFFSetFieldBit(tif, fip->field_bit);
                        break;
                    case TIFFTAG_IMAGEWIDTH:
                    case TIFFTAG_IMAGELENGTH:
                    case TIFFTAG_IMAGEDEPTH:
                    case TIFFTAG_TILELENGTH:
                    case TIFFTAG_TILEWIDTH:
                    case TIFFTAG_TILEDEPTH:
                    case TIFFTAG_PLANARCONFIG:
                    case TIFFTAG_ROWSPERSTRIP:
                    case TIFFTAG_EXTRASAMPLES:
                        if (!TIFFFetchNormalTag(tif, dp, 0))
                            goto bad;
                        dp->tdir_ignore = TRUE;
                        break;
                    default:
                        if (!_TIFFCheckFieldIsValidForCodec(tif, dp->tdir_tag))
                            dp->tdir_ignore = TRUE;
                        break;
                }
            }
        }
    }
    /*
     * XXX: OJPEG hack.
     * If a) compression is OJPEG, b) planarconfig tag says it's separate,
     * c) strip offsets/bytecounts tag are both present and
     * d) both contain exactly one value, then we consistently find
     * that the buggy implementation of the buggy compression scheme
     * matches contig planarconfig best. So we 'fix-up' the tag here
     */
    if ((tif->tif_dir.td_compression == COMPRESSION_OJPEG) &&
        (tif->tif_dir.td_planarconfig == PLANARCONFIG_SEPARATE))
    {
        if (!_TIFFFillStriles(tif))
            goto bad;
        dp = TIFFReadDirectoryFindEntry(tif, dir, dircount,
                                        TIFFTAG_STRIPOFFSETS);
        if ((dp != 0) && (dp->tdir_count == 1))
        {
            dp = TIFFReadDirectoryFindEntry(tif, dir, dircount,
                                            TIFFTAG_STRIPBYTECOUNTS);
            if ((dp != 0) && (dp->tdir_count == 1))
            {
                tif->tif_dir.td_planarconfig = PLANARCONFIG_CONTIG;
                TIFFWarningExtR(tif, module,
                                "Planarconfig tag value assumed incorrect, "
                                "assuming data is contig instead of chunky");
            }
        }
    }
    /*
     * Allocate directory structure and setup defaults.
     */
    if (!TIFFFieldSet(tif, FIELD_IMAGEDIMENSIONS))
    {
        MissingRequired(tif, "ImageLength");
        goto bad;
    }

    /*
     * Second pass: extract other information.
     */
    for (di = 0, dp = dir; di < dircount; di++, dp++)
    {
        if (!dp->tdir_ignore)
        {
            switch (dp->tdir_tag)
            {
                case TIFFTAG_MINSAMPLEVALUE:
                case TIFFTAG_MAXSAMPLEVALUE:
                case TIFFTAG_BITSPERSAMPLE:
                case TIFFTAG_DATATYPE:
                case TIFFTAG_SAMPLEFORMAT:
                    /*
                     * The MinSampleValue, MaxSampleValue, BitsPerSample
                     * DataType and SampleFormat tags are supposed to be
                     * written as one value/sample, but some vendors
                     * incorrectly write one value only -- so we accept
                     * that as well (yuck). Other vendors write correct
                     * value for NumberOfSamples, but incorrect one for
                     * BitsPerSample and friends, and we will read this
                     * too.
                     */
                    {
                        uint16_t value;
                        enum TIFFReadDirEntryErr err;
                        err = TIFFReadDirEntryShort(tif, dp, &value);
                        if (!EvaluateIFDdatasizeReading(tif, dp))
                            goto bad;
                        if (err == TIFFReadDirEntryErrCount)
                            err =
                                TIFFReadDirEntryPersampleShort(tif, dp, &value);
                        if (err != TIFFReadDirEntryErrOk)
                        {
                            fip = TIFFFieldWithTag(tif, dp->tdir_tag);
                            TIFFReadDirEntryOutputErr(
                                tif, err, module,
                                fip ? fip->field_name : "unknown tagname", 0);
                            goto bad;
                        }
