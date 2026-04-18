// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 24/38



bool JBIG2Stream::readPatternDictSeg(unsigned int segNum, unsigned int length)
{
    std::unique_ptr<JBIG2PatternDict> patternDict;
    std::unique_ptr<JBIG2Bitmap> bitmap;
    unsigned int flags, patternW, patternH, grayMax, templ, mmr;
    int atx[4], aty[4];
    unsigned int i, x;

    // halftone dictionary flags, pattern width and height, max gray value
    if (!readUByte(&flags) || !readUByte(&patternW) || !readUByte(&patternH) || !readULong(&grayMax)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }
    templ = (flags >> 1) & 3;
    mmr = flags & 1;

    // set up the arithmetic decoder
    if (!mmr) {
        resetGenericStats(templ, nullptr);
        arithDecoder->start();
    }

    // read the bitmap
    atx[0] = -(int)patternW;
    aty[0] = 0;
    atx[1] = -3;
    aty[1] = -1;
    atx[2] = 2;
    aty[2] = -2;
    atx[3] = -2;
    aty[3] = -2;

    unsigned int grayMaxPlusOne;
    if (unlikely(checkedAdd(grayMax, 1U, &grayMaxPlusOne))) {
        return false;
    }
    unsigned int bitmapW;
    if (unlikely(checkedMultiply(grayMaxPlusOne, patternW, &bitmapW))) {
        return false;
    }
    if (bitmapW >= INT_MAX) {
        return false;
    }
    bitmap = readGenericBitmap(mmr, static_cast<int>(bitmapW), patternH, templ, false, false, nullptr, atx, aty, length - 7);

    if (!bitmap) {
        return false;
    }

    // create the pattern dict object
    patternDict = std::make_unique<JBIG2PatternDict>(segNum, grayMax + 1);

    // split up the bitmap
    x = 0;
    for (i = 0; i <= grayMax && i < patternDict->getSize(); ++i) {
        patternDict->setBitmap(i, bitmap->getSlice(x, 0, patternW, patternH));
        x += patternW;
    }

    // store the new pattern dict
    segments.push_back(std::move(patternDict));

    return true;
}

bool JBIG2Stream::readHalftoneRegionSeg(unsigned int segNum, bool imm, const std::vector<unsigned int> &refSegs)
{
    std::unique_ptr<JBIG2Bitmap> bitmap;
    JBIG2Segment *seg;
    JBIG2PatternDict *patternDict;
    std::unique_ptr<JBIG2Bitmap> skipBitmap;
    unsigned int *grayImg;
    JBIG2Bitmap *patternBitmap;
    unsigned int w, h, x, y, segInfoFlags, extCombOp;
    unsigned int flags, mmr, templ, enableSkip, combOp;
    unsigned int gridW, gridH, stepX, stepY, patW, patH;
    int atx[4], aty[4];
    int gridX, gridY, xx, yy, j;
    unsigned int bpp, m, n, i;

    // region segment info field
    if (!readULong(&w) || !readULong(&h) || !readULong(&x) || !readULong(&y) || !readUByte(&segInfoFlags)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }
    extCombOp = segInfoFlags & 7;

    // rest of the halftone region header
    if (!readUByte(&flags)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }
    mmr = flags & 1;
    templ = (flags >> 1) & 3;
    enableSkip = (flags >> 3) & 1;
    combOp = (flags >> 4) & 7;
    if (!readULong(&gridW) || !readULong(&gridH) || !readLong(&gridX) || !readLong(&gridY) || !readUWord(&stepX) || !readUWord(&stepY)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }
    if (w == 0 || h == 0 || w >= INT_MAX / h) {
        error(errSyntaxError, curStr->getPos(), "Bad bitmap size in JBIG2 halftone segment");
        return false;
    }
    if (gridH == 0 || gridW >= INT_MAX / gridH) {
        error(errSyntaxError, curStr->getPos(), "Bad grid size in JBIG2 halftone segment");
        return false;
    }

    // get pattern dictionary
    if (refSegs.size() != 1) {
        error(errSyntaxError, curStr->getPos(), "Bad symbol dictionary reference in JBIG2 halftone segment");
        return false;
    }
    seg = findSegment(refSegs[0]);
    if (seg == nullptr || seg->getType() != jbig2SegPatternDict) {
        error(errSyntaxError, curStr->getPos(), "Bad symbol dictionary reference in JBIG2 halftone segment");
        return false;
    }

    patternDict = static_cast<JBIG2PatternDict *>(seg);
    i = patternDict->getSize();
    if (i <= 1) {
        bpp = 0;
    } else {
        --i;
        bpp = 0;
        // i = floor((size-1) / 2^bpp)
        while (i > 0) {
            ++bpp;
            i >>= 1;
        }
    }
    patternBitmap = patternDict->getBitmap(0);
    if (unlikely(patternBitmap == nullptr)) {
        error(errSyntaxError, curStr->getPos(), "Bad pattern bitmap");
        return false;
    }
    patW = patternBitmap->getWidth();
    patH = patternBitmap->getHeight();

    // set up the arithmetic decoder
    if (!mmr) {
        resetGenericStats(templ, nullptr);
        arithDecoder->start();
    }

    // allocate the bitmap
    bitmap = std::make_unique<JBIG2Bitmap>(segNum, w, h);
    if (flags & 0x80) { // HDEFPIXEL
        bitmap->clearToOne();
    } else {
        bitmap->clearToZero();
    }