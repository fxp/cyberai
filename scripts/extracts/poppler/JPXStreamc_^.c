// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 30/31



            // unsigned: inverse DC level shift and clip
        } else {
            cover(90);
            maxVal = (1 << tileComp->prec) - 1;
            zeroVal = 1 << (tileComp->prec - 1);
            dataPtr = tileComp->data;
            for (y = 0; y < tileComp->y1 - tileComp->y0; ++y) {
                for (x = 0; x < tileComp->x1 - tileComp->x0; ++x) {
                    coeff = *dataPtr;
                    if (tileComp->transform == 0) {
                        cover(112);
                        coeff >>= fracBits;
                    }
                    coeff += zeroVal;
                    if (coeff < 0) {
                        cover(113);
                        coeff = 0;
                    } else if (coeff > maxVal) {
                        cover(114);
                        coeff = maxVal;
                    }
                    *dataPtr++ = coeff;
                }
            }
        }
    }

    return true;
}

bool JPXStream::readBoxHdr(unsigned int *boxType, unsigned int *boxLen, unsigned int *dataLen)
{
    unsigned int len, lenH;

    if (!readULong(&len) || !readULong(boxType)) {
        return false;
    }
    if (len == 1) {
        if (!readULong(&lenH) || !readULong(&len)) {
            return false;
        }
        if (lenH) {
            error(errSyntaxError, getPos(), "JPX stream contains a box larger than 2^32 bytes");
            return false;
        }
        *boxLen = len;
        *dataLen = len - 16;
    } else if (len == 0) {
        *boxLen = 0;
        *dataLen = 0;
    } else {
        *boxLen = len;
        *dataLen = len - 8;
    }
    return true;
}

int JPXStream::readMarkerHdr(int *segType, unsigned int *segLen)
{
    int c;

    do {
        do {
            if ((c = bufStr->getChar()) == EOF) {
                return false;
            }
        } while (c != 0xff);
        do {
            if ((c = bufStr->getChar()) == EOF) {
                return false;
            }
        } while (c == 0xff);
    } while (c == 0x00);
    *segType = c;
    if ((c >= 0x30 && c <= 0x3f) || c == 0x4f || c == 0x92 || c == 0x93 || c == 0xd9) {
        *segLen = 0;
        return true;
    }
    return readUWord(segLen);
}

bool JPXStream::readUByte(unsigned int *x)
{
    int c0;

    if ((c0 = bufStr->getChar()) == EOF) {
        return false;
    }
    *x = (unsigned int)c0;
    return true;
}

bool JPXStream::readByte(int *x)
{
    int c0;

    if ((c0 = bufStr->getChar()) == EOF) {
        return false;
    }
    *x = c0;
    if (c0 & 0x80) {
        *x |= -1 - 0xff;
    }
    return true;
}

bool JPXStream::readUWord(unsigned int *x)
{
    int c0, c1;

    if ((c0 = bufStr->getChar()) == EOF || (c1 = bufStr->getChar()) == EOF) {
        return false;
    }
    *x = (unsigned int)((c0 << 8) | c1);
    return true;
}

bool JPXStream::readULong(unsigned int *x)
{
    int c0, c1, c2, c3;

    if ((c0 = bufStr->getChar()) == EOF || (c1 = bufStr->getChar()) == EOF || (c2 = bufStr->getChar()) == EOF || (c3 = bufStr->getChar()) == EOF) {
        return false;
    }
    *x = (unsigned int)((c0 << 24) | (c1 << 16) | (c2 << 8) | c3);
    return true;
}

bool JPXStream::readNBytes(int nBytes, bool signd, int *x)
{
    int y, c, i;

    y = 0;
    for (i = 0; i < nBytes; ++i) {
        if ((c = bufStr->getChar()) == EOF) {
            return false;
        }
        y = (y << 8) + c;
    }
    if (signd) {
        if (y & (1 << (8 * nBytes - 1))) {
            y |= -1 << (8 * nBytes);
        }
    }
    *x = y;
    return true;
}

void JPXStream::startBitBuf(unsigned int byteCountA)
{
    bitBufLen = 0;
    bitBufSkip = false;
    byteCount = byteCountA;
}

bool JPXStream::readBits(int nBits, unsigned int *x)
{
    int c;

    while (bitBufLen < nBits) {
        if (byteCount == 0 || (c = bufStr->getChar()) == EOF) {
            return false;
        }
        --byteCount;
        if (bitBufSkip) {
            bitBuf = (bitBuf << 7) | (c & 0x7f);
            bitBufLen += 7;
        } else {
            bitBuf = (bitBuf << 8) | (c & 0xff);
            bitBufLen += 8;
        }
        bitBufSkip = c == 0xff;
    }
    *x = (bitBuf >> (bitBufLen - nBits)) & ((1 << nBits) - 1);
    bitBufLen -= nBits;
    return true;
}

void JPXStream::skipSOP()
{
    int i;

    // SOP occurs at the start of the packet header, so we don't need to
    // worry about bit-stuff prior to it
    if (byteCount >= 6 && bufStr->lookChar(0) == 0xff && bufStr->lookChar(1) == 0x91) {
        for (i = 0; i < 6; ++i) {
            bufStr->getChar();
        }
        byteCount -= 6;
        bitBufLen = 0;
        bitBufSkip = false;
    }
}

void JPXStream::skipEPH()
{
    int i, k;

    k = bitBufSkip ? 1 : 0;
    if (byteCount >= (unsigned int)(k + 2) && bufStr->lookChar(k) == 0xff && bufStr->lookChar(k + 1) == 0x92) {
        for (i = 0; i < k + 2; ++i) {
            bufStr->getChar();
        }
        byteCount -= k + 2;
        bitBufLen = 0;
        bitBufSkip = false;
    }
}