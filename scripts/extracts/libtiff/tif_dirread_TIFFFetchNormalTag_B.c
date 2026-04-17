/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFFetchNormalTag (part B) */
/* Lines: 6366–6503 */

                if (!n)
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_UINT8:
        {
            uint8_t data = 0;
            assert(fip->field_readcount == 1);
            assert(fip->field_passcount == 0);
            err = TIFFReadDirEntryByte(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!TIFFSetField(tif, dp->tdir_tag, data))
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_SINT8:
        {
            int8_t data = 0;
            assert(fip->field_readcount == 1);
            assert(fip->field_passcount == 0);
            err = TIFFReadDirEntrySbyte(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!TIFFSetField(tif, dp->tdir_tag, data))
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_UINT16:
        {
            uint16_t data;
            assert(fip->field_readcount == 1);
            assert(fip->field_passcount == 0);
            err = TIFFReadDirEntryShort(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!TIFFSetField(tif, dp->tdir_tag, data))
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_SINT16:
        {
            int16_t data;
            assert(fip->field_readcount == 1);
            assert(fip->field_passcount == 0);
            err = TIFFReadDirEntrySshort(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!TIFFSetField(tif, dp->tdir_tag, data))
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_UINT32:
        {
            uint32_t data;
            assert(fip->field_readcount == 1);
            assert(fip->field_passcount == 0);
            err = TIFFReadDirEntryLong(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!TIFFSetField(tif, dp->tdir_tag, data))
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_SINT32:
        {
            int32_t data;
            assert(fip->field_readcount == 1);
            assert(fip->field_passcount == 0);
            err = TIFFReadDirEntrySlong(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!TIFFSetField(tif, dp->tdir_tag, data))
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_UINT64:
        {
            uint64_t data;
            assert(fip->field_readcount == 1);
            assert(fip->field_passcount == 0);
            err = TIFFReadDirEntryLong8(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!EvaluateIFDdatasizeReading(tif, dp))
                    return 0;
                if (!TIFFSetField(tif, dp->tdir_tag, data))
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_SINT64:
        {
            int64_t data;
            assert(fip->field_readcount == 1);
            assert(fip->field_passcount == 0);
            err = TIFFReadDirEntrySlong8(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!EvaluateIFDdatasizeReading(tif, dp))
                    return 0;
                if (!TIFFSetField(tif, dp->tdir_tag, data))
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_FLOAT:
        {
            float data;
            assert(fip->field_readcount == 1);
            assert(fip->field_passcount == 0);
            err = TIFFReadDirEntryFloat(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!EvaluateIFDdatasizeReading(tif, dp))
                    return 0;
                if (!TIFFSetField(tif, dp->tdir_tag, data))
                    return (0);
            }
        }
        break;
        case TIFF_SETGET_DOUBLE:
        {
            double data;
            assert(fip->field_readcount == 1);
            assert(fip->field_passcount == 0);
            err = TIFFReadDirEntryDouble(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                if (!EvaluateIFDdatasizeReading(tif, dp))
                    return 0;
