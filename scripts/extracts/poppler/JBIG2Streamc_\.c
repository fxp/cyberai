// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 28/38

hile (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimVertL2:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixelsNeg(refLine[b1i] - 2, blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        if (b1i > 0) {
                            --b1i;
                        } else {
                            ++b1i;
                        }
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimVertL1:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixelsNeg(refLine[b1i] - 1, blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        if (b1i > 0) {
                            --b1i;
                        } else {
                            ++b1i;
                        }
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case EOF:
                    mmrAddPixels(w, 0, codingLine, &a0i, w);
                    break;
                default:
                    error(errSyntaxError, curStr->getPos(), "Illegal code in JBIG2 MMR bitmap data");
                    mmrAddPixels(w, 0, codingLine, &a0i, w);
                    break;
                }
            }

            // convert the run lengths to a bitmap line
            {
                int i = 0;
                while (true) {
                    for (int x = codingLine[i]; x < codingLine[i + 1]; ++x) {
                        bitmap->setPixel(x, y);
                    }
                    if (codingLine[i + 1] >= w || codingLine[i + 2] >= w) {
                        break;
                    }
                    i += 2;
                }
            }
        }

        if (mmrDataLength >= 0) {
            if (!mmrDecoder->skipTo(mmrDataLength)) {
                error(errSyntaxError, curStr->getPos(), "Error in mmrDecoder skikpping");
                gfree(refLine);
                gfree(codingLine);
                return {};
            }
        } else {
            if (mmrDecoder->get24Bits() != 0x001001) {
                error(errSyntaxError, curStr->getPos(), "Missing EOFB in JBIG2 MMR bitmap data");
            }
        }

        gfree(refLine);
        gfree(codingLine);

        //----- arithmetic decode

    } else {
        // set up the typical row context
        const unsigned int ltpCX = [tpgdOn, templ] {
            if (tpgdOn) {
                switch (templ) {
                case 0:
                    return 0x3953; // 001 11001 0101 0011
                    break;
                case 1:
                    return 0x079a; // 0011 11001 101 0
                    break;
                case 2:
                    return 0x0e3; // 001 1100 01 1
                    break;
                case 3:
                    return 0x18b; // 01100 0101 1
                    break;
                }
            }
            return 0;
        }();

        bool ltp = false;
        for (int y = 0; y < h; ++y) {

            // check for a "typical" (duplicate) row
            if (tpgdOn) {
                if (arithDecoder->decodeBit(ltpCX, genericRegionStats.get())) {
                    ltp = !ltp;
                }
                if (ltp) {
                    if (y > 0) {
                        bitmap->duplicateRow(y, y - 1);
                    }
                    continue;
                }
            }

            switch (templ) {
            case 0: {
                if (isPixelOutsideAdaptiveField(atx[0], aty[0]) || isPixelOutsideAdaptiveField(atx[1], aty[1]) || isPixelOutsideAdaptiveField(atx[2], aty[2]) || isPixelOutsideAdaptiveField(atx[3], aty[3])) {
                    return nullptr;
                }