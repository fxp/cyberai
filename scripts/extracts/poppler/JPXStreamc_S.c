// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 19/31

                    for (level = subband->maxTTLevel; level >= 0; --level) {
                            nx = jpxCeilDivPow2(subband->nXCBs, level);
                            ny = jpxCeilDivPow2(subband->nYCBs, level);
                            n += nx * ny;
                        }
                        subband->inclusion = (JPXTagTreeNode *)gmallocn(n, sizeof(JPXTagTreeNode));
                        subband->zeroBitPlane = (JPXTagTreeNode *)gmallocn(n, sizeof(JPXTagTreeNode));
                        for (k = 0; k < n; ++k) {
                            subband->inclusion[k].finished = false;
                            subband->inclusion[k].val = 0;
                            subband->zeroBitPlane[k].finished = false;
                            subband->zeroBitPlane[k].val = 0;
                        }
                        subband->cbs = (JPXCodeBlock *)gmallocn(subband->nXCBs * subband->nYCBs, sizeof(JPXCodeBlock));
                        sbx0 = jpxFloorDivPow2(subband->x0, tileComp->codeBlockW);
                        sby0 = jpxFloorDivPow2(subband->y0, tileComp->codeBlockH);
                        if (r == 0) { // (NL)LL
                            sbCoeffs = tileComp->data;
                        } else if (sb == 0) { // (NL-r+1)HL
                            sbCoeffs = tileComp->data + resLevel->bx1[1] - resLevel->bx0[1];
                        } else if (sb == 1) { // (NL-r+1)LH
                            sbCoeffs = tileComp->data + (resLevel->by1[0] - resLevel->by0[0]) * tileComp->w;
                        } else { // (NL-r+1)HH
                            sbCoeffs = tileComp->data + (resLevel->by1[0] - resLevel->by0[0]) * tileComp->w + resLevel->bx1[1] - resLevel->bx0[1];
                        }
                        cb = subband->cbs;
                        for (cbY = 0; cbY < subband->nYCBs; ++cbY) {
                            for (cbX = 0; cbX < subband->nXCBs; ++cbX) {
                                cb->x0 = (sbx0 + cbX) << tileComp->codeBlockW;
                                cb->x1 = cb->x0 + tileComp->cbW;
                                if (subband->x0 > cb->x0) {
                                    cb->x0 = subband->x0;
                                }
                                if (subband->x1 < cb->x1) {
                                    cb->x1 = subband->x1;
                                }
                                cb->y0 = (sby0 + cbY) << tileComp->codeBlockH;
                                cb->y1 = cb->y0 + tileComp->cbH;
                                if (subband->y0 > cb->y0) {
                                    cb->y0 = subband->y0;
                                }
                                if (subband->y1 < cb->y1) {
                                    cb->y1 = subband->y1;
                                }
                                cb->seen = false;
                                cb->lBlock = 3;
                                cb->nextPass = jpxPassCleanup;
                                cb->nZeroBitPlanes = 0;
                                cb->dataLenSize = 1;
                                cb->dataLen = (unsigned int *)gmalloc(sizeof(unsigned int));
                                cb->coeffs = sbCoeffs + (cb->y0 - subband->y0) * tileComp->w + (cb->x0 - subband->x0);
                                cb->touched = (char *)gmalloc(1 << (tileComp->codeBlockW + tileComp->codeBlockH));
                                cb->len = 0;
                                for (cbj = 0; cbj < cb->y1 - cb->y0; ++cbj) {
                                    for (cbi = 0; cbi < cb->x1 - cb->x0; ++cbi) {
                                        cb->coeffs[cbj * tileComp->w + cbi] = 0;
                                    }
                                }
                                memset(cb->touched, 0, (1 << (tileComp->codeBlockW + tileComp->codeBlockH)));
                                cb->arithDecoder = nullptr;
                                cb->stats = nullptr;
                                ++cb;
                            }
                        }
                    }
                }
            }
        }
    }

    return readTilePartData(tileIdx, tilePartLen, tilePartToEOC);
}

bool JPXStream::readTilePartData(unsigned int tileIdx, unsigned int tilePartLen, bool tilePartToEOC)
{
    JPXTile *tile;
    JPXTileComp *tileComp;
    JPXResLevel *resLevel;
    JPXPrecinct *precinct;
    JPXSubband *subband;
    JPXCodeBlock *cb;
    unsigned int ttVal;
    unsigned int bits, cbX, cbY, nx, ny, i, j, n, sb;
    int level;

    tile = &img.tiles[tileIdx];

    // read all packets from this tile-part
    while (1) {
        if (tilePartToEOC) {
            //~ peek for an EOC marker
            cover(93);
        } else if (tilePartLen == 0) {
            break;
        }