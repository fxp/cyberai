// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 22/36



                        // store back into frameBuf, doing replication for
                        // subsampled components
                        p1 = &frameBuf[cc][(y1 + y2) * bufWidth + (x1 + x2)];
                        if (hSub == 1 && vSub == 1) {
                            for (y3 = 0, i = 0; y3 < 8; ++y3, i += 8) {
                                p1[0] = dataOut[i] & 0xff;
                                p1[1] = dataOut[i + 1] & 0xff;
                                p1[2] = dataOut[i + 2] & 0xff;
                                p1[3] = dataOut[i + 3] & 0xff;
                                p1[4] = dataOut[i + 4] & 0xff;
                                p1[5] = dataOut[i + 5] & 0xff;
                                p1[6] = dataOut[i + 6] & 0xff;
                                p1[7] = dataOut[i + 7] & 0xff;
                                p1 += bufWidth;
                            }
                        } else if (hSub == 2 && vSub == 2) {
                            p2 = p1 + bufWidth;
                            for (y3 = 0, i = 0; y3 < 16; y3 += 2, i += 8) {
                                p1[0] = p1[1] = p2[0] = p2[1] = dataOut[i] & 0xff;
                                p1[2] = p1[3] = p2[2] = p2[3] = dataOut[i + 1] & 0xff;
                                p1[4] = p1[5] = p2[4] = p2[5] = dataOut[i + 2] & 0xff;
                                p1[6] = p1[7] = p2[6] = p2[7] = dataOut[i + 3] & 0xff;
                                p1[8] = p1[9] = p2[8] = p2[9] = dataOut[i + 4] & 0xff;
                                p1[10] = p1[11] = p2[10] = p2[11] = dataOut[i + 5] & 0xff;
                                p1[12] = p1[13] = p2[12] = p2[13] = dataOut[i + 6] & 0xff;
                                p1[14] = p1[15] = p2[14] = p2[15] = dataOut[i + 7] & 0xff;
                                p1 += bufWidth * 2;
                                p2 += bufWidth * 2;
                            }
                        } else {
                            i = 0;
                            for (y3 = 0, y4 = 0; y3 < 8; ++y3, y4 += vSub) {
                                for (x3 = 0, x4 = 0; x3 < 8; ++x3, x4 += hSub) {
                                    p2 = p1 + x4;
                                    for (y5 = 0; y5 < vSub; ++y5) {
                                        for (x5 = 0; x5 < hSub; ++x5) {
                                            p2[x5] = dataOut[i] & 0xff;
                                        }
                                        p2 += bufWidth;
                                    }
                                    ++i;
                                }
                                p1 += bufWidth * vSub;
                            }
                        }
                    }
                }
            }