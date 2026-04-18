// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 30/38



                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats.get())) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
                                    if (aty[0] == 0) {
                                        atBuf0 |= 0x8000;
                                    }
                                    if (aty[1] == 0) {
                                        atBuf1 |= 0x8000;
                                    }
                                    if (aty[2] == 0) {
                                        atBuf2 |= 0x8000;
                                    }
                                    if (aty[3] == 0) {
                                        atBuf3 |= 0x8000;
                                    }
                                }
                            }

                            // update the context
                            buf0 <<= 1;
                            buf1 <<= 1;
                            buf2 <<= 1;
                            atBuf0 <<= 1;
                            atBuf1 <<= 1;
                            atBuf2 <<= 1;
                            atBuf3 <<= 1;
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
                            const unsigned int cx0 = (buf0 >> 14) & 0x07;
                            const unsigned int cx1 = (buf1 >> 13) & 0x1f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x0f;
                            const unsigned int cx = (cx0 << 13) | (cx1 << 8) | (cx2 << 4) | (bitmap->getPixel(x + atx[0], y + aty[0]) << 3) | (bitmap->getPixel(x + atx[1], y + aty[1]) << 2) | (bitmap->getPixel(x + atx[2], y + aty[2]) << 1)
                                    | bitmap->getPixel(x + atx[3], y + aty[3]);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {

                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats.get())) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
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
            case 1: {
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

                if (atx[0] >= -8 && atx[0] <= 8) {
                    // set up the adaptive context
                    const int atY = y + aty[0];
                    unsigned int atBuf0;
                    unsigned char *atP0;
                    if ((atY >= 0) && (atY < bitmap->getHeight())) {
                        atP0 = bitmap->getDataPtr() + atY * bitmap->getLineSize();
                        atBuf0 = *atP0++ << 8;
                    } else {
                        atP0 = nullptr;
                        atBuf0 = 0;
                    }
                    const int atShift0 = 15 - atx[0];