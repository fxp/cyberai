// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 37/38



    if (!readUByte(&flags) || !readLong(&lowVal) || !readLong(&highVal)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }
    oob = flags & 1;
    prefixBits = ((flags >> 1) & 7) + 1;
    rangeBits = ((flags >> 4) & 7) + 1;

    huffDecoder->reset();
    huffTabSize = 8;
    huffTab = (JBIG2HuffmanTable *)gmallocn_checkoverflow(huffTabSize, sizeof(JBIG2HuffmanTable));
    if (unlikely(!huffTab)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }

    i = 0;
    val = lowVal;
    while (val < highVal) {
        if (i == huffTabSize) {
            huffTabSize *= 2;
            huffTab = (JBIG2HuffmanTable *)greallocn_checkoverflow(huffTab, huffTabSize, sizeof(JBIG2HuffmanTable));
            if (unlikely(!huffTab)) {
                error(errInternal, curStr->getPos(), "Failed allocation when processing JBIG2 stream");
                return false;
            }
        }
        huffTab[i].val = val;
        huffTab[i].prefixLen = huffDecoder->readBits(prefixBits, &eof);
        huffTab[i].rangeLen = huffDecoder->readBits(rangeBits, &eof);

        const int shiftBits = huffTab[i].rangeLen % intNBits;

        if (eof || unlikely(checkedAdd(val, 1 << shiftBits, &val))) {
            free(huffTab);
            return false;
        }
        ++i;
    }
    if (i + oob + 3 > huffTabSize) {
        huffTabSize = i + oob + 3;
        huffTab = (JBIG2HuffmanTable *)greallocn_checkoverflow(huffTab, huffTabSize, sizeof(JBIG2HuffmanTable));
        if (unlikely(!huffTab)) {
            error(errInternal, curStr->getPos(), "Failed allocation when processing JBIG2 stream");
            return false;
        }
    }
    huffTab[i].val = lowVal - 1;
    huffTab[i].prefixLen = huffDecoder->readBits(prefixBits);
    huffTab[i].rangeLen = jbig2HuffmanLOW;
    ++i;
    huffTab[i].val = highVal;
    huffTab[i].prefixLen = huffDecoder->readBits(prefixBits);
    huffTab[i].rangeLen = 32;
    ++i;
    if (oob) {
        huffTab[i].val = 0;
        huffTab[i].prefixLen = huffDecoder->readBits(prefixBits);
        huffTab[i].rangeLen = jbig2HuffmanOOB;
        ++i;
    }
    huffTab[i].val = 0;
    huffTab[i].prefixLen = 0;
    huffTab[i].rangeLen = jbig2HuffmanEOT;
    if (JBIG2HuffmanDecoder::buildTable(huffTab, i)) {
        // create and store the new table segment
        segments.push_back(std::make_unique<JBIG2CodeTable>(segNum, huffTab));
    } else {
        free(huffTab);
    }

    return true;
}

bool JBIG2Stream::readExtensionSeg(unsigned int length)
{
    // skip the segment
    const unsigned int discardedChars = curStr->discardChars(length);
    byteCounter += discardedChars;
    return discardedChars == length;
}

JBIG2Segment *JBIG2Stream::findSegment(unsigned int segNum)
{
    for (std::unique_ptr<JBIG2Segment> &seg : globalSegments) {
        if (seg->getSegNum() == segNum) {
            return seg.get();
        }
    }
    for (std::unique_ptr<JBIG2Segment> &seg : segments) {
        if (seg->getSegNum() == segNum) {
            return seg.get();
        }
    }
    return nullptr;
}

void JBIG2Stream::discardSegment(unsigned int segNum)
{
    for (auto it = globalSegments.begin(); it != globalSegments.end(); ++it) {
        if ((*it)->getSegNum() == segNum) {
            globalSegments.erase(it);
            return;
        }
    }
    for (auto it = segments.begin(); it != segments.end(); ++it) {
        if ((*it)->getSegNum() == segNum) {
            segments.erase(it);
            return;
        }
    }
}

void JBIG2Stream::resetGenericStats(unsigned int templ, const JArithmeticDecoderStats *prevStats)
{
    int size;

    size = contextSize[templ];
    if (prevStats && prevStats->getContextSize() == size) {
        if (genericRegionStats->getContextSize() == size) {
            genericRegionStats->copyFrom(*prevStats);
        } else {
            genericRegionStats = prevStats->copy();
        }
    } else {
        if (genericRegionStats->getContextSize() == size) {
            genericRegionStats->resetContext();
        } else {
            genericRegionStats = std::make_unique<JArithmeticDecoderStats>(1 << size);
        }
    }
}

void JBIG2Stream::resetRefinementStats(unsigned int templ, const JArithmeticDecoderStats *prevStats)
{
    int size;

    size = refContextSize[templ];
    if (prevStats && prevStats->getContextSize() == size) {
        if (refinementRegionStats->getContextSize() == size) {
            refinementRegionStats->copyFrom(*prevStats);
        } else {
            refinementRegionStats = prevStats->copy();
        }
    } else {
        if (refinementRegionStats->getContextSize() == size) {
            refinementRegionStats->resetContext();
        } else {
            refinementRegionStats = std::make_unique<JArithmeticDecoderStats>(1 << size);
        }
    }
}