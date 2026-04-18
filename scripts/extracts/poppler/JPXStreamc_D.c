// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 4/31



    if (readBufLen < 8) {
        fillReadBuf();
    }
    if (readBufLen == 8) {
        c = readBuf & 0xff;
        readBufLen = 0;
    } else if (readBufLen > 8) {
        c = (readBuf >> (readBufLen - 8)) & 0xff;
        readBufLen -= 8;
    } else if (readBufLen == 0) {
        c = EOF;
    } else {
        c = (readBuf << (8 - readBufLen)) & 0xff;
        readBufLen = 0;
    }
    return c;
}

int JPXStream::lookChar()
{
    int c;

    if (readBufLen < 8) {
        fillReadBuf();
    }
    if (readBufLen == 8) {
        c = readBuf & 0xff;
    } else if (readBufLen > 8) {
        c = (readBuf >> (readBufLen - 8)) & 0xff;
    } else if (readBufLen == 0) {
        c = EOF;
    } else {
        c = (readBuf << (8 - readBufLen)) & 0xff;
    }
    return c;
}

void JPXStream::fillReadBuf()
{
    JPXTileComp *tileComp;
    unsigned int tileIdx, tx, ty;
    int pix, pixBits;

    do {
        if (curY >= img.ySize) {
            return;
        }
        tileIdx = ((curY - img.yTileOffset) / img.yTileSize) * img.nXTiles + (curX - img.xTileOffset) / img.xTileSize;
#if 1 //~ ignore the palette, assume the PDF ColorSpace object is valid
        if (img.tiles == nullptr || tileIdx >= img.nXTiles * img.nYTiles || img.tiles[tileIdx].tileComps == nullptr) {
            error(errSyntaxError, getPos(), "Unexpected tileIdx in fillReadBuf in JPX stream");
            return;
        }
        tileComp = &img.tiles[tileIdx].tileComps[curComp];
#else
        tileComp = &img.tiles[tileIdx].tileComps[havePalette ? 0 : curComp];
#endif
        tx = jpxCeilDiv((curX - img.xTileOffset) % img.xTileSize, tileComp->hSep);
        ty = jpxCeilDiv((curY - img.yTileOffset) % img.yTileSize, tileComp->vSep);
        if (unlikely(ty >= (tileComp->y1 - tileComp->y0))) {
            error(errSyntaxError, getPos(), "Unexpected ty in fillReadBuf in JPX stream");
            return;
        }
        if (unlikely(tx >= (tileComp->x1 - tileComp->x0))) {
            error(errSyntaxError, getPos(), "Unexpected tx in fillReadBuf in JPX stream");
            return;
        }
        pix = (int)tileComp->data[ty * (tileComp->x1 - tileComp->x0) + tx];
        pixBits = tileComp->prec;
#if 1 //~ ignore the palette, assume the PDF ColorSpace object is valid
        if (++curComp == img.nComps) {
#else
        if (havePalette) {
            if (pix >= 0 && pix < palette.nEntries) {
                pix = palette.c[pix * palette.nComps + curComp];
            } else {
                pix = 0;
            }
            pixBits = palette.bpc[curComp];
        }
        if (++curComp == (unsigned int)(havePalette ? palette.nComps : img.nComps)) {
#endif
            curComp = 0;
            if (++curX == img.xSize) {
                curX = img.xOffset;
                ++curY;
                if (pixBits < 8) {
                    pix <<= 8 - pixBits;
                    pixBits = 8;
                }
            }
        }
        if (pixBits == 8) {
            readBuf = (readBuf << 8) | (pix & 0xff);
        } else {
            readBuf = (readBuf << pixBits) | (pix & ((1 << pixBits) - 1));
        }
        readBufLen += pixBits;
    } while (readBufLen < 8);
}

std::optional<std::string> JPXStream::getPSFilter(int /*psLevel*/, const char * /*indent*/)
{
    return {};
}

bool JPXStream::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}

void JPXStream::getImageParams(int *bitsPerComponent, StreamColorSpaceMode *csMode, bool *hasAlpha)
{
    unsigned int boxType, boxLen, dataLen, csEnum;
    unsigned int bpc1, dummy, i;
    int csMeth, csPrec, csPrec1, dummy2;
    StreamColorSpaceMode csMode1;
    bool haveBPC, haveCSMode;