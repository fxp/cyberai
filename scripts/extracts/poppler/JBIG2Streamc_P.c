// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 16/38



    // get the Huffman tables
    huffDHTable = huffDWTable = nullptr; // make gcc happy
    huffBMSizeTable = huffAggInstTable = nullptr; // make gcc happy
    i = 0;
    if (huff) {
        const unsigned int huffDH = (flags >> 2) & 3;
        if (huffDH == 0) {
            huffDHTable = huffTableD;
        } else if (huffDH == 1) {
            huffDHTable = huffTableE;
        } else {
            if (i >= codeTables.size()) {
                error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 symbol dictionary");
                return false;
            }
            huffDHTable = static_cast<JBIG2CodeTable *>(codeTables[i++])->getHuffTable();
        }

        const unsigned int huffDW = (flags >> 4) & 3;
        if (huffDW == 0) {
            huffDWTable = huffTableB;
        } else if (huffDW == 1) {
            huffDWTable = huffTableC;
        } else {
            if (i >= codeTables.size()) {
                error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 symbol dictionary");
                return false;
            }
            huffDWTable = static_cast<JBIG2CodeTable *>(codeTables[i++])->getHuffTable();
        }

        const unsigned int huffBMSize = (flags >> 6) & 1;
        if (huffBMSize == 0) {
            huffBMSizeTable = huffTableA;
        } else {
            if (i >= codeTables.size()) {
                error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 symbol dictionary");
                return false;
            }
            huffBMSizeTable = static_cast<JBIG2CodeTable *>(codeTables[i++])->getHuffTable();
        }

        const unsigned int huffAggInst = (flags >> 7) & 1;
        if (huffAggInst == 0) {
            huffAggInstTable = huffTableA;
        } else {
            if (i >= codeTables.size()) {
                error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 symbol dictionary");
                return false;
            }
            huffAggInstTable = static_cast<JBIG2CodeTable *>(codeTables[i++])->getHuffTable();
        }
    }

    // set up the Huffman decoder
    if (huff) {
        huffDecoder->reset();

        // set up the arithmetic decoder
    } else {
        if (contextUsed && inputSymbolDict) {
            resetGenericStats(sdTemplate, inputSymbolDict->getGenericRegionStats());
        } else {
            resetGenericStats(sdTemplate, nullptr);
        }
        if (!resetIntStats(symCodeLen)) {
            return false;
        }
        arithDecoder->start();
    }

    // set up the arithmetic decoder for refinement/aggregation
    if (refAgg) {
        if (contextUsed && inputSymbolDict) {
            resetRefinementStats(sdrTemplate, inputSymbolDict->getRefinementRegionStats());
        } else {
            resetRefinementStats(sdrTemplate, nullptr);
        }
    }

    if (huff && !refAgg) {
        if (sizeIsBiggerThanVectorMaxSize(numNewSyms, symWidths)) {
            return false;
        }
        symWidths.resize(numNewSyms);
    }

    symHeight = 0;
    i = 0;
    while (i < numNewSyms) {

        // read the height class delta height
        if (huff) {
            huffDecoder->decodeInt(&dh, huffDHTable);
        } else {
            arithDecoder->decodeInt(&dh, iadhStats);
        }
        if (dh < 0 && (unsigned int)-dh >= symHeight) {
            error(errSyntaxError, curStr->getPos(), "Bad delta-height value in JBIG2 symbol dictionary");
            return false;
        }
        symHeight += dh;
        if (unlikely(symHeight > 0x40000000)) {
            error(errSyntaxError, curStr->getPos(), "Bad height value in JBIG2 symbol dictionary");
            return false;
        }
        symWidth = 0;
        totalWidth = 0;
        j = i;

        // read the symbols in this height class
        while (true) {

            // read the delta width
            if (huff) {
                if (!huffDecoder->decodeInt(&dw, huffDWTable)) {
                    break;
                }
            } else {
                if (!arithDecoder->decodeInt(&dw, iadwStats)) {
                    break;
                }
            }
            if (dw < 0 && (unsigned int)-dw >= symWidth) {
                error(errSyntaxError, curStr->getPos(), "Bad delta-height value in JBIG2 symbol dictionary");
                return false;
            }
            symWidth += dw;
            if (i >= numNewSyms) {
                error(errSyntaxError, curStr->getPos(), "Too many symbols in JBIG2 symbol dictionary");
                return false;
            }

            // using a collective bitmap, so don't read a bitmap here
            if (huff && !refAgg) {
                symWidths[i] = symWidth;
                totalWidth += symWidth;