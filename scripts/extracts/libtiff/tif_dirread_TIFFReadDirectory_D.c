/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFReadDirectory (part D) */
/* Lines: 4544–4635 */

                        if (!TIFFSetField(tif, dp->tdir_tag, value))
                            goto bad;
                        if (dp->tdir_tag == TIFFTAG_BITSPERSAMPLE)
                            bitspersample_read = TRUE;
                    }
                    break;
                case TIFFTAG_SMINSAMPLEVALUE:
                case TIFFTAG_SMAXSAMPLEVALUE:
                {

                    double *data = NULL;
                    enum TIFFReadDirEntryErr err;
                    uint32_t saved_flags;
                    int m;
                    if (dp->tdir_count !=
                        (uint64_t)tif->tif_dir.td_samplesperpixel)
                        err = TIFFReadDirEntryErrCount;
                    else
                        err = TIFFReadDirEntryDoubleArray(tif, dp, &data);
                    if (!EvaluateIFDdatasizeReading(tif, dp))
                        goto bad;
                    if (err != TIFFReadDirEntryErrOk)
                    {
                        fip = TIFFFieldWithTag(tif, dp->tdir_tag);
                        TIFFReadDirEntryOutputErr(
                            tif, err, module,
                            fip ? fip->field_name : "unknown tagname", 0);
                        goto bad;
                    }
                    saved_flags = tif->tif_flags;
                    tif->tif_flags |= TIFF_PERSAMPLE;
                    m = TIFFSetField(tif, dp->tdir_tag, data);
                    tif->tif_flags = saved_flags;
                    _TIFFfreeExt(tif, data);
                    if (!m)
                        goto bad;
                }
                break;
                case TIFFTAG_STRIPOFFSETS:
                case TIFFTAG_TILEOFFSETS:
                {
                    switch (dp->tdir_type)
                    {
                        case TIFF_SHORT:
                        case TIFF_LONG:
                        case TIFF_LONG8:
                            break;
                        default:
                            /* Warn except if directory typically created with
                             * TIFFDeferStrileArrayWriting() */
                            if (!(tif->tif_mode == O_RDWR &&
                                  dp->tdir_count == 0 && dp->tdir_type == 0 &&
                                  dp->tdir_offset.toff_long8 == 0))
                            {
                                fip = TIFFFieldWithTag(tif, dp->tdir_tag);
                                TIFFWarningExtR(
                                    tif, module, "Invalid data type for tag %s",
                                    fip ? fip->field_name : "unknown tagname");
                            }
                            break;
                    }
                    _TIFFmemcpy(&(tif->tif_dir.td_stripoffset_entry), dp,
                                sizeof(TIFFDirEntry));
                    if (!EvaluateIFDdatasizeReading(tif, dp))
                        goto bad;
                }
                break;
                case TIFFTAG_STRIPBYTECOUNTS:
                case TIFFTAG_TILEBYTECOUNTS:
                {
                    switch (dp->tdir_type)
                    {
                        case TIFF_SHORT:
                        case TIFF_LONG:
                        case TIFF_LONG8:
                            break;
                        default:
                            /* Warn except if directory typically created with
                             * TIFFDeferStrileArrayWriting() */
                            if (!(tif->tif_mode == O_RDWR &&
                                  dp->tdir_count == 0 && dp->tdir_type == 0 &&
                                  dp->tdir_offset.toff_long8 == 0))
                            {
                                fip = TIFFFieldWithTag(tif, dp->tdir_tag);
                                TIFFWarningExtR(
                                    tif, module, "Invalid data type for tag %s",
                                    fip ? fip->field_name : "unknown tagname");
                            }
                            break;
                    }
                    _TIFFmemcpy(&(tif->tif_dir.td_stripbytecount_entry), dp,
                                sizeof(TIFFDirEntry));
