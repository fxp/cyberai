/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFReadDirectory (part F) */
/* Lines: 4731–4824 */

                        if (value != 0)
                            TIFFSetField(tif, TIFFTAG_SUBFILETYPE, value);
                    }
                }
                break;
                /* END REV 4.0 COMPATIBILITY */
#if 0
                case TIFFTAG_EP_BATTERYLEVEL:
                    /* TIFFTAG_EP_BATTERYLEVEL can be RATIONAL or ASCII.
                     * LibTiff defines it as ASCII and converts RATIONAL to an
                     * ASCII string. */
                    switch (dp->tdir_type)
                    {
                        case TIFF_RATIONAL:
                        {
                            /* Read rational and convert to ASCII*/
                            enum TIFFReadDirEntryErr err;
                            TIFFRational_t rValue;
                            err = TIFFReadDirEntryCheckedRationalDirect(
                                tif, dp, &rValue);
                            if (err != TIFFReadDirEntryErrOk)
                            {
                                fip = TIFFFieldWithTag(tif, dp->tdir_tag);
                                TIFFReadDirEntryOutputErr(
                                    tif, err, module,
                                    fip ? fip->field_name : "unknown tagname",
                                    1);
                            }
                            else
                            {
                                char szAux[32];
                                snprintf(szAux, sizeof(szAux) - 1, "%d/%d",
                                         rValue.uNum, rValue.uDenom);
                                TIFFSetField(tif, dp->tdir_tag, szAux);
                            }
                        }
                        break;
                        case TIFF_ASCII:
                            (void)TIFFFetchNormalTag(tif, dp, TRUE);
                            break;
                        default:
                            fip = TIFFFieldWithTag(tif, dp->tdir_tag);
                            TIFFWarningExtR(tif, module,
                                            "Invalid data type for tag %s. "
                                            "ASCII or RATIONAL expected",
                                            fip ? fip->field_name
                                                : "unknown tagname");
                            break;
                    }
                    break;
#endif
                default:
                    (void)TIFFFetchNormalTag(tif, dp, TRUE);
                    break;
            } /* -- switch (dp->tdir_tag) -- */
        }     /* -- if (!dp->tdir_ignore) */
    }         /* -- for-loop -- */

    /* Evaluate final IFD data size. */
    CalcFinalIFDdatasizeReading(tif, dircount);

    /*
     * OJPEG hack:
     * - If a) compression is OJPEG, and b) photometric tag is missing,
     * then we consistently find that photometric should be YCbCr
     * - If a) compression is OJPEG, and b) photometric tag says it's RGB,
     * then we consistently find that the buggy implementation of the
     * buggy compression scheme matches photometric YCbCr instead.
     * - If a) compression is OJPEG, and b) bitspersample tag is missing,
     * then we consistently find bitspersample should be 8.
     * - If a) compression is OJPEG, b) samplesperpixel tag is missing,
     * and c) photometric is RGB or YCbCr, then we consistently find
     * samplesperpixel should be 3
     * - If a) compression is OJPEG, b) samplesperpixel tag is missing,
     * and c) photometric is MINISWHITE or MINISBLACK, then we consistently
     * find samplesperpixel should be 3
     */
    if (tif->tif_dir.td_compression == COMPRESSION_OJPEG)
    {
        if (!TIFFFieldSet(tif, FIELD_PHOTOMETRIC))
        {
            TIFFWarningExtR(
                tif, module,
                "Photometric tag is missing, assuming data is YCbCr");
            if (!TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR))
                goto bad;
        }
        else if (tif->tif_dir.td_photometric == PHOTOMETRIC_RGB)
        {
            tif->tif_dir.td_photometric = PHOTOMETRIC_YCBCR;
            TIFFWarningExtR(tif, module,
                            "Photometric tag value assumed incorrect, "
                            "assuming data is YCbCr instead of RGB");
        }
