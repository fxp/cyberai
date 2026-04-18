// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 25/36



    // read headers
    doScan = false;
    while (!doScan) {
        c = readMarker();
        switch (c) {
        case 0xc0: // SOF0 (sequential)
        case 0xc1: // SOF1 (extended sequential)
            if (!readBaselineSOF()) {
                return false;
            }
            break;
        case 0xc2: // SOF2 (progressive)
            if (!readProgressiveSOF()) {
                return false;
            }
            break;
        case 0xc4: // DHT
            if (!readHuffmanTables()) {
                return false;
            }
            break;
        case 0xd8: // SOI
            break;
        case 0xd9: // EOI
            return false;
        case 0xda: // SOS
            if (!readScanInfo()) {
                return false;
            }
            doScan = true;
            break;
        case 0xdb: // DQT
            if (!readQuantTables()) {
                return false;
            }
            break;
        case 0xdd: // DRI
            if (!readRestartInterval()) {
                return false;
            }
            break;
        case 0xe0: // APP0
            if (!readJFIFMarker()) {
                return false;
            }
            break;
        case 0xee: // APP14
            if (!readAdobeMarker()) {
                return false;
            }
            break;
        case EOF:
            error(errSyntaxError, getPos(), "Bad DCT header");
            return false;
        default:
            // skip APPn / COM / etc.
            if (c >= 0xe0) {
                n = read16() - 2;
                for (i = 0; i < n; ++i) {
                    str->getChar();
                }
            } else {
                error(errSyntaxError, getPos(), "Unknown DCT marker <{0:02x}>", c);
                return false;
            }
            break;
        }
    }

    return true;
}

bool DCTStream::readBaselineSOF()
{
    int prec;
    int i;
    int c;

    // read the length
    (void)read16();
    prec = str->getChar();
    height = read16();
    width = read16();
    numComps = str->getChar();
    if (numComps <= 0 || numComps > 4) {
        error(errSyntaxError, getPos(), "Bad number of components in DCT stream");
        numComps = 0;
        return false;
    }
    if (prec != 8) {
        error(errSyntaxError, getPos(), "Bad DCT precision {0:d}", prec);
        return false;
    }
    for (i = 0; i < numComps; ++i) {
        compInfo[i].id = str->getChar();
        c = str->getChar();
        compInfo[i].hSample = (c >> 4) & 0x0f;
        compInfo[i].vSample = c & 0x0f;
        compInfo[i].quantTable = str->getChar();
        if (compInfo[i].hSample < 1 || compInfo[i].hSample > 4 || compInfo[i].vSample < 1 || compInfo[i].vSample > 4) {
            error(errSyntaxError, getPos(), "Bad DCT sampling factor");
            return false;
        }
        if (compInfo[i].quantTable < 0 || compInfo[i].quantTable > 3) {
            error(errSyntaxError, getPos(), "Bad DCT quant table selector");
            return false;
        }
    }
    progressive = false;
    return true;
}

bool DCTStream::readProgressiveSOF()
{
    int prec;
    int i;
    int c;

    // read the length
    (void)read16();
    prec = str->getChar();
    height = read16();
    width = read16();
    numComps = str->getChar();

    if (numComps <= 0 || numComps > 4) {
        error(errSyntaxError, getPos(), "Bad number of components in DCT stream");
        numComps = 0;
        return false;
    }
    if (prec != 8) {
        error(errSyntaxError, getPos(), "Bad DCT precision {0:d}", prec);
        return false;
    }
    for (i = 0; i < numComps; ++i) {
        compInfo[i].id = str->getChar();
        c = str->getChar();
        compInfo[i].hSample = (c >> 4) & 0x0f;
        compInfo[i].vSample = c & 0x0f;
        compInfo[i].quantTable = str->getChar();
        if (compInfo[i].hSample < 1 || compInfo[i].hSample > 4 || compInfo[i].vSample < 1 || compInfo[i].vSample > 4) {
            error(errSyntaxError, getPos(), "Bad DCT sampling factor");
            return false;
        }
        if (compInfo[i].quantTable < 0 || compInfo[i].quantTable > 3) {
            error(errSyntaxError, getPos(), "Bad DCT quant table selector");
            return false;
        }
    }
    progressive = true;
    return true;
}

bool DCTStream::readScanInfo()
{
    int length;
    int id, c;
    int i, j;