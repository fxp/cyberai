// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 15/38



bool JBIG2Stream::readSymbolDictSeg(unsigned int segNum, const std::vector<unsigned int> &refSegs)
{
    std::unique_ptr<JBIG2SymbolDict> symbolDict;
    const JBIG2HuffmanTable *huffDHTable, *huffDWTable;
    const JBIG2HuffmanTable *huffBMSizeTable, *huffAggInstTable;
    std::vector<JBIG2Segment *> codeTables;
    JBIG2SymbolDict *inputSymbolDict;
    unsigned int flags;
    int sdATX[4], sdATY[4], sdrATX[2], sdrATY[2];
    unsigned int numExSyms, numNewSyms, numInputSyms;
    std::vector<JBIG2Bitmap *> bitmaps;
    std::vector<std::unique_ptr<JBIG2Bitmap>> bitmapsToDelete;
    std::vector<unsigned int> symWidths;
    unsigned int symHeight, symWidth, totalWidth, symID;
    int dh = 0, dw;
    bool ex;
    int run;
    unsigned int i, j, k;

    // symbol dictionary flags
    if (!readUWord(&flags)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }
    const unsigned int sdTemplate = (flags >> 10) & 3;
    const unsigned int sdrTemplate = (flags >> 12) & 1;
    const unsigned int huff = flags & 1;
    const unsigned int refAgg = (flags >> 1) & 1;
    const unsigned int contextUsed = (flags >> 8) & 1;
    const unsigned int contextRetained = (flags >> 9) & 1;

    // symbol dictionary AT flags
    if (!huff) {
        if (sdTemplate == 0) {
            if (!readByte(&sdATX[0]) || !readByte(&sdATY[0]) || !readByte(&sdATX[1]) || !readByte(&sdATY[1]) || !readByte(&sdATX[2]) || !readByte(&sdATY[2]) || !readByte(&sdATX[3]) || !readByte(&sdATY[3])) {
                error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
                return false;
            }
        } else {
            if (!readByte(&sdATX[0]) || !readByte(&sdATY[0])) {
                error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
                return false;
            }
        }
    }

    // symbol dictionary refinement AT flags
    if (refAgg && !sdrTemplate) {
        if (!readByte(&sdrATX[0]) || !readByte(&sdrATY[0]) || !readByte(&sdrATX[1]) || !readByte(&sdrATY[1])) {
            error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
            return false;
        }
    }

    // SDNUMEXSYMS and SDNUMNEWSYMS
    if (!readULong(&numExSyms) || !readULong(&numNewSyms)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }

    // get referenced segments: input symbol dictionaries and code tables
    numInputSyms = 0;
    for (const unsigned int refSegI : refSegs) {
        // This is need by bug 12014, returning false makes it not crash
        // but we end up with a empty page while acroread is able to render
        // part of it
        JBIG2Segment *seg = findSegment(refSegI);
        if (seg) {
            if (seg->getType() == jbig2SegSymbolDict) {
                j = static_cast<JBIG2SymbolDict *>(seg)->getSize();
                if (numInputSyms > UINT_MAX - j) {
                    error(errSyntaxError, curStr->getPos(), "Too many input symbols in JBIG2 symbol dictionary");
                    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
                    return false;
                }
                numInputSyms += j;
            } else if (seg->getType() == jbig2SegCodeTable) {
                codeTables.push_back(seg);
            }
        } else {
            return false;
        }
    }
    if (numInputSyms > UINT_MAX - numNewSyms) {
        error(errSyntaxError, curStr->getPos(), "Too many input symbols in JBIG2 symbol dictionary");
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }

    const unsigned int symCodeLen = calculateSymCodeLen(numInputSyms, numNewSyms, huff);

    // get the input symbol bitmaps
    if (sizeIsBiggerThanVectorMaxSize(numInputSyms + numNewSyms, bitmaps)) {
        error(errSyntaxError, curStr->getPos(), "Too many input symbols in JBIG2 symbol dictionary");
        return false;
    }
    bitmaps.resize(numInputSyms + numNewSyms);
    k = 0;
    inputSymbolDict = nullptr;
    for (const unsigned int refSegI : refSegs) {
        JBIG2Segment *seg = findSegment(refSegI);
        if (seg != nullptr && seg->getType() == jbig2SegSymbolDict) {
            inputSymbolDict = static_cast<JBIG2SymbolDict *>(seg);
            for (j = 0; j < inputSymbolDict->getSize(); ++j) {
                bitmaps[k++] = inputSymbolDict->getBitmap(j);
            }
        }
    }