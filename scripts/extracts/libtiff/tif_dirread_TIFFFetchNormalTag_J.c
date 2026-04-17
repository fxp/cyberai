/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFFetchNormalTag (part J) */
/* Lines: 7341–7465 */

                if ((err != TIFFReadDirEntryErrOk) || (origdata == 0))
                {
                    data = NULL;
                }
                else
                {
                    if (tif->tif_flags & TIFF_SWAB)
                        TIFFSwabArrayOfLong((uint32_t *)origdata, count);
                    data = (uint8_t *)origdata;
                    count = (uint32_t)(count * 4);
                }
            }
            else
            {
                err = TIFFReadDirEntryByteArray(tif, dp, &data);
                count = (uint32_t)(dp->tdir_count);
            }
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!EvaluateIFDdatasizeReading(tif, dp))
                {
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    return 0;
                }
                int m;
                m = TIFFSetField(tif, dp->tdir_tag, count, data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
                if (!m)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_C32_SINT8:
        {
            int8_t *data = NULL;
            assert(fip->field_readcount == TIFF_VARIABLE2);
            assert(fip->field_passcount == 1);
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
                m = TIFFSetField(tif, dp->tdir_tag, (uint32_t)(dp->tdir_count),
                                 data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
                if (!m)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_C32_UINT16:
        {
            uint16_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE2);
            assert(fip->field_passcount == 1);
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
                m = TIFFSetField(tif, dp->tdir_tag, (uint32_t)(dp->tdir_count),
                                 data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
                if (!m)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_C32_SINT16:
        {
            int16_t *data = NULL;
            assert(fip->field_readcount == TIFF_VARIABLE2);
            assert(fip->field_passcount == 1);
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
                m = TIFFSetField(tif, dp->tdir_tag, (uint32_t)(dp->tdir_count),
                                 data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
                if (!m)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_C32_UINT32:
        {
            uint32_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE2);
            assert(fip->field_passcount == 1);
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
                m = TIFFSetField(tif, dp->tdir_tag, (uint32_t)(dp->tdir_count),
                                 data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
