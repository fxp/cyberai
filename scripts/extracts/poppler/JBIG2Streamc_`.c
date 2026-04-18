// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 32/38



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
                            const unsigned int cx0 = (buf0 >> 14) & 0x07;
                            const unsigned int cx1 = (buf1 >> 14) & 0x0f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x03;
                            const unsigned int cx = (cx0 << 7) | (cx1 << 3) | (cx2 << 1) | ((atBuf0 >> atShift0) & 1);

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
                            const unsigned int cx0 = (buf0 >> 14) & 0x07;
                            const unsigned int cx1 = (buf1 >> 14) & 0x0f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x03;
                            const unsigned int cx = (cx0 << 7) | (cx1 << 3) | (cx2 << 1) | bitmap->getPixel(x + atx[0], y + aty[0]);

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
            case 3:
                if (isPixelOutsideAdaptiveField(atx[0], aty[0])) {
                    return nullptr;
                }