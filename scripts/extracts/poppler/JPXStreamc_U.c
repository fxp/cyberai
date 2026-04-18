// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 21/31



                            // zero bit-plane count
                            if (!cb->seen) {
                                cover(50);
                                ttVal = 0;
                                i = 0;
                                for (level = subband->maxTTLevel; level >= 0; --level) {
                                    nx = jpxCeilDivPow2(subband->nXCBs, level);
                                    ny = jpxCeilDivPow2(subband->nYCBs, level);
                                    j = i + (cbY >> level) * nx + (cbX >> level);
                                    if (!subband->zeroBitPlane[j].finished && !subband->zeroBitPlane[j].val) {
                                        subband->zeroBitPlane[j].val = ttVal;
                                    } else {
                                        ttVal = subband->zeroBitPlane[j].val;
                                    }
                                    while (!subband->zeroBitPlane[j].finished) {
                                        if (!readBits(1, &bits)) {
                                            goto err;
                                        }
                                        if (bits == 1) {
                                            subband->zeroBitPlane[j].finished = true;
                                        } else {
                                            ++ttVal;
                                        }
                                    }
                                    subband->zeroBitPlane[j].val = ttVal;
                                    i += nx * ny;
                                }
                                cb->nZeroBitPlanes = ttVal;
                            }

                            // number of coding passes
                            if (!readBits(1, &bits)) {
                                goto err;
                            }
                            if (bits == 0) {
                                cover(51);
                                cb->nCodingPasses = 1;
                            } else {
                                if (!readBits(1, &bits)) {
                                    goto err;
                                }
                                if (bits == 0) {
                                    cover(52);
                                    cb->nCodingPasses = 2;
                                } else {
                                    cover(53);
                                    if (!readBits(2, &bits)) {
                                        goto err;
                                    }
                                    if (bits < 3) {
                                        cover(54);
                                        cb->nCodingPasses = 3 + bits;
                                    } else {
                                        cover(55);
                                        if (!readBits(5, &bits)) {
                                            goto err;
                                        }
                                        if (bits < 31) {
                                            cover(56);
                                            cb->nCodingPasses = 6 + bits;
                                        } else {
                                            cover(57);
                                            if (!readBits(7, &bits)) {
                                                goto err;
                                            }
                                            cb->nCodingPasses = 37 + bits;
                                        }
                                    }
                                }
                            }

                            // update Lblock
                            while (1) {
                                if (!readBits(1, &bits)) {
                                    goto err;
                                }
                                if (!bits) {
                                    break;
                                }
                                ++cb->lBlock;
                            }

                            // one codeword segment for each of the coding passes
                            if (tileComp->codeBlockStyle & 0x04) {
                                if (cb->nCodingPasses > cb->dataLenSize) {
                                    cb->dataLenSize = cb->nCodingPasses;
                                    cb->dataLen = (unsigned int *)greallocn(cb->dataLen, cb->dataLenSize, sizeof(unsigned int));
                                }

                                // read the lengths
                                for (i = 0; i < cb->nCodingPasses; ++i) {
                                    if (!readBits(cb->lBlock, &cb->dataLen[i])) {
                                        goto err;
                                    }
                                }