// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 29/31



        if (tileComp->transform == 0) {
            cover(84);
            // step 1 (even)
            for (i = 1; i <= end + 2; i += 2) {
                data[i] = (int)(idwtKappa * data[i]);
            }
            // step 2 (odd)
            for (i = 0; i <= end + 3; i += 2) {
                data[i] = (int)(idwtIKappa * data[i]);
            }
            // step 3 (even)
            for (i = 1; i <= end + 2; i += 2) {
                data[i] = (int)(data[i] - idwtDelta * (data[i - 1] + data[i + 1]));
            }
            // step 4 (odd)
            for (i = 2; i <= end + 1; i += 2) {
                data[i] = (int)(data[i] - idwtGamma * (data[i - 1] + data[i + 1]));
            }
            // step 5 (even)
            for (i = 3; i <= end; i += 2) {
                data[i] = (int)(data[i] - idwtBeta * (data[i - 1] + data[i + 1]));
            }
            // step 6 (odd)
            for (i = 4; i <= end - 1; i += 2) {
                data[i] = (int)(data[i] - idwtAlpha * (data[i - 1] + data[i + 1]));
            }

            //----- 5-3 reversible filter

        } else {
            cover(85);
            // step 1 (even)
            for (i = 3; i <= end; i += 2) {
                data[i] -= (data[i - 1] + data[i + 1] + 2) >> 2;
            }
            // step 2 (odd)
            for (i = 4; i < end; i += 2) {
                data[i] += (data[i - 1] + data[i + 1]) >> 1;
            }
        }
    }
}

// Inverse multi-component transform and DC level shift.  This also
// converts fixed point samples back to integers.
bool JPXStream::inverseMultiCompAndDC(JPXTile *tile)
{
    JPXTileComp *tileComp;
    int coeff, d0, d1, d2, t, minVal, maxVal, zeroVal;
    int *dataPtr;
    unsigned int j, comp, x, y;

    //----- inverse multi-component transform

    if (tile->multiComp == 1) {
        cover(86);
        if (img.nComps < 3 || tile->tileComps[0].hSep != tile->tileComps[1].hSep || tile->tileComps[0].vSep != tile->tileComps[1].vSep || tile->tileComps[1].hSep != tile->tileComps[2].hSep
            || tile->tileComps[1].vSep != tile->tileComps[2].vSep) {
            return false;
        }

        // inverse irreversible multiple component transform
        if (tile->tileComps[0].transform == 0) {
            cover(87);
            j = 0;
            for (y = 0; y < tile->tileComps[0].y1 - tile->tileComps[0].y0; ++y) {
                for (x = 0; x < tile->tileComps[0].x1 - tile->tileComps[0].x0; ++x) {
                    d0 = tile->tileComps[0].data[j];
                    d1 = tile->tileComps[1].data[j];
                    d2 = tile->tileComps[2].data[j];
                    tile->tileComps[0].data[j] = (int)(d0 + 1.402 * d2 + 0.5);
                    tile->tileComps[1].data[j] = (int)(d0 - 0.34413 * d1 - 0.71414 * d2 + 0.5);
                    tile->tileComps[2].data[j] = (int)(d0 + 1.772 * d1 + 0.5);
                    ++j;
                }
            }

            // inverse reversible multiple component transform
        } else {
            cover(88);
            j = 0;
            for (y = 0; y < tile->tileComps[0].y1 - tile->tileComps[0].y0; ++y) {
                for (x = 0; x < tile->tileComps[0].x1 - tile->tileComps[0].x0; ++x) {
                    d0 = tile->tileComps[0].data[j];
                    d1 = tile->tileComps[1].data[j];
                    d2 = tile->tileComps[2].data[j];
                    tile->tileComps[1].data[j] = t = d0 - ((d2 + d1) >> 2);
                    tile->tileComps[0].data[j] = d2 + t;
                    tile->tileComps[2].data[j] = d1 + t;
                    ++j;
                }
            }
        }
    }

    //----- DC level shift
    for (comp = 0; comp < img.nComps; ++comp) {
        tileComp = &tile->tileComps[comp];

        // signed: clip
        if (tileComp->sgned) {
            cover(89);
            minVal = -(1 << (tileComp->prec - 1));
            maxVal = (1 << (tileComp->prec - 1)) - 1;
            dataPtr = tileComp->data;
            for (y = 0; y < tileComp->y1 - tileComp->y0; ++y) {
                for (x = 0; x < tileComp->x1 - tileComp->x0; ++x) {
                    coeff = *dataPtr;
                    if (tileComp->transform == 0) {
                        cover(109);
                        coeff >>= fracBits;
                    }
                    if (coeff < minVal) {
                        cover(110);
                        coeff = minVal;
                    } else if (coeff > maxVal) {
                        cover(111);
                        coeff = maxVal;
                    }
                    *dataPtr++ = coeff;
                }
            }