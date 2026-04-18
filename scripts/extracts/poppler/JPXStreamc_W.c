// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 23/31



    if (cb->arithDecoder) {
        cover(63);
        cb->arithDecoder->restart(cb->dataLen[0]);
    } else {
        cover(64);
        cb->arithDecoder = new JArithmeticDecoder();
        cb->arithDecoder->setStream(bufStr.get(), cb->dataLen[0]);
        cb->arithDecoder->start();
        cb->stats = new JArithmeticDecoderStats(jpxNContexts);
        cb->stats->setEntry(jpxContextSigProp, 4, 0);
        cb->stats->setEntry(jpxContextRunLength, 3, 0);
        cb->stats->setEntry(jpxContextUniform, 46, 0);
    }

    for (i = 0; i < cb->nCodingPasses; ++i) {
        if ((tileComp->codeBlockStyle & 0x04) && i > 0) {
            cb->arithDecoder->setStream(bufStr.get(), cb->dataLen[i]);
            cb->arithDecoder->start();
        }

        switch (cb->nextPass) {

        //----- significance propagation pass
        case jpxPassSigProp:
            cover(65);
            for (y0 = cb->y0, coeff0 = cb->coeffs, touched0 = cb->touched; y0 < cb->y1; y0 += 4, coeff0 += 4 * tileComp->w, touched0 += 4 << tileComp->codeBlockW) {
                for (x = cb->x0, coeff1 = coeff0, touched1 = touched0; x < cb->x1; ++x, ++coeff1, ++touched1) {
                    for (y1 = 0, coeff = coeff1, touched = touched1; y1 < 4 && y0 + y1 < cb->y1; ++y1, coeff += tileComp->w, touched += tileComp->cbW) {
                        if (!*coeff) {
                            horiz = vert = diag = 0;
                            horizSign = vertSign = 2;
                            if (x > cb->x0) {
                                if (coeff[-1]) {
                                    ++horiz;
                                    horizSign += coeff[-1] < 0 ? -1 : 1;
                                }
                                if (y0 + y1 > cb->y0) {
                                    diag += coeff[-(int)tileComp->w - 1] ? 1 : 0;
                                }
                                if (y0 + y1 < cb->y1 - 1 && (!(tileComp->codeBlockStyle & 0x08) || y1 < 3)) {
                                    diag += coeff[tileComp->w - 1] ? 1 : 0;
                                }
                            }
                            if (x < cb->x1 - 1) {
                                if (coeff[1]) {
                                    ++horiz;
                                    horizSign += coeff[1] < 0 ? -1 : 1;
                                }
                                if (y0 + y1 > cb->y0) {
                                    diag += coeff[-(int)tileComp->w + 1] ? 1 : 0;
                                }
                                if (y0 + y1 < cb->y1 - 1 && (!(tileComp->codeBlockStyle & 0x08) || y1 < 3)) {
                                    diag += coeff[tileComp->w + 1] ? 1 : 0;
                                }
                            }
                            if (y0 + y1 > cb->y0) {
                                if (coeff[-(int)tileComp->w]) {
                                    ++vert;
                                    vertSign += coeff[-(int)tileComp->w] < 0 ? -1 : 1;
                                }
                            }
                            if (y0 + y1 < cb->y1 - 1 && (!(tileComp->codeBlockStyle & 0x08) || y1 < 3)) {
                                if (coeff[tileComp->w]) {
                                    ++vert;
                                    vertSign += coeff[tileComp->w] < 0 ? -1 : 1;
                                }
                            }
                            cx = sigPropContext[horiz][vert][diag][res == 0 ? 1 : sb];
                            if (cx != 0) {
                                if (cb->arithDecoder->decodeBit(cx, cb->stats)) {
                                    cx = signContext[horizSign][vertSign][0];
                                    xorBit = signContext[horizSign][vertSign][1];
                                    if (cb->arithDecoder->decodeBit(cx, cb->stats) ^ xorBit) {
                                        *coeff = -1;
                                    } else {
                                        *coeff = 1;
                                    }
                                }
                                *touched = 1;
                            }
                        }
                    }
                }
            }
            ++cb->nextPass;
            break;