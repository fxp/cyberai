// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 34/38



    // region segment info field
    if (!readULong(&w) || !readULong(&h) || !readULong(&x) || !readULong(&y) || !readUByte(&segInfoFlags)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }
    const unsigned int extCombOp = segInfoFlags & 7;

    // rest of the generic refinement region segment header
    if (!readUByte(&flags)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }
    const unsigned int templ = flags & 1;
    const unsigned int tpgrOn = (flags >> 1) & 1;

    // AT flags
    if (!templ) {
        if (!readByte(&atx[0]) || !readByte(&aty[0]) || !readByte(&atx[1]) || !readByte(&aty[1])) {
            error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
            return false;
        }
    }

    // resize the page bitmap if needed
    if (refSegs.empty() || imm) {
        if (pageH == 0xffffffff && y + h > curPageH) {
            pageBitmap->expand(y + h, pageDefPixel);
        }
    }

    // get referenced bitmap
    if (refSegs.size() > 1) {
        error(errSyntaxError, curStr->getPos(), "Bad reference in JBIG2 generic refinement segment");
        return false;
    }
    JBIG2Bitmap *refBitmap;
    std::unique_ptr<JBIG2Bitmap> bitmapDeleter;
    if (refSegs.size() == 1) {
        JBIG2Segment *seg = findSegment(refSegs[0]);
        if (seg == nullptr || seg->getType() != jbig2SegBitmap) {
            error(errSyntaxError, curStr->getPos(), "Bad bitmap reference in JBIG2 generic refinement segment");
            return false;
        }
        refBitmap = static_cast<JBIG2Bitmap *>(seg);
    } else {
        bitmapDeleter = pageBitmap->getSlice(x, y, w, h);
        refBitmap = bitmapDeleter.get();
    }

    // set up the arithmetic decoder
    resetRefinementStats(templ, nullptr);
    arithDecoder->start();

    // read
    std::unique_ptr<JBIG2Bitmap> bitmap = readGenericRefinementRegion(w, h, templ, tpgrOn, refBitmap, 0, 0, atx, aty);

    // combine the region bitmap into the page bitmap
    if (imm && bitmap) {
        pageBitmap->combine(*bitmap, x, y, extCombOp);

        // store the region bitmap
    } else {
        if (bitmap) {
            bitmap->setSegNum(segNum);
            segments.push_back(std::move(bitmap));
        } else {
            error(errSyntaxError, curStr->getPos(), "readGenericRefinementRegionSeg with null bitmap");
        }
    }

    // delete the referenced bitmap
    if (refSegs.size() == 1) {
        discardSegment(refSegs[0]);
    }

    return true;
}

std::unique_ptr<JBIG2Bitmap> JBIG2Stream::readGenericRefinementRegion(int w, int h, int templ, bool tpgrOn, JBIG2Bitmap *refBitmap, int refDX, int refDY, int *atx, int *aty)
{
    bool ltp;
    unsigned int ltpCX, cx, cx0, cx2, cx3, cx4, tpgrCX0, tpgrCX1, tpgrCX2;
    JBIG2BitmapPtr cxPtr0 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr cxPtr1 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr cxPtr2 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr cxPtr3 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr cxPtr4 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr cxPtr5 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr cxPtr6 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr tpgrCXPtr0 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr tpgrCXPtr1 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr tpgrCXPtr2 = { .p = nullptr, .shift = 0, .x = 0 };
    int x, y, pix;

    if (!refBitmap) {
        return nullptr;
    }

    auto bitmap = std::make_unique<JBIG2Bitmap>(0, w, h);
    if (!bitmap->isOk()) {
        return nullptr;
    }
    bitmap->clearToZero();

    // set up the typical row context
    if (templ) {
        ltpCX = 0x008;
    } else {
        ltpCX = 0x0010;
    }

    ltp = false;
    for (y = 0; y < h; ++y) {

        if (templ) {

            // set up the context
            bitmap->getPixelPtr(0, y - 1, &cxPtr0);
            cx0 = bitmap->nextPixel(&cxPtr0);
            bitmap->getPixelPtr(-1, y, &cxPtr1);
            refBitmap->getPixelPtr(-refDX, y - 1 - refDY, &cxPtr2);
            refBitmap->getPixelPtr(-1 - refDX, y - refDY, &cxPtr3);
            cx3 = refBitmap->nextPixel(&cxPtr3);
            cx3 = (cx3 << 1) | refBitmap->nextPixel(&cxPtr3);
            refBitmap->getPixelPtr(-refDX, y + 1 - refDY, &cxPtr4);
            cx4 = refBitmap->nextPixel(&cxPtr4);