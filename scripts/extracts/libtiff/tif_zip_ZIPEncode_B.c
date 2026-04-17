/* ===== EXTRACT: tif_zip.c ===== */
/* Function: ZIPEncode (part B) */
/* Lines: 496–506 */

            if (!TIFFFlushData1(tif))
                return 0;
            sp->stream.next_out = tif->tif_rawdata;
            sp->stream.avail_out = (uint64_t)tif->tif_rawdatasize <= 0xFFFFFFFFU
                                       ? (uInt)tif->tif_rawdatasize
                                       : 0xFFFFFFFFU;
        }
        cc -= (avail_in_before - sp->stream.avail_in);
    } while (cc > 0);
    return (1);
}
