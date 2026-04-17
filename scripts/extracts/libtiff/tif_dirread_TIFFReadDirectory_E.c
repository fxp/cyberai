/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFReadDirectory (part E) */
/* Lines: 4636–4730 */

                    if (!EvaluateIFDdatasizeReading(tif, dp))
                        goto bad;
                }
                break;
                case TIFFTAG_COLORMAP:
                case TIFFTAG_TRANSFERFUNCTION:
                {
                    enum TIFFReadDirEntryErr err;
                    uint32_t countpersample;
                    uint32_t countrequired;
                    uint32_t incrementpersample;
                    uint16_t *value = NULL;
                    /* It would be dangerous to instantiate those tag values */
                    /* since if td_bitspersample has not yet been read (due to
                     */
                    /* unordered tags), it could be read afterwards with a */
                    /* values greater than the default one (1), which may cause
                     */
                    /* crashes in user code */
                    if (!bitspersample_read)
                    {
                        fip = TIFFFieldWithTag(tif, dp->tdir_tag);
                        TIFFWarningExtR(
                            tif, module,
                            "Ignoring %s since BitsPerSample tag not found",
                            fip ? fip->field_name : "unknown tagname");
                        continue;
                    }
                    /* ColorMap or TransferFunction for high bit */
                    /* depths do not make much sense and could be */
                    /* used as a denial of service vector */
                    if (tif->tif_dir.td_bitspersample > 24)
                    {
                        fip = TIFFFieldWithTag(tif, dp->tdir_tag);
                        TIFFWarningExtR(
                            tif, module,
                            "Ignoring %s because BitsPerSample=%" PRIu16 ">24",
                            fip ? fip->field_name : "unknown tagname",
                            tif->tif_dir.td_bitspersample);
                        continue;
                    }
                    countpersample = (1U << tif->tif_dir.td_bitspersample);
                    if ((dp->tdir_tag == TIFFTAG_TRANSFERFUNCTION) &&
                        (dp->tdir_count == (uint64_t)countpersample))
                    {
                        countrequired = countpersample;
                        incrementpersample = 0;
                    }
                    else
                    {
                        countrequired = 3 * countpersample;
                        incrementpersample = countpersample;
                    }
                    if (dp->tdir_count != (uint64_t)countrequired)
                        err = TIFFReadDirEntryErrCount;
                    else
                        err = TIFFReadDirEntryShortArray(tif, dp, &value);
                    if (!EvaluateIFDdatasizeReading(tif, dp))
                        goto bad;
                    if (err != TIFFReadDirEntryErrOk)
                    {
                        fip = TIFFFieldWithTag(tif, dp->tdir_tag);
                        TIFFReadDirEntryOutputErr(
                            tif, err, module,
                            fip ? fip->field_name : "unknown tagname", 1);
                    }
                    else
                    {
                        TIFFSetField(tif, dp->tdir_tag, value,
                                     value + incrementpersample,
                                     value + 2 * incrementpersample);
                        _TIFFfreeExt(tif, value);
                    }
                }
                break;
                    /* BEGIN REV 4.0 COMPATIBILITY */
                case TIFFTAG_OSUBFILETYPE:
                {
                    uint16_t valueo;
                    uint32_t value;
                    if (TIFFReadDirEntryShort(tif, dp, &valueo) ==
                        TIFFReadDirEntryErrOk)
                    {
                        switch (valueo)
                        {
                            case OFILETYPE_REDUCEDIMAGE:
                                value = FILETYPE_REDUCEDIMAGE;
                                break;
                            case OFILETYPE_PAGE:
                                value = FILETYPE_PAGE;
                                break;
                            default:
                                value = 0;
                                break;
                        }
