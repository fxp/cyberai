// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 22/38



            // T value
            if (strips == 1) {
                dt = 0;
            } else if (huff) {
                dt = huffDecoder->readBits(logStrips);
            } else {
                arithDecoder->decodeInt(&dt, iaitStats);
            }
            if (unlikely(checkedAdd(t, dt, &tt))) {
                return nullptr;
            }

            // symbol ID
            if (huff) {
                if (symCodeTab) {
                    huffDecoder->decodeInt(&j, symCodeTab);
                    symID = (unsigned int)j;
                } else {
                    symID = huffDecoder->readBits(symCodeLen);
                }
            } else {
                if (iaidStats == nullptr) {
                    return nullptr;
                }
                symID = arithDecoder->decodeIAID(symCodeLen, iaidStats);
            }

            if (symID >= numSyms) {
                error(errSyntaxError, curStr->getPos(), "Invalid symbol number in JBIG2 text region");
                if (unlikely(numInstances - inst > 0x800)) {
                    // don't loop too often with damaged JBIg2 streams
                    return nullptr;
                }
            } else {

                // get the symbol bitmap
                symbolBitmap = nullptr;
                if (refine) {
                    if (huff) {
                        ri = (int)huffDecoder->readBit();
                    } else {
                        arithDecoder->decodeInt(&ri, iariStats);
                    }
                } else {
                    ri = 0;
                }
                if (ri) {
                    bool decodeSuccess;
                    if (huff) {
                        decodeSuccess = huffDecoder->decodeInt(&rdw, huffRDWTable);
                        decodeSuccess = decodeSuccess && huffDecoder->decodeInt(&rdh, huffRDHTable);
                        decodeSuccess = decodeSuccess && huffDecoder->decodeInt(&rdx, huffRDXTable);
                        decodeSuccess = decodeSuccess && huffDecoder->decodeInt(&rdy, huffRDYTable);
                        decodeSuccess = decodeSuccess && huffDecoder->decodeInt(&bmSize, huffRSizeTable);
                        huffDecoder->reset();
                        arithDecoder->start();
                    } else {
                        decodeSuccess = arithDecoder->decodeInt(&rdw, iardwStats);
                        decodeSuccess = decodeSuccess && arithDecoder->decodeInt(&rdh, iardhStats);
                        decodeSuccess = decodeSuccess && arithDecoder->decodeInt(&rdx, iardxStats);
                        decodeSuccess = decodeSuccess && arithDecoder->decodeInt(&rdy, iardyStats);
                    }

                    if (decodeSuccess && syms[symID]) {
                        if (checkedAdd(((rdw >= 0) ? rdw : rdw - 1) / 2, rdx, &refDX)) {
                            return nullptr;
                        }
                        if (checkedAdd(((rdh >= 0) ? rdh : rdh - 1) / 2, rdy, &refDY)) {
                            return nullptr;
                        }
                        int refinementSectionW, refinementSectionH;
                        if (checkedAdd(rdw, syms[symID]->getWidth(), &refinementSectionW)) {
                            return nullptr;
                        }
                        if (checkedAdd(rdh, syms[symID]->getHeight(), &refinementSectionH)) {
                            return nullptr;
                        }

                        symbolBitmap = readGenericRefinementRegion(refinementSectionW, refinementSectionH, templ, false, syms[symID], refDX, refDY, atx, aty).release();
                    }
                    //~ do we need to use the bmSize value here (in Huffman mode)?
                } else {
                    symbolBitmap = syms[symID];
                }