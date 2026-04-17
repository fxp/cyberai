/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFFetchNormalTag (part Z) */
/* Lines: 7466–7625 */

                if (!m)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_C32_SINT32:
        {
            int32_t *data = NULL;
            assert(fip->field_readcount == TIFF_VARIABLE2);
            assert(fip->field_passcount == 1);
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
                m = TIFFSetField(tif, dp->tdir_tag, (uint32_t)(dp->tdir_count),
                                 data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
                if (!m)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_C32_UINT64:
        {
            uint64_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE2);
            assert(fip->field_passcount == 1);
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
                m = TIFFSetField(tif, dp->tdir_tag, (uint32_t)(dp->tdir_count),
                                 data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
                if (!m)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_C32_SINT64:
        {
            int64_t *data = NULL;
            assert(fip->field_readcount == TIFF_VARIABLE2);
            assert(fip->field_passcount == 1);
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
                m = TIFFSetField(tif, dp->tdir_tag, (uint32_t)(dp->tdir_count),
                                 data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
                if (!m)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_C32_FLOAT:
        {
            float *data;
            assert(fip->field_readcount == TIFF_VARIABLE2);
            assert(fip->field_passcount == 1);
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
                m = TIFFSetField(tif, dp->tdir_tag, (uint32_t)(dp->tdir_count),
                                 data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
                if (!m)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_C32_DOUBLE:
        {
            double *data;
            assert(fip->field_readcount == TIFF_VARIABLE2);
            assert(fip->field_passcount == 1);
            err = TIFFReadDirEntryDoubleArray(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!EvaluateIFDdatasizeReading(tif, dp))
                {
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    return 0;
                }
                int m;
                m = TIFFSetField(tif, dp->tdir_tag, (uint32_t)(dp->tdir_count),
                                 data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
                if (!m)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_C32_IFD8:
        {
            uint64_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE2);
            assert(fip->field_passcount == 1);
            err = TIFFReadDirEntryIfd8Array(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!EvaluateIFDdatasizeReading(tif, dp))
                {
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    return 0;
                }
                int m;
                m = TIFFSetField(tif, dp->tdir_tag, (uint32_t)(dp->tdir_count),
                                 data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
                if (!m)
                    return (0);
            }
        }
        break;
        default:
            assert(0); /* we should never get here */
            break;
    }
    if (err != TIFFReadDirEntryErrOk)
    {
        TIFFReadDirEntryOutputErr(tif, err, module, fip->field_name, recover);
        return (0);
    }
    return (1);
}
