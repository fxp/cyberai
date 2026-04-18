// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 24/31



        //----- magnitude refinement pass
        case jpxPassMagRef:
            cover(66);
            for (y0 = cb->y0, coeff0 = cb->coeffs, touched0 = cb->touched; y0 < cb->y1; y0 += 4, coeff0 += 4 * tileComp->w, touched0 += 4 << tileComp->codeBlockW) {
                for (x = cb->x0, coeff1 = coeff0, touched1 = touched0; x < cb->x1; ++x, ++coeff1, ++touched1) {
                    for (y1 = 0, coeff = coeff1, touched = touched1; y1 < 4 && y0 + y1 < cb->y1; ++y1, coeff += tileComp->w, touched += tileComp->cbW) {
                        if (*coeff && !*touched) {
                            if (*coeff == 1 || *coeff == -1) {
                                all = 0;
                                if (x > cb->x0) {
                                    all += coeff[-1] ? 1 : 0;
                                    if (y0 + y1 > cb->y0) {
                                        all += coeff[-(int)tileComp->w - 1] ? 1 : 0;
                                    }
                                    if (y0 + y1 < cb->y1 - 1 && (!(tileComp->codeBlockStyle & 0x08) || y1 < 3)) {
                                        all += coeff[tileComp->w - 1] ? 1 : 0;
                                    }
                                }
                                if (x < cb->x1 - 1) {
                                    all += coeff[1] ? 1 : 0;
                                    if (y0 + y1 > cb->y0) {
                                        all += coeff[-(int)tileComp->w + 1] ? 1 : 0;
                                    }
                                    if (y0 + y1 < cb->y1 - 1 && (!(tileComp->codeBlockStyle & 0x08) || y1 < 3)) {
                                        all += coeff[tileComp->w + 1] ? 1 : 0;
                                    }
                                }
                                if (y0 + y1 > cb->y0) {
                                    all += coeff[-(int)tileComp->w] ? 1 : 0;
                                }
                                if (y0 + y1 < cb->y1 - 1 && (!(tileComp->codeBlockStyle & 0x08) || y1 < 3)) {
                                    all += coeff[tileComp->w] ? 1 : 0;
                                }
                                cx = all ? 15 : 14;
                            } else {
                                cx = 16;
                            }
                            bit = cb->arithDecoder->decodeBit(cx, cb->stats);
                            if (*coeff < 0) {
                                *coeff = (*coeff << 1) - bit;
                            } else {
                                *coeff = (*coeff << 1) + bit;
                            }
                            *touched = 1;
                        }
                    }
                }
            }
            ++cb->nextPass;
            break;