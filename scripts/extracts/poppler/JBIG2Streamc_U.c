// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 21/38



    if (huff) {
        symCodeTab = (JBIG2HuffmanTable *)gmallocn_checkoverflow(numSyms + 1, sizeof(JBIG2HuffmanTable));
        if (!symCodeTab) {
            return false;
        }
        for (i = 0; i < numSyms; ++i) {
            symCodeTab[i].val = i;
            symCodeTab[i].rangeLen = 0;
        }
        i = 0;
        while (i < numSyms) {
            huffDecoder->decodeInt(&j, runLengthTab);
            if (j > 0x200) {
                for (j -= 0x200; j && i < numSyms; --j) {
                    symCodeTab[i++].prefixLen = 0;
                }
            } else if (j > 0x100) {
                if (unlikely(i == 0)) {
                    symCodeTab[i].prefixLen = 0;
                    ++i;
                }
                for (j -= 0x100; j && i < numSyms; --j) {
                    symCodeTab[i].prefixLen = symCodeTab[i - 1].prefixLen;
                    ++i;
                }
            } else {
                symCodeTab[i++].prefixLen = j;
            }
        }
        symCodeTab[numSyms].prefixLen = 0;
        symCodeTab[numSyms].rangeLen = jbig2HuffmanEOT;
        if (!JBIG2HuffmanDecoder::buildTable(symCodeTab, numSyms)) {
            huff = false;
            gfree(symCodeTab);
            symCodeTab = nullptr;
        }
        huffDecoder->reset();

        // set up the arithmetic decoder
    }

    if (!huff) {
        if (!resetIntStats(symCodeLen)) {
            return false;
        }
        arithDecoder->start();
    }
    if (refine) {
        resetRefinementStats(templ, nullptr);
    }

    bitmap = readTextRegion(huff, refine, w, h, numInstances, logStrips, numSyms, symCodeTab, symCodeLen, syms, defPixel, combOp, transposed, refCorner, sOffset, huffFSTable, huffDSTable, huffDTTable, huffRDWTable, huffRDHTable,
                            huffRDXTable, huffRDYTable, huffRSizeTable, templ, atx, aty);

    if (bitmap) {
        // combine the region bitmap into the page bitmap
        if (imm) {
            if (pageH == 0xffffffff && y + h > curPageH) {
                pageBitmap->expand(y + h, pageDefPixel);
            }
            pageBitmap->combine(*bitmap, x, y, extCombOp);

            // store the region bitmap
        } else {
            bitmap->setSegNum(segNum);
            segments.push_back(std::move(bitmap));
        }
    }

    // clean up the Huffman decoder
    if (huff) {
        gfree(symCodeTab);
    }

    return true;
}

std::unique_ptr<JBIG2Bitmap> JBIG2Stream::readTextRegion(bool huff, bool refine, int w, int h, unsigned int numInstances, unsigned int logStrips, unsigned int numSyms, const JBIG2HuffmanTable *symCodeTab, unsigned int symCodeLen,
                                                         const std::vector<JBIG2Bitmap *> &syms, unsigned int defPixel, unsigned int combOp, unsigned int transposed, unsigned int refCorner, int sOffset, const JBIG2HuffmanTable *huffFSTable,
                                                         const JBIG2HuffmanTable *huffDSTable, const JBIG2HuffmanTable *huffDTTable, const JBIG2HuffmanTable *huffRDWTable, const JBIG2HuffmanTable *huffRDHTable,
                                                         const JBIG2HuffmanTable *huffRDXTable, const JBIG2HuffmanTable *huffRDYTable, const JBIG2HuffmanTable *huffRSizeTable, unsigned int templ, int *atx, int *aty)
{
    JBIG2Bitmap *symbolBitmap;
    unsigned int strips;
    int t = 0, dt = 0, tt, s, ds = 0, sFirst, j = 0;
    int rdw, rdh, rdx, rdy, ri = 0, refDX, refDY, bmSize;
    unsigned int symID, inst, bw, bh;

    strips = 1 << logStrips;

    // allocate the bitmap
    std::unique_ptr<JBIG2Bitmap> bitmap = std::make_unique<JBIG2Bitmap>(0, w, h);
    if (!bitmap->isOk()) {
        return nullptr;
    }
    if (defPixel) {
        bitmap->clearToOne();
    } else {
        bitmap->clearToZero();
    }

    // decode initial T value
    if (huff) {
        huffDecoder->decodeInt(&t, huffDTTable);
    } else {
        arithDecoder->decodeInt(&t, iadtStats);
    }

    if (checkedMultiply(t, -(int)strips, &t)) {
        return {};
    }

    inst = 0;
    sFirst = 0;
    while (inst < numInstances) {

        // decode delta-T
        if (huff) {
            huffDecoder->decodeInt(&dt, huffDTTable);
        } else {
            arithDecoder->decodeInt(&dt, iadtStats);
        }
        t += dt * strips;

        // first S value
        if (huff) {
            huffDecoder->decodeInt(&ds, huffFSTable);
        } else {
            arithDecoder->decodeInt(&ds, iafsStats);
        }
        if (unlikely(checkedAdd(sFirst, ds, &sFirst))) {
            return nullptr;
        }
        s = sFirst;

        // read the instances
        // (this loop test is here to avoid an infinite loop with damaged
        // JBIG2 streams where the normal loop exit doesn't get triggered)
        while (inst < numInstances) {