// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 20/38



    // get the Huffman tables
    huffFSTable = huffDSTable = huffDTTable = nullptr; // make gcc happy
    huffRDWTable = huffRDHTable = nullptr; // make gcc happy
    huffRDXTable = huffRDYTable = huffRSizeTable = nullptr; // make gcc happy
    i = 0;
    if (huff) {
        if (huffFS == 0) {
            huffFSTable = huffTableF;
        } else if (huffFS == 1) {
            huffFSTable = huffTableG;
        } else {
            if (i >= codeTables.size()) {
                error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 text region");
                return false;
            }
            huffFSTable = static_cast<JBIG2CodeTable *>(codeTables[i++])->getHuffTable();
        }
        if (huffDS == 0) {
            huffDSTable = huffTableH;
        } else if (huffDS == 1) {
            huffDSTable = huffTableI;
        } else if (huffDS == 2) {
            huffDSTable = huffTableJ;
        } else {
            if (i >= codeTables.size()) {
                error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 text region");
                return false;
            }
            huffDSTable = static_cast<JBIG2CodeTable *>(codeTables[i++])->getHuffTable();
        }
        if (huffDT == 0) {
            huffDTTable = huffTableK;
        } else if (huffDT == 1) {
            huffDTTable = huffTableL;
        } else if (huffDT == 2) {
            huffDTTable = huffTableM;
        } else {
            if (i >= codeTables.size()) {
                error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 text region");
                return false;
            }
            huffDTTable = static_cast<JBIG2CodeTable *>(codeTables[i++])->getHuffTable();
        }
        if (huffRDW == 0) {
            huffRDWTable = huffTableN;
        } else if (huffRDW == 1) {
            huffRDWTable = huffTableO;
        } else {
            if (i >= codeTables.size()) {
                error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 text region");
                return false;
            }
            huffRDWTable = static_cast<JBIG2CodeTable *>(codeTables[i++])->getHuffTable();
        }
        if (huffRDH == 0) {
            huffRDHTable = huffTableN;
        } else if (huffRDH == 1) {
            huffRDHTable = huffTableO;
        } else {
            if (i >= codeTables.size()) {
                error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 text region");
                return false;
            }
            huffRDHTable = static_cast<JBIG2CodeTable *>(codeTables[i++])->getHuffTable();
        }
        if (huffRDX == 0) {
            huffRDXTable = huffTableN;
        } else if (huffRDX == 1) {
            huffRDXTable = huffTableO;
        } else {
            if (i >= codeTables.size()) {
                error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 text region");
                return false;
            }
            huffRDXTable = static_cast<JBIG2CodeTable *>(codeTables[i++])->getHuffTable();
        }
        if (huffRDY == 0) {
            huffRDYTable = huffTableN;
        } else if (huffRDY == 1) {
            huffRDYTable = huffTableO;
        } else {
            if (i >= codeTables.size()) {
                error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 text region");
                return false;
            }
            huffRDYTable = static_cast<JBIG2CodeTable *>(codeTables[i++])->getHuffTable();
        }
        if (huffRSize == 0) {
            huffRSizeTable = huffTableA;
        } else {
            if (i >= codeTables.size()) {
                error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 text region");
                return false;
            }
            huffRSizeTable = static_cast<JBIG2CodeTable *>(codeTables[i++])->getHuffTable();
        }
    }

    // symbol ID Huffman decoding table
    if (huff) {
        huffDecoder->reset();
        for (i = 0; i < 32; ++i) {
            runLengthTab[i].val = i;
            runLengthTab[i].prefixLen = huffDecoder->readBits(4);
            runLengthTab[i].rangeLen = 0;
        }
        runLengthTab[32].val = 0x103;
        runLengthTab[32].prefixLen = huffDecoder->readBits(4);
        runLengthTab[32].rangeLen = 2;
        runLengthTab[33].val = 0x203;
        runLengthTab[33].prefixLen = huffDecoder->readBits(4);
        runLengthTab[33].rangeLen = 3;
        runLengthTab[34].val = 0x20b;
        runLengthTab[34].prefixLen = huffDecoder->readBits(4);
        runLengthTab[34].rangeLen = 7;
        runLengthTab[35].prefixLen = 0;
        runLengthTab[35].rangeLen = jbig2HuffmanEOT;
        if (!JBIG2HuffmanDecoder::buildTable(runLengthTab, 35)) {
            huff = false;
        }
    }