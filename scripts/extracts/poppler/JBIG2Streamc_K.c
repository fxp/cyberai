// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 11/38



            // middle bytes
            for (; xx < x1 - 8; xx += 8) {
                dest = *destPtr;
                src0 = src1;
                src1 = *srcPtr++;
                src = (((src0 << 8) | src1) >> s1) & 0xff;
                switch (combOp) {
                case 0: // or
                    dest |= src;
                    break;
                case 1: // and
                    dest &= src;
                    break;
                case 2: // xor
                    dest ^= src;
                    break;
                case 3: // xnor
                    dest ^= src ^ 0xff;
                    break;
                case 4: // replace
                    dest = src;
                    break;
                }
                *destPtr++ = dest;
            }

            // right-most byte
            // note: this last byte (src1) may not actually be used, depending
            // on the values of s1, m1, and m2 - and in fact, it may be off
            // the edge of the source bitmap, which means we need to allocate
            // one extra guard byte at the end of each bitmap
            dest = *destPtr;
            src0 = src1;
            src1 = *srcPtr++;
            src = (((src0 << 8) | src1) >> s1) & 0xff;
            switch (combOp) {
            case 0: // or
                dest |= src & m2;
                break;
            case 1: // and
                dest &= src | m1;
                break;
            case 2: // xor
                dest ^= src & m2;
                break;
            case 3: // xnor
                dest ^= (src ^ 0xff) & m2;
                break;
            case 4: // replace
                dest = (src & m2) | (dest & m1);
                break;
            }
            *destPtr = dest;
        }
    }
}

//------------------------------------------------------------------------
// JBIG2SymbolDict
//------------------------------------------------------------------------

class JBIG2SymbolDict : public JBIG2Segment
{
public:
    JBIG2SymbolDict(unsigned int segNumA, unsigned int sizeA);
    ~JBIG2SymbolDict() override;
    JBIG2SegmentType getType() const override { return jbig2SegSymbolDict; }
    unsigned int getSize() const { return bitmaps.size(); }
    void setBitmap(unsigned int idx, std::unique_ptr<JBIG2Bitmap> bitmap) { bitmaps[idx] = std::move(bitmap); }
    JBIG2Bitmap *getBitmap(unsigned int idx) { return bitmaps[idx].get(); }
    bool isOk() const { return ok; }
    void setGenericRegionStats(std::unique_ptr<JArithmeticDecoderStats> stats) { genericRegionStats = std::move(stats); }
    void setRefinementRegionStats(std::unique_ptr<JArithmeticDecoderStats> stats) { refinementRegionStats = std::move(stats); }
    JArithmeticDecoderStats *getGenericRegionStats() { return genericRegionStats.get(); }
    JArithmeticDecoderStats *getRefinementRegionStats() { return refinementRegionStats.get(); }

private:
    bool ok;
    std::vector<std::unique_ptr<JBIG2Bitmap>> bitmaps;
    std::unique_ptr<JArithmeticDecoderStats> genericRegionStats;
    std::unique_ptr<JArithmeticDecoderStats> refinementRegionStats;
};

JBIG2SymbolDict::JBIG2SymbolDict(unsigned int segNumA, unsigned int sizeA) : JBIG2Segment(segNumA)
{
    ok = sizeA <= bitmaps.max_size();
    if (ok) {
        bitmaps.resize(sizeA);
    }
}

JBIG2SymbolDict::~JBIG2SymbolDict() = default;

//------------------------------------------------------------------------
// JBIG2PatternDict
//------------------------------------------------------------------------

class JBIG2PatternDict : public JBIG2Segment
{
public:
    JBIG2PatternDict(unsigned int segNumA, unsigned int sizeA);
    ~JBIG2PatternDict() override;
    JBIG2SegmentType getType() const override { return jbig2SegPatternDict; }
    unsigned int getSize() const { return bitmaps.size(); }
    void setBitmap(unsigned int idx, std::unique_ptr<JBIG2Bitmap> bitmap)
    {
        if (likely(idx < bitmaps.size())) {
            bitmaps[idx] = std::move(bitmap);
        }
    }
    JBIG2Bitmap *getBitmap(unsigned int idx) { return (idx < bitmaps.size()) ? bitmaps[idx].get() : nullptr; }

private:
    std::vector<std::unique_ptr<JBIG2Bitmap>> bitmaps;
};

JBIG2PatternDict::JBIG2PatternDict(unsigned int segNumA, unsigned int sizeA) : JBIG2Segment(segNumA)
{
    if (sizeA > bitmaps.max_size()) {
        error(errSyntaxError, -1, "JBIG2PatternDict: can't allocate bitmaps");
        return;
    }
    bitmaps.resize(sizeA);
}

JBIG2PatternDict::~JBIG2PatternDict() = default;

//------------------------------------------------------------------------
// JBIG2CodeTable
//------------------------------------------------------------------------

class JBIG2CodeTable : public JBIG2Segment
{
public:
    JBIG2CodeTable(unsigned int segNumA, JBIG2HuffmanTable *tableA);
    ~JBIG2CodeTable() override;
    JBIG2SegmentType getType() const override { return jbig2SegCodeTable; }
    JBIG2HuffmanTable *getHuffTable() { return table; }