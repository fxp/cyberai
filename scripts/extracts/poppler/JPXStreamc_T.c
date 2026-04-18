// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 20/31



        tileComp = &tile->tileComps[tile->comp];
        resLevel = &tileComp->resLevels[tile->res];
        precinct = &resLevel->precincts[tile->precinct];

        //----- packet header

        // setup
        startBitBuf(tilePartLen);
        if (tileComp->style & 0x02) {
            skipSOP();
        }

        // zero-length flag
        if (!readBits(1, &bits)) {
            goto err;
        }
        if (!bits) {
            // packet is empty -- clear all code-block inclusion flags
            cover(45);
            for (sb = 0; sb < (unsigned int)(tile->res == 0 ? 1 : 3); ++sb) {
                subband = &precinct->subbands[sb];
                for (cbY = 0; cbY < subband->nYCBs; ++cbY) {
                    for (cbX = 0; cbX < subband->nXCBs; ++cbX) {
                        cb = &subband->cbs[cbY * subband->nXCBs + cbX];
                        cb->included = false;
                    }
                }
            }
        } else {

            for (sb = 0; sb < (unsigned int)(tile->res == 0 ? 1 : 3); ++sb) {
                subband = &precinct->subbands[sb];
                for (cbY = 0; cbY < subband->nYCBs; ++cbY) {
                    for (cbX = 0; cbX < subband->nXCBs; ++cbX) {
                        cb = &subband->cbs[cbY * subband->nXCBs + cbX];

                        // skip code-blocks with no coefficients
                        if (cb->x0 >= cb->x1 || cb->y0 >= cb->y1) {
                            cover(46);
                            cb->included = false;
                            continue;
                        }

                        // code-block inclusion
                        if (cb->seen) {
                            cover(47);
                            if (!readBits(1, &cb->included)) {
                                goto err;
                            }
                        } else {
                            cover(48);
                            ttVal = 0;
                            i = 0;
                            for (level = subband->maxTTLevel; level >= 0; --level) {
                                nx = jpxCeilDivPow2(subband->nXCBs, level);
                                ny = jpxCeilDivPow2(subband->nYCBs, level);
                                j = i + (cbY >> level) * nx + (cbX >> level);
                                if (!subband->inclusion[j].finished && !subband->inclusion[j].val) {
                                    subband->inclusion[j].val = ttVal;
                                } else {
                                    ttVal = subband->inclusion[j].val;
                                }
                                while (!subband->inclusion[j].finished && ttVal <= tile->layer) {
                                    if (!readBits(1, &bits)) {
                                        goto err;
                                    }
                                    if (bits == 1) {
                                        subband->inclusion[j].finished = true;
                                    } else {
                                        ++ttVal;
                                    }
                                }
                                subband->inclusion[j].val = ttVal;
                                if (ttVal > tile->layer) {
                                    break;
                                }
                                i += nx * ny;
                            }
                            cb->included = level < 0;
                        }

                        if (cb->included) {
                            cover(49);