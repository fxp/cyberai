/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFFetchNormalTag (part C) */
/* Lines: 6504–6623 */

                if (!TIFFSetField(tif, dp->tdir_tag, data))
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_IFD8:
        {
            uint64_t data;
            assert(fip->field_readcount == 1);
            assert(fip->field_passcount == 0);
            err = TIFFReadDirEntryIfd8(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!EvaluateIFDdatasizeReading(tif, dp))
                    return 0;
                if (!TIFFSetField(tif, dp->tdir_tag, data))
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_UINT16_PAIR:
        {
            uint16_t *data;
            assert(fip->field_readcount == 2);
            assert(fip->field_passcount == 0);
            if (dp->tdir_count != 2)
            {
                TIFFWarningExtR(tif, module,
                                "incorrect count for field \"%s\", expected 2, "
                                "got %" PRIu64,
                                fip->field_name, dp->tdir_count);
                return (0);
            }
            err = TIFFReadDirEntryShortArray(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                int m;
                assert(data); /* avoid CLang static Analyzer false positive */
                m = TIFFSetField(tif, dp->tdir_tag, data[0], data[1]);
                _TIFFfreeExt(tif, data);
                if (!m)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_C0_UINT8:
        {
            uint8_t *data;
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
                err = TIFFReadDirEntryByteArray(tif, dp, &data);
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
        case TIFF_SETGET_C0_SINT8:
        {
            int8_t *data;
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
                err = TIFFReadDirEntrySbyteArray(tif, dp, &data);
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
        case TIFF_SETGET_C0_UINT16:
        {
            uint16_t *data;
            assert(fip->field_readcount >= 1);
            assert(fip->field_passcount == 0);
