// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 25/31



        //----- cleanup pass
        case jpxPassCleanup:
            cover(67);
            for (y0 = cb->y0, coeff0 = cb->coeffs, touched0 = cb->touched; y0 < cb->y1; y0 += 4, coeff0 += 4 * tileComp->w, touched0 += 4 << tileComp->codeBlockW) {
                for (x = cb->x0, coeff1 = coeff0, touched1 = touched0; x < cb->x1; ++x, ++coeff1, ++touched1) {
                    y1 = 0;
                    if (y0 + 3 < cb->y1 && !(*touched1) && !(touched1[tileComp->cbW]) && !(touched1[2 * tileComp->cbW]) && !(touched1[3 * tileComp->cbW]) && (x == cb->x0 || y0 == cb->y0 || !coeff1[-(int)tileComp->w - 1])
                        && (y0 == cb->y0 || !coeff1[-(int)tileComp->w]) && (x == cb->x1 - 1 || y0 == cb->y0 || !coeff1[-(int)tileComp->w + 1])
                        && (x == cb->x0 || (!coeff1[-1] && !coeff1[tileComp->w - 1] && !coeff1[2 * tileComp->w - 1] && !coeff1[3 * tileComp->w - 1]))
                        && (x == cb->x1 - 1 || (!coeff1[1] && !coeff1[tileComp->w + 1] && !coeff1[2 * tileComp->w + 1] && !coeff1[3 * tileComp->w + 1]))
                        && ((tileComp->codeBlockStyle & 0x08)
                            || ((x == cb->x0 || y0 + 4 == cb->y1 || !coeff1[4 * tileComp->w - 1]) && (y0 + 4 == cb->y1 || !coeff1[4 * tileComp->w]) && (x == cb->x1 - 1 || y0 + 4 == cb->y1 || !coeff1[4 * tileComp->w + 1])))) {
                        if (cb->arithDecoder->decodeBit(jpxContextRunLength, cb->stats)) {
                            y1 = cb->arithDecoder->decodeBit(jpxContextUniform, cb->stats);
                            y1 = (y1 << 1) | cb->arithDecoder->decodeBit(jpxContextUniform, cb->stats);
                            coeff = &coeff1[y1 * tileComp->w];
                            cx = signContext[2][2][0];
                            xorBit = signContext[2][2][1];
                            if (cb->arithDecoder->decodeBit(cx, cb->stats) ^ xorBit) {
                                *coeff = -1;
                            } else {
                                *coeff = 1;
                            }
                            ++y1;
                        } else {
                            y1 = 4;
                        }
                    }
                    for (coeff = &coeff1[y1 * tileComp->w], touched = &touched1[y1 << tileComp->codeBlockW]; y1 < 4 && y0 + y1 < cb->y1; ++y1, coeff += tileComp->w, touched += tileComp->cbW) {
                        if (!*touched) {
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
                            if (cb->arithDecoder->decodeBit(cx, cb->stats)) {
                                cx = signContext[horizSign][vertSign][0];
                                xorBit = signContext[horizSign][vertSign][1];
                                if (cb->arithDecoder->decodeBit(cx, cb->stats) ^ xorBit) {
                                    *coeff = -