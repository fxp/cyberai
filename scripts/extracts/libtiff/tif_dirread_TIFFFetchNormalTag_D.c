/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFFetchNormalTag (part D) */
/* Lines: 6624–6742 */

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
                err = TIFFReadDirEntryShortArray(tif, dp, &data);
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
        case TIFF_SETGET_C0_SINT16:
        {
            int16_t *data;
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
                err = TIFFReadDirEntrySshortArray(tif, dp, &data);
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
        case TIFF_SETGET_C0_UINT32:
        {
            uint32_t *data;
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
                err = TIFFReadDirEntryLongArray(tif, dp, &data);
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
        case TIFF_SETGET_C0_SINT32:
        {
            int32_t *data;
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
                err = TIFFReadDirEntrySlongArray(tif, dp, &data);
                if (err == TIFFReadDirEntryErrOk)
                {
