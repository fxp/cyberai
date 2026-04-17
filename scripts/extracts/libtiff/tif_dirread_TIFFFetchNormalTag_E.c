/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFFetchNormalTag (part E) */
/* Lines: 6743–6857 */

                    if (!EvaluateIFDdatasizeReading(tif, dp))
                    {
                        if (data != 0)
                            _TIFFfreeExt(tif, data);
                        return 0;
                    }
                    int m;
                    m = TIFFSetField(tif, dp->tdir_tag, data);
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    if (!m)
                        return (0);
                }
            }
        }
        break;
        case TIFF_SETGET_C0_UINT64:
        {
            uint64_t *data;
            assert(fip->field_readcount >= 1);
            assert(fip->field_passcount == 0);
            if (dp->tdir_count != (uint64_t)fip->field_readcount)
            {
                TIFFWarningExtR(tif, module,
                                "incorrect count for field \"%s\", expected "
                                "%d, got %" PRIu64,
                                fip->field_name, (int)fip->field_readcount,
                                dp->tdir_count);
                return (0);
            }
            else
            {
                err = TIFFReadDirEntryLong8Array(tif, dp, &data);
                if (err == TIFFReadDirEntryErrOk)
                {
                    if (!EvaluateIFDdatasizeReading(tif, dp))
                    {
                        if (data != 0)
                            _TIFFfreeExt(tif, data);
                        return 0;
                    }
                    int m;
                    m = TIFFSetField(tif, dp->tdir_tag, data);
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    if (!m)
                        return (0);
                }
            }
        }
        break;
        case TIFF_SETGET_C0_SINT64:
        {
            int64_t *data;
            assert(fip->field_readcount >= 1);
            assert(fip->field_passcount == 0);
            if (dp->tdir_count != (uint64_t)fip->field_readcount)
            {
                TIFFWarningExtR(tif, module,
                                "incorrect count for field \"%s\", expected "
                                "%d, got %" PRIu64,
                                fip->field_name, (int)fip->field_readcount,
                                dp->tdir_count);
                return (0);
            }
            else
            {
                err = TIFFReadDirEntrySlong8Array(tif, dp, &data);
                if (err == TIFFReadDirEntryErrOk)
                {
                    if (!EvaluateIFDdatasizeReading(tif, dp))
                    {
                        if (data != 0)
                            _TIFFfreeExt(tif, data);
                        return 0;
                    }
                    int m;
                    m = TIFFSetField(tif, dp->tdir_tag, data);
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    if (!m)
                        return (0);
                }
            }
        }
        break;
        case TIFF_SETGET_C0_FLOAT:
        {
            float *data;
            assert(fip->field_readcount >= 1);
            assert(fip->field_passcount == 0);
            if (dp->tdir_count != (uint64_t)fip->field_readcount)
            {
                TIFFWarningExtR(tif, module,
                                "incorrect count for field \"%s\", expected "
                                "%d, got %" PRIu64,
                                fip->field_name, (int)fip->field_readcount,
                                dp->tdir_count);
                return (0);
            }
            else
            {
                err = TIFFReadDirEntryFloatArray(tif, dp, &data);
                if (err == TIFFReadDirEntryErrOk)
                {
                    if (!EvaluateIFDdatasizeReading(tif, dp))
                    {
                        if (data != 0)
                            _TIFFfreeExt(tif, data);
                        return 0;
                    }
                    int m;
                    m = TIFFSetField(tif, dp->tdir_tag, data);
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
