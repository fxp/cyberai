// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 27/38



            // decode a line
            codingLine[0] = 0;
            int a0i = 0;
            int b1i = 0;
            int blackPixels = 0;
            // invariant:
            // refLine[b1i-1] <= codingLine[a0i] < refLine[b1i] < refLine[b1i+1] <= w
            // exception at left edge:
            //   codingLine[a0i = 0] = refLine[b1i = 0] = 0 is possible
            // exception at right edge:
            //   refLine[b1i] = refLine[b1i+1] = w is possible
            while (codingLine[a0i] < w) {
                switch (mmrDecoder->get2DCode()) {
                case twoDimPass:
                    if (unlikely(b1i + 1 >= w + 2)) {
                        break;
                    }
                    mmrAddPixels(refLine[b1i + 1], blackPixels, codingLine, &a0i, w);
                    if (refLine[b1i + 1] < w) {
                        b1i += 2;
                    }
                    break;
                case twoDimHoriz: {
                    int code1 = 0;
                    int code2 = 0;
                    if (blackPixels) {
                        int code3;
                        do {
                            code1 += code3 = mmrDecoder->getBlackCode();
                        } while (code3 >= 64);
                        do {
                            code2 += code3 = mmrDecoder->getWhiteCode();
                        } while (code3 >= 64);
                    } else {
                        int code3;
                        do {
                            code1 += code3 = mmrDecoder->getWhiteCode();
                        } while (code3 >= 64);
                        do {
                            code2 += code3 = mmrDecoder->getBlackCode();
                        } while (code3 >= 64);
                    }
                    mmrAddPixels(codingLine[a0i] + code1, blackPixels, codingLine, &a0i, w);
                    if (codingLine[a0i] < w) {
                        mmrAddPixels(codingLine[a0i] + code2, blackPixels ^ 1, codingLine, &a0i, w);
                    }
                    while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                        b1i += 2;
                    }
                    break;
                }
                case twoDimVertR3:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixels(refLine[b1i] + 3, blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        ++b1i;
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimVertR2:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixels(refLine[b1i] + 2, blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        ++b1i;
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimVertR1:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixels(refLine[b1i] + 1, blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        ++b1i;
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimVert0:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixels(refLine[b1i], blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        ++b1i;
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimVertL3:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixelsNeg(refLine[b1i] - 3, blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        if (b1i > 0) {
                            --b1i;
                        } else {
                            ++b1i;
                        }
                        w