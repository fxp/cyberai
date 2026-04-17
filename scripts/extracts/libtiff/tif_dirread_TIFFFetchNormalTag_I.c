/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFFetchNormalTag (part I) */
/* Lines: 7225–7340 */

            else
            {
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
        case TIFF_SETGET_C16_IFD8:
        {
            uint64_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE);
            assert(fip->field_passcount == 1);
            if (dp->tdir_count > 0xFFFF)
                err = TIFFReadDirEntryErrCount;
            else
            {
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
        case TIFF_SETGET_C32_ASCII:
        {
            uint8_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE2);
            assert(fip->field_passcount == 1);
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
                if (data != 0 && dp->tdir_count > 0 &&
                    data[dp->tdir_count - 1] != '\0')
                {
                    TIFFWarningExtR(
                        tif, module,
                        "ASCII value for ASCII array tag \"%s\" does not end "
                        "in null byte. Forcing it to be null",
                        fip->field_name);
                    /* Enlarge buffer and add terminating null. */
                    uint8_t *o =
                        _TIFFmallocExt(tif, (uint32_t)dp->tdir_count + 1);
                    if (o == NULL)
                    {
                        if (data != NULL)
                            _TIFFfreeExt(tif, data);
                        return (0);
                    }
                    if (dp->tdir_count > 0)
                    {
                        _TIFFmemcpy(o, data, (uint32_t)dp->tdir_count);
                    }
                    o[(uint32_t)dp->tdir_count] = 0;
                    dp->tdir_count++; /* Increment for added null. */
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    data = o;
                }
                m = TIFFSetField(tif, dp->tdir_tag, (uint32_t)(dp->tdir_count),
                                 data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
                if (!m)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_C32_UINT8:
        {
            uint8_t *data;
            uint32_t count = 0;
            assert(fip->field_readcount == TIFF_VARIABLE2);
            assert(fip->field_passcount == 1);
            if (fip->field_tag == TIFFTAG_RICHTIFFIPTC &&
                dp->tdir_type == TIFF_LONG)
            {
                /* Adobe's software (wrongly) writes RichTIFFIPTC tag with
                 * data type LONG instead of UNDEFINED. Work around this
                 * frequently found issue */
                void *origdata;
                err = TIFFReadDirEntryArray(tif, dp, &count, 4, &origdata);
