// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 23/36



            // color space conversion
            if (colorXform) {
                // convert YCbCr to RGB
                if (numComps == 3) {
                    for (y2 = 0; y2 < mcuHeight; ++y2) {
                        p0 = &frameBuf[0][(y1 + y2) * bufWidth + x1];
                        p1 = &frameBuf[1][(y1 + y2) * bufWidth + x1];
                        p2 = &frameBuf[2][(y1 + y2) * bufWidth + x1];
                        for (x2 = 0; x2 < mcuWidth; ++x2) {
                            pY = *p0;
                            pCb = *p1 - 128;
                            pCr = *p2 - 128;
                            pR = ((pY << 16) + dctCrToR * pCr + 32768) >> 16;
                            *p0++ = dctClip[dctClipOffset + pR];
                            pG = ((pY << 16) + dctCbToG * pCb + dctCrToG * pCr + 32768) >> 16;
                            *p1++ = dctClip[dctClipOffset + pG];
                            pB = ((pY << 16) + dctCbToB * pCb + 32768) >> 16;
                            *p2++ = dctClip[dctClipOffset + pB];
                        }
                    }
                    // convert YCbCrK to CMYK (K is passed through unchanged)
                } else if (numComps == 4) {
                    for (y2 = 0; y2 < mcuHeight; ++y2) {
                        p0 = &frameBuf[0][(y1 + y2) * bufWidth + x1];
                        p1 = &frameBuf[1][(y1 + y2) * bufWidth + x1];
                        p2 = &frameBuf[2][(y1 + y2) * bufWidth + x1];
                        for (x2 = 0; x2 < mcuWidth; ++x2) {
                            pY = *p0;
                            pCb = *p1 - 128;
                            pCr = *p2 - 128;
                            pR = ((pY << 16) + dctCrToR * pCr + 32768) >> 16;
                            *p0++ = 255 - dctClip[dctClipOffset + pR];
                            pG = ((pY << 16) + dctCbToG * pCb + dctCrToG * pCr + 32768) >> 16;
                            *p1++ = 255 - dctClip[dctClipOffset + pG];
                            pB = ((pY << 16) + dctCbToB * pCb + 32768) >> 16;
                            *p2++ = 255 - dctClip[dctClipOffset + pB];
                        }
                    }
                }
            }
        }
    }
}

// Transform one data unit -- this performs the dequantization and
// IDCT steps.  This IDCT algorithm is taken from:
//   Christoph Loeffler, Adriaan Ligtenberg, George S. Moschytz,
//   "Practical Fast 1-D DCT Algorithms with 11 Multiplications",
//   IEEE Intl. Conf. on Acoustics, Speech & Signal Processing, 1989,
//   988-991.
// The stage numbers mentioned in the comments refer to Figure 1 in this
// paper.
void DCTStream::transformDataUnit(unsigned short *quantTable, int dataIn[64], unsigned char dataOut[64])
{
    int v0, v1, v2, v3, v4, v5, v6, v7, t;
    int *p;
    int i;

    // dequant
    for (i = 0; i < 64; ++i) {
        dataIn[i] *= quantTable[i];
    }

    // inverse DCT on rows
    for (i = 0; i < 64; i += 8) {
        p = dataIn + i;

        // check for all-zero AC coefficients
        if (p[1] == 0 && p[2] == 0 && p[3] == 0 && p[4] == 0 && p[5] == 0 && p[6] == 0 && p[7] == 0) {
            t = (dctSqrt2 * p[0] + 512) >> 10;
            p[0] = t;
            p[1] = t;
            p[2] = t;
            p[3] = t;
            p[4] = t;
            p[5] = t;
            p[6] = t;
            p[7] = t;
            continue;
        }

        // stage 4
        v0 = (dctSqrt2 * p[0] + 128) >> 8;
        v1 = (dctSqrt2 * p[4] + 128) >> 8;
        v2 = p[2];
        v3 = p[6];
        v4 = (dctSqrt1d2 * (p[1] - p[7]) + 128) >> 8;
        v7 = (dctSqrt1d2 * (p[1] + p[7]) + 128) >> 8;
        v5 = p[3] << 4;
        v6 = p[5] << 4;

        // stage 3
        t = (v0 - v1 + 1) >> 1;
        v0 = (v0 + v1 + 1) >> 1;
        v1 = t;
        t = (v2 * dctSin6 + v3 * dctCos6 + 128) >> 8;
        v2 = (v2 * dctCos6 - v3 * dctSin6 + 128) >> 8;
        v3 = t;
        t = (v4 - v6 + 1) >> 1;
        v4 = (v4 + v6 + 1) >> 1;
        v6 = t;
        t = (v7 + v5 + 1) >> 1;
        v5 = (v7 - v5 + 1) >> 1;
        v7 = t;

        // stage 2
        t = (v0 - v3 + 1) >> 1;
        v0 = (v0 + v3 + 1) >> 1;
        v3 = t;
        t = (v1 - v2 + 1) >> 1;
        v1 = (v1 + v2 + 1) >> 1;
        v2 = t;
        t = (v4 * dctSin3 + v7 * dctCos3 + 2048) >> 12;
        v4 = (v4 * dctCos3 - v7 * dctSin3 + 2048) >> 12;
        v7 = t;
        t = (v5 * dctSin1 + v6 * dctCos1 + 2048) >> 12;
        v5 = (v5 * dctCos1 - v6 * dctSin1 + 2048) >> 12;
        v6 = t;

        // stage 1
        p[0] = v0 + v7;
        p[7] = v0 - v7;
        p[1] = v1 + v6;
        p[6] = v1 - v6;
        p[2] = v2 + v5;
        p[5] = v2 - v5;
        p[3] = v3 + v4;
        p[4] = v3 - v4;
    }

    // inverse DCT on columns
    for (i = 0; i < 8; ++i) {
        p = dataIn + i;