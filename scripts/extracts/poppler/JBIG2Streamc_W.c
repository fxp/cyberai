// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 23/38



                if (symbolBitmap) {
                    // combine the symbol bitmap into the region bitmap
                    //~ something is wrong here - refCorner shouldn't degenerate into
                    //~   two cases
                    bw = symbolBitmap->getWidth() - 1;
                    if (unlikely(symbolBitmap->getHeight() == 0)) {
                        error(errSyntaxError, curStr->getPos(), "Invalid symbol bitmap height");
                        if (ri) {
                            delete symbolBitmap;
                        }
                        return nullptr;
                    }
                    bh = symbolBitmap->getHeight() - 1;
                    if (transposed) {
                        if (unlikely(s > 2 * bitmap->getHeight())) {
                            error(errSyntaxError, curStr->getPos(), "Invalid JBIG2 combine");
                            if (ri) {
                                delete symbolBitmap;
                            }
                            return nullptr;
                        }
                        switch (refCorner) {
                        case 0: // bottom left
                            bitmap->combine(*symbolBitmap, tt, s, combOp);
                            break;
                        case 1: // top left
                            bitmap->combine(*symbolBitmap, tt, s, combOp);
                            break;
                        case 2: // bottom right
                            bitmap->combine(*symbolBitmap, tt - bw, s, combOp);
                            break;
                        case 3: // top right
                            bitmap->combine(*symbolBitmap, tt - bw, s, combOp);
                            break;
                        }
                        s += bh;
                    } else {
                        switch (refCorner) {
                        case 0: // bottom left
                            if (unlikely(tt - (int)bh > 2 * bitmap->getHeight())) {
                                error(errSyntaxError, curStr->getPos(), "Invalid JBIG2 combine");
                                if (ri) {
                                    delete symbolBitmap;
                                }
                                return nullptr;
                            }
                            bitmap->combine(*symbolBitmap, s, tt - bh, combOp);
                            break;
                        case 1: // top left
                            if (unlikely(tt > 2 * bitmap->getHeight())) {
                                error(errSyntaxError, curStr->getPos(), "Invalid JBIG2 combine");
                                if (ri) {
                                    delete symbolBitmap;
                                }
                                return nullptr;
                            }
                            bitmap->combine(*symbolBitmap, s, tt, combOp);
                            break;
                        case 2: // bottom right
                            if (unlikely(tt - (int)bh > 2 * bitmap->getHeight())) {
                                error(errSyntaxError, curStr->getPos(), "Invalid JBIG2 combine");
                                if (ri) {
                                    delete symbolBitmap;
                                }
                                return nullptr;
                            }
                            bitmap->combine(*symbolBitmap, s, tt - bh, combOp);
                            break;
                        case 3: // top right
                            if (unlikely(tt > 2 * bitmap->getHeight())) {
                                error(errSyntaxError, curStr->getPos(), "Invalid JBIG2 combine");
                                if (ri) {
                                    delete symbolBitmap;
                                }
                                return nullptr;
                            }
                            bitmap->combine(*symbolBitmap, s, tt, combOp);
                            break;
                        }
                        s += bw;
                    }
                    if (ri) {
                        delete symbolBitmap;
                    }
                } else {
                    // NULL symbolBitmap only happens on error
                    return nullptr;
                }
            }

            // next instance
            ++inst;

            // next S value
            if (huff) {
                if (!huffDecoder->decodeInt(&ds, huffDSTable)) {
                    break;
                }
            } else {
                if (!arithDecoder->decodeInt(&ds, iadsStats)) {
                    break;
                }
            }
            if (checkedAdd(s, sOffset + ds, &s)) {
                return nullptr;
            }
        }
    }

    return bitmap;
}