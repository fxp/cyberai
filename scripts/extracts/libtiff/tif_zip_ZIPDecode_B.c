/* ===== EXTRACT: tif_zip.c ===== */
/* Function: ZIPDecode (part B) */
/* Lines: 291–313 */

        if (state != Z_OK)
        {
            memset(sp->stream.next_out, 0, sp->stream.avail_out);
            TIFFErrorExtR(tif, module, "ZLib error: %s", SAFE_MSG(sp));
            sp->read_error = 1;
            return (0);
        }
    } while (occ > 0);
    if (occ != 0)
    {
        TIFFErrorExtR(tif, module,
                      "Not enough data at scanline %lu (short %" PRIu64
                      " bytes)",
                      (unsigned long)tif->tif_row, (uint64_t)occ);
        memset(sp->stream.next_out, 0, sp->stream.avail_out);
        sp->read_error = 1;
        return (0);
    }

    tif->tif_rawcp = sp->stream.next_in;

    return (1);
}
