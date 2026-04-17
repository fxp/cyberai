/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFFetchNormalTag (part A) */
/* Lines: 6261–6365 */

static int TIFFFetchNormalTag(TIFF *tif, TIFFDirEntry *dp, int recover)
{
    static const char module[] = "TIFFFetchNormalTag";
    enum TIFFReadDirEntryErr err;
    uint32_t fii;
    const TIFFField *fip = NULL;
    TIFFReadDirectoryFindFieldInfo(tif, dp->tdir_tag, &fii);
    if (fii == FAILED_FII)
    {
        TIFFErrorExtR(tif, "TIFFFetchNormalTag",
                      "No definition found for tag %" PRIu16, dp->tdir_tag);
        return 0;
    }
    fip = tif->tif_fields[fii];
    assert(fip != NULL); /* should not happen */
    assert(fip->set_field_type !=
           TIFF_SETGET_OTHER); /* if so, we shouldn't arrive here but deal with
                                  this in specialized code */
    assert(fip->set_field_type !=
           TIFF_SETGET_INT); /* if so, we shouldn't arrive here as this is only
                                the case for pseudo-tags */
    err = TIFFReadDirEntryErrOk;
    switch (fip->set_field_type)
    {
        case TIFF_SETGET_UNDEFINED:
            TIFFErrorExtR(
                tif, "TIFFFetchNormalTag",
                "Defined set_field_type of custom tag %u (%s) is "
                "TIFF_SETGET_UNDEFINED and thus tag is not read from file",
                fip->field_tag, fip->field_name);
            break;
        case TIFF_SETGET_ASCII:
        {
            uint8_t *data;
            assert(fip->field_passcount == 0);
            err = TIFFReadDirEntryByteArray(tif, dp, &data);
            if (err == TIFFReadDirEntryErrOk)
            {
                size_t mb = 0;
                int n;
                if (data != NULL)
                {
                    if (dp->tdir_count > 0 && data[dp->tdir_count - 1] == 0)
                    {
                        /* optimization: if data is known to be 0 terminated, we
                         * can use strlen() */
                        mb = strlen((const char *)data);
                    }
                    else
                    {
                        /* general case. equivalent to non-portable */
                        /* mb = strnlen((const char*)data,
                         * (uint32_t)dp->tdir_count); */
                        uint8_t *ma = data;
                        while (mb < (uint32_t)dp->tdir_count)
                        {
                            if (*ma == 0)
                                break;
                            ma++;
                            mb++;
                        }
                    }
                }
                if (!EvaluateIFDdatasizeReading(tif, dp))
                {
                    if (data != NULL)
                        _TIFFfreeExt(tif, data);
                    return (0);
                }
                if (mb + 1 < (uint32_t)dp->tdir_count)
                    TIFFWarningExtR(
                        tif, module,
                        "ASCII value for tag \"%s\" contains null byte in "
                        "value; value incorrectly truncated during reading due "
                        "to implementation limitations",
                        fip->field_name);
                else if (mb + 1 > (uint32_t)dp->tdir_count)
                {
                    TIFFWarningExtR(tif, module,
                                    "ASCII value for tag \"%s\" does not end "
                                    "in null byte. Forcing it to be null",
                                    fip->field_name);
                    /* TIFFReadDirEntryArrayWithLimit() ensures this can't be
                     * larger than MAX_SIZE_TAG_DATA */
                    assert((uint32_t)dp->tdir_count + 1 == dp->tdir_count + 1);
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
                    if (data != 0)
                        _TIFFfreeExt(tif, data);
                    data = o;
                }
                n = TIFFSetField(tif, dp->tdir_tag, data);
                if (data != 0)
                    _TIFFfreeExt(tif, data);
