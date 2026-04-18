// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 33/38



                // set up the context
                unsigned char *p1, *p2, *pp;
                p2 = pp = bitmap->getDataPtr() + y * bitmap->getLineSize();
                unsigned int buf1;
                unsigned int buf2 = *p2++ << 8;
                if (y >= 1) {
                    p1 = bitmap->getDataPtr() + (y - 1) * bitmap->getLineSize();
                    buf1 = *p1++ << 8;
                } else {
                    p1 = nullptr;
                    buf1 = 0;
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

                    // decode the row
                    for (int x0 = 0, x = 0; x0 < w; x0 += 8, ++pp) {
                        if (x0 + 8 < w) {
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
                            const unsigned int cx1 = (buf1 >> 14) & 0x1f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x0f;
                            const unsigned int cx = (cx1 << 5) | (cx2 << 1) | ((atBuf0 >> atShift0) & 1);

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
                            buf1 <<= 1;
                            buf2 <<= 1;
                            atBuf0 <<= 1;
                        }
                    }

                } else {
                    // decode the row
                    for (int x0 = 0, x = 0; x0 < w; x0 += 8, ++pp) {
                        if (x0 + 8 < w) {
                            if (p1) {
                                buf1 |= *p1++;
                            }
                            buf2 |= *p2++;
                        }
                        unsigned char mask = 0x80;
                        for (int x1 = 0; x1 < 8 && x < w; ++x1, ++x, mask >>= 1) {

                            // build the context
                            const unsigned int cx1 = (buf1 >> 14) & 0x1f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x0f;
                            const unsigned int cx = (cx1 << 5) | (cx2 << 1) | bitmap->getPixel(x + atx[0], y + aty[0]);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {

                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats.get())) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
                                }
                            }

                            // update the context
                            buf1 <<= 1;
                            buf2 <<= 1;
                        }
                    }
                }
                break;
            }
        }
    }

    return bitmap;
}

bool JBIG2Stream::readGenericRefinementRegionSeg(unsigned int segNum, bool imm, const std::vector<unsigned int> &refSegs)
{
    unsigned int w, h, x, y, segInfoFlags;
    unsigned int flags;
    int atx[2], aty[2];