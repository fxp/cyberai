// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 31/38



                    // decode the row
                    for (int x0 = 0, x = 0; x0 < w; x0 += 8, ++pp) {
                        if (x0 + 8 < w) {
                            if (p0) {
                                buf0 |= *p0++;
                            }
                            if (p1) {
                                buf1 |= *p1++;
                            }
                            buf2 |= *p2++;
                            if (atP0) {
                                atBuf0 |= *atP0++;
                            }
                        }
                        unsigned char mask = 0x80;
                        for (int x1 = 0; x1 < 8 && x < w; ++x1, ++x, mask >>= 1) {

                            // build the context
                            const unsigned int cx0 = (buf0 >> 13) & 0x0f;
                            const unsigned int cx1 = (buf1 >> 13) & 0x1f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x07;
                            const unsigned int cx = (cx0 << 9) | (cx1 << 4) | (cx2 << 1) | ((atBuf0 >> atShift0) & 1);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {

                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats.get())) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
                                    if (aty[0] == 0) {
                                        atBuf0 |= 0x8000;
                                    }
                                }
                                if (unlikely(arithDecoder->getReadPastEndOfStream())) {
                                    return nullptr;
                                }
                            }

                            // update the context
                            buf0 <<= 1;
                            buf1 <<= 1;
                            buf2 <<= 1;
                            atBuf0 <<= 1;
                        }
                    }

                } else {
                    // decode the row
                    for (int x0 = 0, x = 0; x0 < w; x0 += 8, ++pp) {
                        if (x0 + 8 < w) {
                            if (p0) {
                                buf0 |= *p0++;
                            }
                            if (p1) {
                                buf1 |= *p1++;
                            }
                            buf2 |= *p2++;
                        }
                        unsigned char mask = 0x80;
                        for (int x1 = 0; x1 < 8 && x < w; ++x1, ++x, mask >>= 1) {

                            // build the context
                            const unsigned int cx0 = (buf0 >> 13) & 0x0f;
                            const unsigned int cx1 = (buf1 >> 13) & 0x1f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x07;
                            const unsigned int cx = (cx0 << 9) | (cx1 << 4) | (cx2 << 1) | bitmap->getPixel(x + atx[0], y + aty[0]);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {

                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats.get())) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
                                }
                                if (unlikely(arithDecoder->getReadPastEndOfStream())) {
                                    return nullptr;
                                }
                            }

                            // update the context
                            buf0 <<= 1;
                            buf1 <<= 1;
                            buf2 <<= 1;
                        }
                    }
                }
                break;
            }
            case 2: {
                if (isPixelOutsideAdaptiveField(atx[0], aty[0])) {
                    return nullptr;
                }

                // set up the context
                unsigned char *p0, *p1, *p2, *pp;
                p2 = pp = bitmap->getDataPtr() + y * bitmap->getLineSize();
                unsigned int buf0, buf1;
                unsigned int buf2 = *p2++ << 8;
                if (y >= 1) {
                    p1 = bitmap->getDataPtr() + (y - 1) * bitmap->getLineSize();
                    buf1 = *p1++ << 8;
                    if (y >= 2) {
                        p0 = bitmap->getDataPtr() + (y - 2) * bitmap->getLineSize();
                        buf0 = *p0++ << 8;
                    } else {
                        p0 = nullptr;
                        buf0 = 0;
                    }
                } else {
                    p1 = p0 = nullptr;
                    buf1 = buf0 = 0;
                }