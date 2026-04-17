/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFFetchNormalTag (part F) */
/* Lines: 6858–6970 */

                    if (!m)
                        return (0);
                }
            }
        }
        break;
        /*--: Rational2Double: Extend for Double Arrays and Rational-Arrays read
         * into Double-Arrays. */
        case TIFF_SETGET_C0_DOUBLE:
        {
            double *data;
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
                    m = TIFFSetField(tif, dp->tdir_tag, data);
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    if (!m)
                        return (0);
                }
            }
        }
        break;
        case TIFF_SETGET_C16_ASCII:
        {
            uint8_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE);
            assert(fip->field_passcount == 1);
            if (dp->tdir_count > 0xFFFF)
                err = TIFFReadDirEntryErrCount;
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
                    if (data != 0 && dp->tdir_count > 0 &&
                        data[dp->tdir_count - 1] != '\0')
                    {
                        TIFFWarningExtR(tif, module,
                                        "ASCII value for ASCII array tag "
                                        "\"%s\" does not end in null "
                                        "byte. Forcing it to be null",
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
        case TIFF_SETGET_C16_UINT8:
        {
            uint8_t *data;
            assert(fip->field_readcount == TIFF_VARIABLE);
            assert(fip->field_passcount == 1);
            if (dp->tdir_count > 0xFFFF)
                err = TIFFReadDirEntryErrCount;
            else
            {
                err = TIFFReadDirEntryByteArray(tif, dp, &data);
                if (err == TIFFReadDirEntryErrOk)
                {
                    if (!EvaluateIFDdatasizeReading(tif, dp))
                    {
