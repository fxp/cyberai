/* ===== EXTRACT: tif_pixarlog.c ===== */
/* Function: PixarLogDecode (part B) */
/* Lines: 952–992 */

        switch (sp->user_datafmt)
        {
            case PIXARLOGDATAFMT_FLOAT:
                horizontalAccumulateF(up, llen, sp->stride, (float *)op,
                                      sp->ToLinearF);
                op += llen * sizeof(float);
                break;
            case PIXARLOGDATAFMT_16BIT:
                horizontalAccumulate16(up, llen, sp->stride, (uint16_t *)op,
                                       sp->ToLinear16);
                op += llen * sizeof(uint16_t);
                break;
            case PIXARLOGDATAFMT_12BITPICIO:
                horizontalAccumulate12(up, llen, sp->stride, (int16_t *)op,
                                       sp->ToLinearF);
                op += llen * sizeof(int16_t);
                break;
            case PIXARLOGDATAFMT_11BITLOG:
                horizontalAccumulate11(up, llen, sp->stride, (uint16_t *)op);
                op += llen * sizeof(uint16_t);
                break;
            case PIXARLOGDATAFMT_8BIT:
                horizontalAccumulate8(up, llen, sp->stride, (unsigned char *)op,
                                      sp->ToLinear8);
                op += llen * sizeof(unsigned char);
                break;
            case PIXARLOGDATAFMT_8BITABGR:
                horizontalAccumulate8abgr(up, llen, sp->stride,
                                          (unsigned char *)op, sp->ToLinear8);
                op += llen * sizeof(unsigned char);
                break;
            default:
                TIFFErrorExtR(tif, module, "Unsupported bits/sample: %" PRIu16,
                              td->td_bitspersample);
                memset(op, 0, (size_t)occ);
                return (0);
        }
    }

    return (1);
}
