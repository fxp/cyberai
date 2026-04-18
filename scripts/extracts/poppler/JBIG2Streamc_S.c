// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 19/38



    // rest of the text region header
    if (!readUWord(&flags)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }
    unsigned int huff = flags & 1;
    const unsigned int refine = (flags >> 1) & 1;
    const unsigned int logStrips = (flags >> 2) & 3;
    const unsigned int refCorner = (flags >> 4) & 3;
    const unsigned int transposed = (flags >> 6) & 1;
    const unsigned int combOp = (flags >> 7) & 3;
    const unsigned int defPixel = (flags >> 9) & 1;
    sOffset = (flags >> 10) & 0x1f;
    if (sOffset & 0x10) {
        sOffset |= -1 - 0x0f;
    }
    const unsigned int templ = (flags >> 15) & 1;
    huffFS = huffDS = huffDT = 0; // make gcc happy
    huffRDW = huffRDH = huffRDX = huffRDY = huffRSize = 0; // make gcc happy
    if (huff) {
        if (!readUWord(&huffFlags)) {
            error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
            return false;
        }
        huffFS = huffFlags & 3;
        huffDS = (huffFlags >> 2) & 3;
        huffDT = (huffFlags >> 4) & 3;
        huffRDW = (huffFlags >> 6) & 3;
        huffRDH = (huffFlags >> 8) & 3;
        huffRDX = (huffFlags >> 10) & 3;
        huffRDY = (huffFlags >> 12) & 3;
        huffRSize = (huffFlags >> 14) & 1;
    }
    if (refine && templ == 0) {
        if (!readByte(&atx[0]) || !readByte(&aty[0]) || !readByte(&atx[1]) || !readByte(&aty[1])) {
            error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
            return false;
        }
    }
    if (!readULong(&numInstances)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }

    // get symbol dictionaries and tables
    numSyms = 0;
    for (const unsigned int refSegI : refSegs) {
        JBIG2Segment *seg = findSegment(refSegI);
        if (seg) {
            if (seg->getType() == jbig2SegSymbolDict) {
                const unsigned int segSize = static_cast<JBIG2SymbolDict *>(seg)->getSize();
                if (unlikely(checkedAdd(numSyms, segSize, &numSyms))) {
                    error(errSyntaxError, getPos(), "Too many symbols in JBIG2 text region");
                    return false;
                }
            } else if (seg->getType() == jbig2SegCodeTable) {
                codeTables.push_back(seg);
            }
        } else {
            error(errSyntaxError, curStr->getPos(), "Invalid segment reference in JBIG2 text region");
            return false;
        }
    }
    i = numSyms;
    if (i <= 1) {
        symCodeLen = huff ? 1 : 0;
    } else {
        --i;
        symCodeLen = 0;
        // i = floor((numSyms-1) / 2^symCodeLen)
        while (i > 0) {
            ++symCodeLen;
            i >>= 1;
        }
    }

    // get the symbol bitmaps
    if (sizeIsBiggerThanVectorMaxSize(numSyms, syms)) {
        return false;
    }
    syms.resize(numSyms);
    unsigned int kk = 0;
    for (const unsigned int refSegI : refSegs) {
        JBIG2Segment *seg = findSegment(refSegI);
        if (seg) {
            if (seg->getType() == jbig2SegSymbolDict) {
                auto *symbolDict = static_cast<JBIG2SymbolDict *>(seg);
                for (unsigned int k = 0; k < symbolDict->getSize(); ++k) {
                    syms[kk++] = symbolDict->getBitmap(k);
                }
            }
        }
    }