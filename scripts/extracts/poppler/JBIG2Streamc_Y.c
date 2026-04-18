// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 25/38



    // compute the skip bitmap
    if (enableSkip) {
        skipBitmap = std::make_unique<JBIG2Bitmap>(0, gridW, gridH);
        skipBitmap->clearToZero();
        for (m = 0; m < gridH; ++m) {
            for (n = 0; n < gridW; ++n) {
                xx = gridX + m * stepY + n * stepX;
                yy = gridY + m * stepX - n * stepY;
                if (((xx + (int)patW) >> 8) <= 0 || (xx >> 8) >= (int)w || ((yy + (int)patH) >> 8) <= 0 || (yy >> 8) >= (int)h) {
                    skipBitmap->setPixel(n, m);
                }
            }
        }
    }

    // read the gray-scale image
    grayImg = (unsigned int *)gmallocn_checkoverflow(gridW * gridH, sizeof(unsigned int));
    if (!grayImg) {
        return false;
    }
    memset(grayImg, 0, gridW * gridH * sizeof(unsigned int));
    atx[0] = templ <= 1 ? 3 : 2;
    aty[0] = -1;
    atx[1] = -3;
    aty[1] = -1;
    atx[2] = 2;
    aty[2] = -2;
    atx[3] = -2;
    aty[3] = -2;
    for (j = bpp - 1; j >= 0; --j) {
        const std::unique_ptr<JBIG2Bitmap> grayBitmap = readGenericBitmap(mmr, gridW, gridH, templ, false, enableSkip, skipBitmap.get(), atx, aty, -1);
        if (!grayBitmap) {
            gfree(grayImg);
            return false;
        }
        i = 0;
        for (m = 0; m < gridH; ++m) {
            for (n = 0; n < gridW; ++n) {
                const int bit = grayBitmap->getPixel(n, m) ^ (grayImg[i] & 1);
                grayImg[i] = (grayImg[i] << 1) | bit;
                ++i;
            }
        }
    }

    // decode the image
    i = 0;
    for (m = 0; m < gridH; ++m) {
        xx = gridX + m * stepY;
        yy = gridY + m * stepX;
        for (n = 0; n < gridW; ++n) {
            if (!(enableSkip && skipBitmap->getPixel(n, m))) {
                patternBitmap = patternDict->getBitmap(grayImg[i]);
                if (unlikely(patternBitmap == nullptr)) {
                    gfree(grayImg);
                    error(errSyntaxError, curStr->getPos(), "Bad pattern bitmap");
                    return false;
                }
                bitmap->combine(*patternBitmap, xx >> 8, yy >> 8, combOp);
            }
            xx += stepX;
            yy -= stepY;
            ++i;
        }
    }

    gfree(grayImg);

    // combine the region bitmap into the page bitmap
    if (imm) {
        if (pageH == 0xffffffff && y + h > curPageH) {
            pageBitmap->expand(y + h, pageDefPixel);
        }
        pageBitmap->combine(*bitmap, x, y, extCombOp);

        // store the region bitmap
    } else {
        segments.push_back(std::move(bitmap));
    }

    return true;
}

bool JBIG2Stream::readGenericRegionSeg(unsigned int segNum, bool imm, unsigned int length)
{
    std::unique_ptr<JBIG2Bitmap> bitmap;
    unsigned int w, h, x, y, segInfoFlags, extCombOp, rowCount;
    unsigned int flags, mmr, templ, tpgdOn;
    int atx[4], aty[4];

    // region segment info field
    if (!readULong(&w) || !readULong(&h) || !readULong(&x) || !readULong(&y) || !readUByte(&segInfoFlags)) {
        return false;
    }
    extCombOp = segInfoFlags & 7;

    // rest of the generic region segment header
    if (!readUByte(&flags)) {
        return false;
    }
    mmr = flags & 1;
    templ = (flags >> 1) & 3;
    tpgdOn = (flags >> 3) & 1;

    // AT flags
    if (!mmr) {
        if (templ == 0) {
            if (!readByte(&atx[0]) || !readByte(&aty[0]) || !readByte(&atx[1]) || !readByte(&aty[1]) || !readByte(&atx[2]) || !readByte(&aty[2]) || !readByte(&atx[3]) || !readByte(&aty[3])) {
                return false;
            }
        } else {
            if (!readByte(&atx[0]) || !readByte(&aty[0])) {
                return false;
            }
        }
    }

    // set up the arithmetic decoder
    if (!mmr) {
        resetGenericStats(templ, nullptr);
        arithDecoder->start();
    }

    // read the bitmap
    bitmap = readGenericBitmap(mmr, w, h, templ, tpgdOn, false, nullptr, atx, aty, mmr ? length - 18 : 0);
    if (!bitmap) {
        return false;
    }

    // combine the region bitmap into the page bitmap
    if (imm) {
        if (pageH == 0xffffffff && y + h > curPageH) {
            pageBitmap->expand(y + h, pageDefPixel);
            if (!pageBitmap->isOk()) {
                return false;
            }
        }
        pageBitmap->combine(*bitmap, x, y, extCombOp);

        // store the region bitmap
    } else {
        bitmap->setSegNum(segNum);
        segments.push_back(std::move(bitmap));
    }

    // immediate generic segments can have an unspecified length, in
    // which case, a row count is stored at the end of the segment
    if (imm && length == 0xffffffff) {
        if (!readULong(&rowCount)) {
            return false;
        }
    }

    return true;
}