// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 17/38



                // refinement/aggregate coding
            } else if (refAgg) {
                int refAggNum;
                if (huff) {
                    if (!huffDecoder->decodeInt(&refAggNum, huffAggInstTable)) {
                        break;
                    }
                } else {
                    if (!arithDecoder->decodeInt(&refAggNum, iaaiStats)) {
                        break;
                    }
                }
                //~ This special case was added about a year before the final draft
                //~ of the JBIG2 spec was released.  I have encountered some old
                //~ JBIG2 images that predate it.
                //~ if (0) {
                if (refAggNum == 1) {
                    int refDX = 0, refDY = 0, bmSize;
                    if (huff) {
                        symID = huffDecoder->readBits(symCodeLen);
                        huffDecoder->decodeInt(&refDX, huffTableO);
                        huffDecoder->decodeInt(&refDY, huffTableO);
                        huffDecoder->decodeInt(&bmSize, huffTableA);
                        huffDecoder->reset();
                        arithDecoder->start();
                    } else {
                        if (iaidStats == nullptr) {
                            return false;
                        }
                        symID = arithDecoder->decodeIAID(symCodeLen, iaidStats);
                        arithDecoder->decodeInt(&refDX, iardxStats);
                        arithDecoder->decodeInt(&refDY, iardyStats);
                    }
                    if (symID >= numInputSyms + i) {
                        error(errSyntaxError, curStr->getPos(), "Invalid symbol ID in JBIG2 symbol dictionary");
                        return false;
                    }
                    JBIG2Bitmap *refBitmap = bitmaps[symID];
                    if (unlikely(refBitmap == nullptr)) {
                        error(errSyntaxError, curStr->getPos(), "Invalid ref bitmap for symbol ID {0:ud} in JBIG2 symbol dictionary", symID);
                        return false;
                    }
                    std::unique_ptr<JBIG2Bitmap> bitmap = readGenericRefinementRegion(symWidth, symHeight, sdrTemplate, false, refBitmap, refDX, refDY, sdrATX, sdrATY);
                    bitmaps[numInputSyms + i] = bitmap.get();
                    bitmapsToDelete.emplace_back(std::move(bitmap));
                    //~ do we need to use the bmSize value here (in Huffman mode)?
                } else {
                    std::unique_ptr<JBIG2Bitmap> bitmap = readTextRegion(huff, true, symWidth, symHeight, refAggNum, 0, numInputSyms + i, nullptr, symCodeLen, bitmaps, 0, 0, 0, 1, 0, huffTableF, huffTableH, huffTableK, huffTableO,
                                                                         huffTableO, huffTableO, huffTableO, huffTableA, sdrTemplate, sdrATX, sdrATY);
                    if (unlikely(!bitmap)) {
                        error(errSyntaxError, curStr->getPos(), "NULL bitmap in readTextRegion");
                        return false;
                    }
                    bitmaps[numInputSyms + i] = bitmap.get();
                    bitmapsToDelete.emplace_back(std::move(bitmap));
                }

                // non-ref/agg coding
            } else {
                std::unique_ptr<JBIG2Bitmap> bitmap = readGenericBitmap(false, symWidth, symHeight, sdTemplate, false, false, nullptr, sdATX, sdATY, 0);
                if (unlikely(!bitmap)) {
                    error(errSyntaxError, curStr->getPos(), "NULL bitmap in readGenericBitmap");
                    return false;
                }
                bitmaps[numInputSyms + i] = bitmap.get();
                bitmapsToDelete.emplace_back(std::move(bitmap));
            }

            ++i;
        }