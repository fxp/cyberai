/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFReadDirectory (part A) */
/* Lines: 4233–4326 */

int TIFFReadDirectory(TIFF *tif)
{
    static const char module[] = "TIFFReadDirectory";
    TIFFDirEntry *dir;
    uint16_t dircount;
    TIFFDirEntry *dp;
    uint16_t di;
    const TIFFField *fip;
    uint32_t fii = FAILED_FII;
    toff_t nextdiroff;
    int bitspersample_read = FALSE;
    int color_channels;

    if (tif->tif_nextdiroff == 0)
    {
        /* In this special case, tif_diroff needs also to be set to 0.
         * This is behind the last IFD, thus no checking or reading necessary.
         */
        tif->tif_diroff = tif->tif_nextdiroff;
        return 0;
    }

    nextdiroff = tif->tif_nextdiroff;
    /* tif_curdir++ and tif_nextdiroff should only be updated after SUCCESSFUL
     * reading of the directory. Otherwise, invalid IFD offsets could corrupt
     * the IFD list. */
    if (!_TIFFCheckDirNumberAndOffset(tif,
                                      tif->tif_curdir ==
                                              TIFF_NON_EXISTENT_DIR_NUMBER
                                          ? 0
                                          : tif->tif_curdir + 1,
                                      nextdiroff))
    {
        return 0; /* bad offset (IFD looping or more than TIFF_MAX_DIR_COUNT
                     IFDs) */
    }
    dircount = TIFFFetchDirectory(tif, nextdiroff, &dir, &tif->tif_nextdiroff);
    if (!dircount)
    {
        TIFFErrorExtR(tif, module,
                      "Failed to read directory at offset %" PRIu64,
                      nextdiroff);
        return 0;
    }
    /* Set global values after a valid directory has been fetched.
     * tif_diroff is already set to nextdiroff in TIFFFetchDirectory() in the
     * beginning. */
    if (tif->tif_curdir == TIFF_NON_EXISTENT_DIR_NUMBER)
        tif->tif_curdir = 0;
    else
        tif->tif_curdir++;

    TIFFReadDirectoryCheckOrder(tif, dir, dircount);

    /*
     * Mark duplicates of any tag to be ignored (bugzilla 1994)
     * to avoid certain pathological problems.
     */
    {
        TIFFDirEntry *ma;
        uint16_t mb;
        for (ma = dir, mb = 0; mb < dircount; ma++, mb++)
        {
            TIFFDirEntry *na;
            uint16_t nb;
            for (na = ma + 1, nb = mb + 1; nb < dircount; na++, nb++)
            {
                if (ma->tdir_tag == na->tdir_tag)
                {
                    na->tdir_ignore = TRUE;
                }
            }
        }
    }

    tif->tif_flags &= ~TIFF_BEENWRITING; /* reset before new dir */
    tif->tif_flags &= ~TIFF_BUF4WRITE;   /* reset before new dir */
    tif->tif_flags &= ~TIFF_CHOPPEDUPARRAYS;

    /* free any old stuff and reinit */
    (*tif->tif_cleanup)(tif); /* cleanup any previous compression state */
    TIFFFreeDirectory(tif);
    TIFFDefaultDirectory(tif);

    /* After setup a fresh directory indicate that now active IFD is also
     * present on file, even if its entries could not be read successfully
     * below.  */
    tif->tif_dir.td_iswrittentofile = TRUE;

    /* Allocate arrays for offset values outside IFD entry for IFD data size
     * checking. Note: Counter are reset within TIFFFreeDirectory(). */
    tif->tif_dir.td_dirdatasize_offsets =
        (TIFFEntryOffsetAndLength *)_TIFFmallocExt(
            tif, dircount * sizeof(TIFFEntryOffsetAndLength));
