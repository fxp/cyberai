/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFFetchNormalTag (part G) */
/* Lines: 6971–7095 */

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
        case TIFF_SETGET_C16_SINT8:
        {
            int8_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE);
            assert(fip->field_passcount == 1);
            if (dp->tdir_count > 0xFFFF)
                err = TIFFReadDirEntryErrCount;
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
        case TIFF_SETGET_C16_UINT16:
        {
            uint16_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE);
            assert(fip->field_passcount == 1);
            if (dp->tdir_count > 0xFFFF)
                err = TIFFReadDirEntryErrCount;
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
        case TIFF_SETGET_C16_SINT16:
        {
            int16_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE);
            assert(fip->field_passcount == 1);
            if (dp->tdir_count > 0xFFFF)
                err = TIFFReadDirEntryErrCount;
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
        case TIFF_SETGET_C16_UINT32:
        {
            uint32_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE);
            assert(fip->field_passcount == 1);
            if (dp->tdir_count > 0xFFFF)
                err = TIFFReadDirEntryErrCount;
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
                    m = TIFFSetField(tif, dp->tdir_tag,
                                     (uint16_t)(dp->tdir_count), data);
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
