/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFFetchNormalTag (part H) */
/* Lines: 7096–7224 */

                    if (!m)
                        return (0);
                }
            }
        }
        break;
        case TIFF_SETGET_C16_SINT32:
        {
            int32_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE);
            assert(fip->field_passcount == 1);
            if (dp->tdir_count > 0xFFFF)
                err = TIFFReadDirEntryErrCount;
            else
            {
                err = TIFFReadDirEntrySlongArray(tif, dp, &data);
                if (err == TIFFReadDirEntryErrOk)
                {
                    if (!EvaluateIFDdatasizeReading(tif, dp))
                    {
                        if (data != 0)
                            _TIFFfreeExt(tif, data);
                        return 0;
                    }
                    int m;
                    m = TIFFSetField(tif, dp->tdir_tag,
                                     (uint16_t)(dp->tdir_count), data);
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    if (!m)
                        return (0);
                }
            }
        }
        break;
        case TIFF_SETGET_C16_UINT64:
        {
            uint64_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE);
            assert(fip->field_passcount == 1);
            if (dp->tdir_count > 0xFFFF)
                err = TIFFReadDirEntryErrCount;
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
                    m = TIFFSetField(tif, dp->tdir_tag,
                                     (uint16_t)(dp->tdir_count), data);
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    if (!m)
                        return (0);
                }
            }
        }
        break;
        case TIFF_SETGET_C16_SINT64:
        {
            int64_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE);
            assert(fip->field_passcount == 1);
            if (dp->tdir_count > 0xFFFF)
                err = TIFFReadDirEntryErrCount;
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
                    m = TIFFSetField(tif, dp->tdir_tag,
                                     (uint16_t)(dp->tdir_count), data);
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    if (!m)
                        return (0);
                }
            }
        }
        break;
        case TIFF_SETGET_C16_FLOAT:
        {
            float *data;
            assert(fip->field_readcount == TIFF_VARIABLE);
            assert(fip->field_passcount == 1);
            if (dp->tdir_count > 0xFFFF)
                err = TIFFReadDirEntryErrCount;
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
                    m = TIFFSetField(tif, dp->tdir_tag,
                                     (uint16_t)(dp->tdir_count), data);
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    if (!m)
                        return (0);
                }
            }
        }
        break;
        case TIFF_SETGET_C16_DOUBLE:
        {
            double *data;
            assert(fip->field_readcount == TIFF_VARIABLE);
            assert(fip->field_passcount == 1);
            if (dp->tdir_count > 0xFFFF)
                err = TIFFReadDirEntryErrCount;
