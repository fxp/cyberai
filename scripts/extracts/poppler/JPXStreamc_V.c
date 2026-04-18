// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 22/31



                                // one codeword segment for all of the coding passes
                            } else {

                                // read the length
                                for (n = cb->lBlock, i = cb->nCodingPasses >> 1; i; ++n, i >>= 1)
                                    ;
                                if (!readBits(n, &cb->dataLen[0])) {
                                    goto err;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (tileComp->style & 0x04) {
            skipEPH();
        }
        tilePartLen = finishBitBuf();

        //----- packet data

        for (sb = 0; sb < (unsigned int)(tile->res == 0 ? 1 : 3); ++sb) {
            subband = &precinct->subbands[sb];
            for (cbY = 0; cbY < subband->nYCBs; ++cbY) {
                for (cbX = 0; cbX < subband->nXCBs; ++cbX) {
                    cb = &subband->cbs[cbY * subband->nXCBs + cbX];
                    if (cb->included) {
                        if (!readCodeBlockData(tileComp, tile->res, sb, cb)) {
                            return false;
                        }
                        if (tileComp->codeBlockStyle & 0x04) {
                            for (i = 0; i < cb->nCodingPasses; ++i) {
                                tilePartLen -= cb->dataLen[i];
                            }
                        } else {
                            tilePartLen -= cb->dataLen[0];
                        }
                        cb->seen = true;
                    }
                }
            }
        }

        //----- next packet

        switch (tile->progOrder) {
        case 0: // layer, resolution level, component, precinct
            cover(58);
            if (++tile->comp == img.nComps) {
                tile->comp = 0;
                if (++tile->res == tile->maxNDecompLevels + 1) {
                    tile->res = 0;
                    if (++tile->layer == tile->nLayers) {
                        tile->layer = 0;
                    }
                }
            }
            break;
        case 1: // resolution level, layer, component, precinct
            cover(59);
            if (++tile->comp == img.nComps) {
                tile->comp = 0;
                if (++tile->layer == tile->nLayers) {
                    tile->layer = 0;
                    if (++tile->res == tile->maxNDecompLevels + 1) {
                        tile->res = 0;
                    }
                }
            }
            break;
        case 2: // resolution level, precinct, component, layer
            //~ this isn't correct -- see B.12.1.3
            cover(60);
            if (++tile->layer == tile->nLayers) {
                tile->layer = 0;
                if (++tile->comp == img.nComps) {
                    tile->comp = 0;
                    if (++tile->res == tile->maxNDecompLevels + 1) {
                        tile->res = 0;
                    }
                }
                tileComp = &tile->tileComps[tile->comp];
                if (tile->res >= tileComp->nDecompLevels + 1) {
                    if (++tile->comp == img.nComps) {
                        return true;
                    }
                }
            }
            break;
        case 3: // precinct, component, resolution level, layer
            //~ this isn't correct -- see B.12.1.4
            cover(61);
            if (++tile->layer == tile->nLayers) {
                tile->layer = 0;
                if (++tile->res == tile->maxNDecompLevels + 1) {
                    tile->res = 0;
                    if (++tile->comp == img.nComps) {
                        tile->comp = 0;
                    }
                }
            }
            break;
        case 4: // component, precinct, resolution level, layer
            //~ this isn't correct -- see B.12.1.5
            cover(62);
            if (++tile->layer == tile->nLayers) {
                tile->layer = 0;
                if (++tile->res == tile->maxNDecompLevels + 1) {
                    tile->res = 0;
                    if (++tile->comp == img.nComps) {
                        tile->comp = 0;
                    }
                }
            }
            break;
        }
    }

    return true;

err:
    error(errSyntaxError, getPos(), "Error in JPX stream");
    return false;
}

bool JPXStream::readCodeBlockData(JPXTileComp *tileComp, unsigned int res, unsigned int sb, JPXCodeBlock *cb)
{
    int *coeff0, *coeff1, *coeff;
    char *touched0, *touched1, *touched;
    unsigned int horiz, vert, diag, all, cx, xorBit;
    int horizSign, vertSign, bit;
    int segSym;
    unsigned int i, x, y0, y1;