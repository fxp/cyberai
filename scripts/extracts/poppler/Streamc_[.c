// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 27/36



    length = read16();
    if (length < 14) {
        goto err;
    }
    for (i = 0; i < 12; ++i) {
        if ((c = str->getChar()) == EOF) {
            goto err;
        }
        buf[i] = c;
    }
    if (strncmp(buf, "Adobe", 5)) {
        goto err;
    }
    colorXform = buf[11];
    gotAdobeMarker = true;
    for (i = 14; i < length; ++i) {
        if (str->getChar() == EOF) {
            goto err;
        }
    }
    return true;

err:
    error(errSyntaxError, getPos(), "Bad DCT Adobe APP14 marker");
    return false;
}

bool DCTStream::readTrailer()
{
    int c;

    c = readMarker();
    if (c != 0xd9) { // EOI
        error(errSyntaxError, getPos(), "Bad DCT trailer");
        return false;
    }
    return true;
}

int DCTStream::readMarker()
{
    int c;

    do {
        do {
            c = str->getChar();
        } while (c != 0xff && c != EOF);
        while (c == 0xff) {
            c = str->getChar();
        }
    } while (c == 0x00);
    return c;
}

int DCTStream::read16()
{
    int c1, c2;

    if ((c1 = str->getChar()) == EOF)
        return EOF;
    if ((c2 = str->getChar()) == EOF)
        return EOF;
    return (c1 << 8) + c2;
}

std::optional<std::string> DCTStream::getPSFilter(int psLevel, const char *indent)
{
    std::optional<std::string> s;

    if (psLevel < 2) {
        return {};
    }
    if (!(s = str->getPSFilter(psLevel, indent))) {
        return {};
    }
    s->append(indent).append("<< >> /DCTDecode filter\n");
    return s;
}

bool DCTStream::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}

#endif

#if !ENABLE_ZLIB_UNCOMPRESS
//------------------------------------------------------------------------
// FlateStream
//------------------------------------------------------------------------

const int FlateStream::codeLenCodeMap[flateMaxCodeLenCodes] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

const FlateDecode FlateStream::lengthDecode[flateMaxLitCodes - 257] = {
    { .bits = 0, .first = 3 },   { .bits = 0, .first = 4 },   { .bits = 0, .first = 5 },   { .bits = 0, .first = 6 },   { .bits = 0, .first = 7 },   { .bits = 0, .first = 8 },   { .bits = 0, .first = 9 },  { .bits = 0, .first = 10 },
    { .bits = 1, .first = 11 },  { .bits = 1, .first = 13 },  { .bits = 1, .first = 15 },  { .bits = 1, .first = 17 },  { .bits = 2, .first = 19 },  { .bits = 2, .first = 23 },  { .bits = 2, .first = 27 }, { .bits = 2, .first = 31 },
    { .bits = 3, .first = 35 },  { .bits = 3, .first = 43 },  { .bits = 3, .first = 51 },  { .bits = 3, .first = 59 },  { .bits = 4, .first = 67 },  { .bits = 4, .first = 83 },  { .bits = 4, .first = 99 }, { .bits = 4, .first = 115 },
    { .bits = 5, .first = 131 }, { .bits = 5, .first = 163 }, { .bits = 5, .first = 195 }, { .bits = 5, .first = 227 }, { .bits = 0, .first = 258 }, { .bits = 0, .first = 258 }, { .bits = 0, .first = 258 }
};

const FlateDecode FlateStream::distDecode[flateMaxDistCodes] = { { .bits = 0, .first = 1 },     { .bits = 0, .first = 2 },     { .bits = 0, .first = 3 },      { .bits = 0, .first = 4 },      { .bits = 1, .first = 5 },
                                                                 { .bits = 1, .first = 7 },     { .bits = 2, .first = 9 },     { .bits = 2, .first = 13 },     { .bits = 3, .first = 17 },     { .bits = 3, .first = 25 },
                                                                 { .bits = 4, .first = 33 },    { .bits = 4, .first = 49 },    { .bits = 5, .first = 65 },     { .bits = 5, .first = 97 },     { .bits = 6, .first = 129 },
                                                                 { .bits = 6, .first = 193 },   { .bits = 7, .first = 257 },   { .bits = 7, .first = 385 },    { .bits = 8, .first = 513 },    { .bits = 8, .first = 769 },
                                                                 { .bits = 9, .first = 1025 },  { .bits = 9, .first = 1537 },  { .bits = 10, .first = 2049 },  { .bits = 10, .first = 3073 },  { .bits = 11, .first = 4097 },
                                                                 { .bits = 11, .first = 6145 }, { .bits = 12, .first = 8193 }, { .bits = 12, .first = 12289 }, { .bits = 13, .first = 16385 }, { .bits = 13, .first = 24577 } };