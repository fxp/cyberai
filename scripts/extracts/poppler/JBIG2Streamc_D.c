// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 36/38



            // set up the typical prediction context
            tpgrCX0 = tpgrCX1 = tpgrCX2 = 0; // make gcc happy
            if (tpgrOn) {
                refBitmap->getPixelPtr(-1 - refDX, y - 1 - refDY, &tpgrCXPtr0);
                tpgrCX0 = refBitmap->nextPixel(&tpgrCXPtr0);
                tpgrCX0 = (tpgrCX0 << 1) | refBitmap->nextPixel(&tpgrCXPtr0);
                tpgrCX0 = (tpgrCX0 << 1) | refBitmap->nextPixel(&tpgrCXPtr0);
                refBitmap->getPixelPtr(-1 - refDX, y - refDY, &tpgrCXPtr1);
                tpgrCX1 = refBitmap->nextPixel(&tpgrCXPtr1);
                tpgrCX1 = (tpgrCX1 << 1) | refBitmap->nextPixel(&tpgrCXPtr1);
                tpgrCX1 = (tpgrCX1 << 1) | refBitmap->nextPixel(&tpgrCXPtr1);
                refBitmap->getPixelPtr(-1 - refDX, y + 1 - refDY, &tpgrCXPtr2);
                tpgrCX2 = refBitmap->nextPixel(&tpgrCXPtr2);
                tpgrCX2 = (tpgrCX2 << 1) | refBitmap->nextPixel(&tpgrCXPtr2);
                tpgrCX2 = (tpgrCX2 << 1) | refBitmap->nextPixel(&tpgrCXPtr2);
            } else {
                tpgrCXPtr0.p = tpgrCXPtr1.p = tpgrCXPtr2.p = nullptr; // make gcc happy
                tpgrCXPtr0.shift = tpgrCXPtr1.shift = tpgrCXPtr2.shift = 0;
                tpgrCXPtr0.x = tpgrCXPtr1.x = tpgrCXPtr2.x = 0;
            }

            for (x = 0; x < w; ++x) {

                // update the context
                cx0 = ((cx0 << 1) | bitmap->nextPixel(&cxPtr0)) & 3;
                cx2 = ((cx2 << 1) | refBitmap->nextPixel(&cxPtr2)) & 3;
                cx3 = ((cx3 << 1) | refBitmap->nextPixel(&cxPtr3)) & 7;
                cx4 = ((cx4 << 1) | refBitmap->nextPixel(&cxPtr4)) & 7;

                if (tpgrOn) {
                    // update the typical predictor context
                    tpgrCX0 = ((tpgrCX0 << 1) | refBitmap->nextPixel(&tpgrCXPtr0)) & 7;
                    tpgrCX1 = ((tpgrCX1 << 1) | refBitmap->nextPixel(&tpgrCXPtr1)) & 7;
                    tpgrCX2 = ((tpgrCX2 << 1) | refBitmap->nextPixel(&tpgrCXPtr2)) & 7;

                    // check for a "typical" pixel
                    if (arithDecoder->decodeBit(ltpCX, refinementRegionStats.get())) {
                        ltp = !ltp;
                    }
                    if (tpgrCX0 == 0 && tpgrCX1 == 0 && tpgrCX2 == 0) {
                        bitmap->clearPixel(x, y);
                        continue;
                    }
                    if (tpgrCX0 == 7 && tpgrCX1 == 7 && tpgrCX2 == 7) {
                        bitmap->setPixel(x, y);
                        continue;
                    }
                }

                // build the context
                cx = (cx0 << 11) | (bitmap->nextPixel(&cxPtr1) << 10) | (cx2 << 8) | (cx3 << 5) | (cx4 << 2) | (bitmap->nextPixel(&cxPtr5) << 1) | refBitmap->nextPixel(&cxPtr6);

                // decode the pixel
                if ((pix = arithDecoder->decodeBit(cx, refinementRegionStats.get()))) {
                    bitmap->setPixel(x, y);
                }

                if (unlikely(arithDecoder->getReadPastEndOfStream())) {
                    return nullptr;
                }
            }
        }
    }

    return bitmap;
}

bool JBIG2Stream::readPageInfoSeg()
{
    unsigned int xRes, yRes, flags, striping;

    if (!readULong(&pageW) || !readULong(&pageH) || !readULong(&xRes) || !readULong(&yRes) || !readUByte(&flags) || !readUWord(&striping)) {
        error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
        return false;
    }
    pageDefPixel = (flags >> 2) & 1;
    defCombOp = (flags >> 3) & 3;

    // allocate the page bitmap
    if (pageH == 0xffffffff) {
        curPageH = striping & 0x7fff;
    } else {
        curPageH = pageH;
    }
    pageBitmap = std::make_unique<JBIG2Bitmap>(0, pageW, curPageH);

    if (!pageBitmap->isOk()) {
        pageBitmap.reset();
        return false;
    }

    // default pixel value
    if (pageDefPixel) {
        pageBitmap->clearToOne();
    } else {
        pageBitmap->clearToZero();
    }

    return true;
}

bool JBIG2Stream::readEndOfStripeSeg(unsigned int length)
{
    // skip the segment
    const unsigned int discardedChars = curStr->discardChars(length);
    byteCounter += discardedChars;
    return discardedChars == length;
}

bool JBIG2Stream::readProfilesSeg(unsigned int length)
{
    // skip the segment
    const unsigned int discardedChars = curStr->discardChars(length);
    byteCounter += discardedChars;
    return discardedChars == length;
}

bool JBIG2Stream::readCodeTableSeg(unsigned int segNum)
{
    JBIG2HuffmanTable *huffTab;
    unsigned int flags, oob, prefixBits, rangeBits;
    int lowVal, highVal, val;
    unsigned int huffTabSize, i;
    bool eof = false;