/* ===== EXTRACT: tif_dirread.c ===== */
/* Function: TIFFReadDirectory (part J) */
/* Lines: 5147–5199 */

    if (tif->tif_dir.td_planarconfig == PLANARCONFIG_CONTIG &&
        tif->tif_dir.td_compression == COMPRESSION_NONE &&
        (tif->tif_flags & (TIFF_STRIPCHOP | TIFF_ISTILED)) == TIFF_STRIPCHOP &&
        TIFFStripSize64(tif) > 0x7FFFFFFFUL)
    {
        TryChopUpUncompressedBigTiff(tif);
    }

    /*
     * Clear the dirty directory flag.
     */
    tif->tif_flags &= ~TIFF_DIRTYDIRECT;
    tif->tif_flags &= ~TIFF_DIRTYSTRIP;

    /*
     * Reinitialize i/o since we are starting on a new directory.
     */
    tif->tif_row = (uint32_t)-1;
    tif->tif_curstrip = (uint32_t)-1;
    tif->tif_col = (uint32_t)-1;
    tif->tif_curtile = (uint32_t)-1;
    tif->tif_tilesize = (tmsize_t)-1;

    tif->tif_scanlinesize = TIFFScanlineSize(tif);
    if (!tif->tif_scanlinesize)
    {
        TIFFErrorExtR(tif, module, "Cannot handle zero scanline size");
        return (0);
    }

    if (isTiled(tif))
    {
        tif->tif_tilesize = TIFFTileSize(tif);
        if (!tif->tif_tilesize)
        {
            TIFFErrorExtR(tif, module, "Cannot handle zero tile size");
            return (0);
        }
    }
    else
    {
        if (!TIFFStripSize(tif))
        {
            TIFFErrorExtR(tif, module, "Cannot handle zero strip size");
            return (0);
        }
    }
    return (1);
bad:
    if (dir)
        _TIFFfreeExt(tif, dir);
    return (0);
} /*-- TIFFReadDirectory() --*/
