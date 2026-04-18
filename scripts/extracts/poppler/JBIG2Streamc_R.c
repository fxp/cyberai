// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 18/38



        // read the collective bitmap
        if (huff && !refAgg) {
            int bmSize;
            huffDecoder->decodeInt(&bmSize, huffBMSizeTable);
            huffDecoder->reset();
            std::unique_ptr<JBIG2Bitmap> collBitmap;
            if (bmSize == 0) {
                collBitmap = std::make_unique<JBIG2Bitmap>(0, totalWidth, symHeight);
                bmSize = symHeight * ((totalWidth + 7) >> 3);
                unsigned char *p = collBitmap->getDataPtr();
                if (unlikely(p == nullptr)) {
                    return false;
                }
                for (k = 0; k < (unsigned int)bmSize; ++k) {
                    const int c = curStr->getChar();
                    if (c == EOF) {
                        memset(p, 0, bmSize - k);
                        break;
                    }
                    *p++ = (unsigned char)c;
                }
                byteCounter += k;
            } else {
                collBitmap = readGenericBitmap(true, totalWidth, symHeight, 0, false, false, nullptr, nullptr, nullptr, bmSize);
            }
            if (collBitmap) {
                unsigned int x = 0;
                for (; j < i; ++j) {
                    std::unique_ptr<JBIG2Bitmap> bitmap = collBitmap->getSlice(x, 0, symWidths[j], symHeight);
                    if (!bitmap) {
                        return false;
                    }
                    bitmaps[numInputSyms + j] = bitmap.get();
                    bitmapsToDelete.emplace_back(std::move(bitmap));
                    x += symWidths[j];
                }
            } else {
                error(errSyntaxError, curStr->getPos(), "collBitmap was null");
                return false;
            }
        }
    }

    // create the symbol dict object
    symbolDict = std::make_unique<JBIG2SymbolDict>(segNum, numExSyms);
    if (!symbolDict->isOk()) {
        return false;
    }

    // exported symbol list
    i = j = 0;
    ex = false;
    run = 0; // initialize it once in case the first decodeInt fails
             // we do not want to use uninitialized memory
    while (i < numInputSyms + numNewSyms) {
        if (huff) {
            huffDecoder->decodeInt(&run, huffTableA);
        } else {
            arithDecoder->decodeInt(&run, iaexStats);
        }
        if (i + run > numInputSyms + numNewSyms || (ex && j + run > numExSyms)) {
            error(errSyntaxError, curStr->getPos(), "Too many exported symbols in JBIG2 symbol dictionary");
            for (; j < numExSyms; ++j) {
                symbolDict->setBitmap(j, nullptr);
            }
            return false;
        }
        if (ex) {
            for (int cnt = 0; cnt < run; ++cnt) {
                symbolDict->setBitmap(j++, std::make_unique<JBIG2Bitmap>(bitmaps[i++]));
            }
        } else {
            i += run;
        }
        ex = !ex;
    }
    if (j != numExSyms) {
        error(errSyntaxError, curStr->getPos(), "Too few symbols in JBIG2 symbol dictionary");
        for (; j < numExSyms; ++j) {
            symbolDict->setBitmap(j, nullptr);
        }
        return false;
    }

    // save the arithmetic decoder stats
    if (!huff && contextRetained) {
        symbolDict->setGenericRegionStats(genericRegionStats->copy());
        if (refAgg) {
            symbolDict->setRefinementRegionStats(refinementRegionStats->copy());
        }
    }

    // store the new symbol dict
    segments.push_back(std::move(symbolDict));

    return true;
}

bool JBIG2Stream::readTextRegionSeg(unsigned int segNum, bool imm, const std::vector<unsigned int> &refSegs)
{
    std::unique_ptr<JBIG2Bitmap> bitmap;
    JBIG2HuffmanTable runLengthTab[36];
    JBIG2HuffmanTable *symCodeTab = nullptr;
    const JBIG2HuffmanTable *huffFSTable, *huffDSTable, *huffDTTable;
    const JBIG2HuffmanTable *huffRDWTable, *huffRDHTable;
    const JBIG2HuffmanTable *huffRDXTable, *huffRDYTable, *huffRSizeTable;
    std::vector<JBIG2Segment *> codeTables;
    std::vector<JBIG2Bitmap *> syms;
    unsigned int w, h, x, y, segInfoFlags;
    unsigned int flags;
    int sOffset;
    unsigned int huffFlags, huffFS, huffDS, huffDT;
    unsigned int huffRDW, huffRDH, huffRDX, huffRDY, huffRSize;
    unsigned int numInstances, numSyms, symCodeLen;
    int atx[2], aty[2];
    unsigned int i;
    int j = 0;

    // region segment info field
    if (!readULong(&w) || !readULong(&h) || !readULong(&x) || !readULong(&y) || !readUByte(&segInfoFlags)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }
    const unsigned int extCombOp = segInfoFlags & 7;