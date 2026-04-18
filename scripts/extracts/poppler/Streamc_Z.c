// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 26/36



    length = read16() - 2;
    scanInfo.numComps = str->getChar();
    if (scanInfo.numComps <= 0 || scanInfo.numComps > 4) {
        error(errSyntaxError, getPos(), "Bad number of components in DCT stream");
        scanInfo.numComps = 0;
        return false;
    }
    --length;
    if (length != 2 * scanInfo.numComps + 3) {
        error(errSyntaxError, getPos(), "Bad DCT scan info block");
        return false;
    }
    interleaved = scanInfo.numComps == numComps;
    for (j = 0; j < numComps; ++j) {
        scanInfo.comp[j] = false;
        scanInfo.dcHuffTable[j] = 0;
        scanInfo.acHuffTable[j] = 0;
    }
    for (i = 0; i < scanInfo.numComps; ++i) {
        id = str->getChar();
        // some (broken) DCT streams reuse ID numbers, but at least they
        // keep the components in order, so we check compInfo[i] first to
        // work around the problem
        if (id == compInfo[i].id) {
            j = i;
        } else {
            for (j = 0; j < numComps; ++j) {
                if (id == compInfo[j].id) {
                    break;
                }
            }
            if (j == numComps) {
                error(errSyntaxError, getPos(), "Bad DCT component ID in scan info block");
                return false;
            }
        }
        scanInfo.comp[j] = true;
        c = str->getChar();
        scanInfo.dcHuffTable[j] = (c >> 4) & 0x0f;
        scanInfo.acHuffTable[j] = c & 0x0f;
    }
    scanInfo.firstCoeff = str->getChar();
    scanInfo.lastCoeff = str->getChar();
    if (scanInfo.firstCoeff < 0 || scanInfo.lastCoeff > 63 || scanInfo.firstCoeff > scanInfo.lastCoeff) {
        error(errSyntaxError, getPos(), "Bad DCT coefficient numbers in scan info block");
        return false;
    }
    c = str->getChar();
    scanInfo.ah = (c >> 4) & 0x0f;
    scanInfo.al = c & 0x0f;
    return true;
}

bool DCTStream::readQuantTables()
{
    int length, prec, i, index;

    length = read16() - 2;
    while (length > 0) {
        index = str->getChar();
        prec = (index >> 4) & 0x0f;
        index &= 0x0f;
        if (prec > 1 || index >= 4) {
            error(errSyntaxError, getPos(), "Bad DCT quantization table");
            return false;
        }
        if (index == numQuantTables) {
            numQuantTables = index + 1;
        }
        for (i = 0; i < 64; ++i) {
            if (prec) {
                quantTables[index][dctZigZag[i]] = read16();
            } else {
                quantTables[index][dctZigZag[i]] = str->getChar();
            }
        }
        if (prec) {
            length -= 129;
        } else {
            length -= 65;
        }
    }
    return true;
}

bool DCTStream::readHuffmanTables()
{
    DCTHuffTable *tbl;
    int length;
    int index;
    unsigned short code;
    unsigned char sym;
    int i;
    int c;

    length = read16() - 2;
    while (length > 0) {
        index = str->getChar();
        --length;
        if ((index & 0x0f) >= 4) {
            error(errSyntaxError, getPos(), "Bad DCT Huffman table");
            return false;
        }
        if (index & 0x10) {
            index &= 0x0f;
            if (index >= numACHuffTables)
                numACHuffTables = index + 1;
            tbl = &acHuffTables[index];
        } else {
            index &= 0x0f;
            if (index >= numDCHuffTables)
                numDCHuffTables = index + 1;
            tbl = &dcHuffTables[index];
        }
        sym = 0;
        code = 0;
        for (i = 1; i <= 16; ++i) {
            c = str->getChar();
            tbl->firstSym[i] = sym;
            tbl->firstCode[i] = code;
            tbl->numCodes[i] = c;
            sym += c;
            code = (code + c) << 1;
        }
        length -= 16;
        for (i = 0; i < sym; ++i)
            tbl->sym[i] = str->getChar();
        length -= sym;
    }
    return true;
}

bool DCTStream::readRestartInterval()
{
    int length;

    length = read16();
    if (length != 4) {
        error(errSyntaxError, getPos(), "Bad DCT restart interval");
        return false;
    }
    restartInterval = read16();
    return true;
}

bool DCTStream::readJFIFMarker()
{
    int length, i;
    char buf[5];
    int c;

    length = read16();
    length -= 2;
    if (length >= 5) {
        for (i = 0; i < 5; ++i) {
            if ((c = str->getChar()) == EOF) {
                error(errSyntaxError, getPos(), "Bad DCT APP0 marker");
                return false;
            }
            buf[i] = c;
        }
        length -= 5;
        if (!memcmp(buf, "JFIF\0", 5)) {
            gotJFIFMarker = true;
        }
    }
    while (length > 0) {
        if (str->getChar() == EOF) {
            error(errSyntaxError, getPos(), "Bad DCT APP0 marker");
            return false;
        }
        --length;
    }
    return true;
}

bool DCTStream::readAdobeMarker()
{
    int length, i;
    char buf[12];
    int c;