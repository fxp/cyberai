// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 12/36



        // 2-D encoding
        if (nextLine2D) {
            for (i = 0; i < columns && codingLine[i] < columns; ++i) {
                refLine[i] = codingLine[i];
            }
            for (; i < columns + 2; ++i) {
                refLine[i] = columns;
            }
            codingLine[0] = 0;
            a0i = 0;
            b1i = 0;
            blackPixels = 0;
            // invariant:
            // refLine[b1i-1] <= codingLine[a0i] < refLine[b1i] < refLine[b1i+1]
            //                                                             <= columns
            // exception at left edge:
            //   codingLine[a0i = 0] = refLine[b1i = 0] = 0 is possible
            // exception at right edge:
            //   refLine[b1i] = refLine[b1i+1] = columns is possible
            while (codingLine[a0i] < columns && !err) {
                code1 = getTwoDimCode();
                switch (code1) {
                case twoDimPass:
                    if (likely(b1i + 1 < columns + 2)) {
                        addPixels(refLine[b1i + 1], blackPixels);
                        if (refLine[b1i + 1] < columns) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimHoriz:
                    code1 = code2 = 0;
                    if (blackPixels) {
                        do {
                            code1 += code3 = getBlackCode();
                        } while (code3 >= 64);
                        do {
                            code2 += code3 = getWhiteCode();
                        } while (code3 >= 64);
                    } else {
                        do {
                            code1 += code3 = getWhiteCode();
                        } while (code3 >= 64);
                        do {
                            code2 += code3 = getBlackCode();
                        } while (code3 >= 64);
                    }
                    addPixels(codingLine[a0i] + code1, blackPixels);
                    if (codingLine[a0i] < columns) {
                        addPixels(codingLine[a0i] + code2, blackPixels ^ 1);
                    }
                    while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
                        b1i += 2;
                        if (unlikely(b1i > columns + 1)) {
                            error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                            err = true;
                            break;
                        }
                    }
                    break;
                case twoDimVertR3:
                    if (unlikely(b1i > columns + 1)) {
                        error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                        err = true;
                        break;
                    }
                    addPixels(refLine[b1i] + 3, blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < columns) {
                        ++b1i;
                        while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
                            b1i += 2;
                            if (unlikely(b1i > columns + 1)) {
                                error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                                err = true;
                                break;
                            }
                        }
                    }
                    break;
                case twoDimVertR2:
                    if (unlikely(b1i > columns + 1)) {
                        error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                        err = true;
                        break;
                    }
                    addPixels(refLine[b1i] + 2, blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < columns) {
                        ++b1i;
                        while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
                            b1i += 2;
                            if (unlikely(b1i > columns + 1)) {
                                error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                                err = true;
                                break;
                            }
                        }
                    }
                    break;
                case twoDimVertR1:
                    if (unlikely(b1i > columns + 1)) {
                        error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                        err = true;
                        break;
                    }
                    addPixels(refLine[b1i] + 1, blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < columns) {
             