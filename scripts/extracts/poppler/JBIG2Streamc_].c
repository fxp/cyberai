// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 29/38



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

                if (atx[0] >= -8 && atx[0] <= 8 && atx[1] >= -8 && atx[1] <= 8 && atx[2] >= -8 && atx[2] <= 8 && atx[3] >= -8 && atx[3] <= 8) {
                    // set up the adaptive context
                    unsigned int atBuf0, atBuf1, atBuf2, atBuf3;
                    unsigned char *atP0, *atP1, *atP2, *atP3;
                    if (y + aty[0] >= 0 && y + aty[0] < bitmap->getHeight()) {
                        atP0 = bitmap->getDataPtr() + (y + aty[0]) * bitmap->getLineSize();
                        atBuf0 = *atP0++ << 8;
                    } else {
                        atP0 = nullptr;
                        atBuf0 = 0;
                    }
                    const int atShift0 = 15 - atx[0];
                    if (y + aty[1] >= 0 && y + aty[1] < bitmap->getHeight()) {
                        atP1 = bitmap->getDataPtr() + (y + aty[1]) * bitmap->getLineSize();
                        atBuf1 = *atP1++ << 8;
                    } else {
                        atP1 = nullptr;
                        atBuf1 = 0;
                    }
                    const int atShift1 = 15 - atx[1];
                    if (y + aty[2] >= 0 && y + aty[2] < bitmap->getHeight()) {
                        atP2 = bitmap->getDataPtr() + (y + aty[2]) * bitmap->getLineSize();
                        atBuf2 = *atP2++ << 8;
                    } else {
                        atP2 = nullptr;
                        atBuf2 = 0;
                    }
                    const int atShift2 = 15 - atx[2];
                    if (y + aty[3] >= 0 && y + aty[3] < bitmap->getHeight()) {
                        atP3 = bitmap->getDataPtr() + (y + aty[3]) * bitmap->getLineSize();
                        atBuf3 = *atP3++ << 8;
                    } else {
                        atP3 = nullptr;
                        atBuf3 = 0;
                    }
                    const int atShift3 = 15 - atx[3];

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
                            if (atP1) {
                                atBuf1 |= *atP1++;
                            }
                            if (atP2) {
                                atBuf2 |= *atP2++;
                            }
                            if (atP3) {
                                atBuf3 |= *atP3++;
                            }
                        }
                        unsigned char mask = 0x80;
                        for (int x1 = 0; x1 < 8 && x < w; ++x1, ++x, mask >>= 1) {

                            // build the context
                            const unsigned int cx0 = (buf0 >> 14) & 0x07;
                            const unsigned int cx1 = (buf1 >> 13) & 0x1f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x0f;
                            const unsigned int cx = (cx0 << 13) | (cx1 << 8) | (cx2 << 4) | (((atBuf0 >> atShift0) & 1) << 3) | (((atBuf1 >> atShift1) & 1) << 2) | (((atBuf2 >> atShift2) & 1) << 1) | ((atBuf3 >> atShift3) & 1);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {