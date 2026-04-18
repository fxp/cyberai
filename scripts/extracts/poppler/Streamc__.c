// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 31/36

, .val = 0x002f }, { .len = 9, .val = 0x00bf }, { .len = 8, .val = 0x000f }, { .len = 8, .val = 0x008f }, { .len = 8, .val = 0x004f }, { .len = 9, .val = 0x00ff }
};

FlateHuffmanTab FlateStream::fixedLitCodeTab = { .codes = flateFixedLitCodeTabCodes, .maxLen = 9 };

static const FlateCode flateFixedDistCodeTabCodes[32] = { { .len = 5, .val = 0x0000 }, { .len = 5, .val = 0x0010 }, { .len = 5, .val = 0x0008 }, { .len = 5, .val = 0x0018 }, { .len = 5, .val = 0x0004 }, { .len = 5, .val = 0x0014 },
                                                          { .len = 5, .val = 0x000c }, { .len = 5, .val = 0x001c }, { .len = 5, .val = 0x0002 }, { .len = 5, .val = 0x0012 }, { .len = 5, .val = 0x000a }, { .len = 5, .val = 0x001a },
                                                          { .len = 5, .val = 0x0006 }, { .len = 5, .val = 0x0016 }, { .len = 5, .val = 0x000e }, { .len = 0, .val = 0x0000 }, { .len = 5, .val = 0x0001 }, { .len = 5, .val = 0x0011 },
                                                          { .len = 5, .val = 0x0009 }, { .len = 5, .val = 0x0019 }, { .len = 5, .val = 0x0005 }, { .len = 5, .val = 0x0015 }, { .len = 5, .val = 0x000d }, { .len = 5, .val = 0x001d },
                                                          { .len = 5, .val = 0x0003 }, { .len = 5, .val = 0x0013 }, { .len = 5, .val = 0x000b }, { .len = 5, .val = 0x001b }, { .len = 5, .val = 0x0007 }, { .len = 5, .val = 0x0017 },
                                                          { .len = 5, .val = 0x000f }, { .len = 0, .val = 0x0000 } };

FlateHuffmanTab FlateStream::fixedDistCodeTab = { .codes = flateFixedDistCodeTabCodes, .maxLen = 5 };

FlateStream::FlateStream(std::unique_ptr<Stream> strA, int predictor, int columns, int colors, int bits) : OwnedFilterStream(std::move(strA))
{
    if (predictor != 1) {
        pred = new StreamPredictor(this, predictor, columns, colors, bits);
        if (!pred->isOk()) {
            delete pred;
            pred = nullptr;
        }
    } else {
        pred = nullptr;
    }
    litCodeTab.codes = nullptr;
    distCodeTab.codes = nullptr;
    memset(buf, 0, flateWindow);
}

FlateStream::~FlateStream()
{
    if (litCodeTab.codes != fixedLitCodeTab.codes) {
        gfree(const_cast<FlateCode *>(litCodeTab.codes));
    }
    if (distCodeTab.codes != fixedDistCodeTab.codes) {
        gfree(const_cast<FlateCode *>(distCodeTab.codes));
    }
    delete pred;
}

bool FlateStream::flateRewind(bool unfiltered)
{
    index = 0;
    remain = 0;
    codeBuf = 0;
    codeSize = 0;
    compressedBlock = false;
    endOfBlock = true;
    eof = true;
    if (unfiltered) {
        return str->unfilteredRewind();
    }
    return str->rewind();
}

bool FlateStream::unfilteredRewind()
{
    return flateRewind(true);
}

bool FlateStream::rewind()
{
    int cmf, flg;

    bool internalResetResult = flateRewind(false);

    // read header
    //~ need to look at window size?
    endOfBlock = eof = true;
    cmf = str->getChar();
    flg = str->getChar();
    if (cmf == EOF || flg == EOF) {
        return false;
    }
    if ((cmf & 0x0f) != 0x08) {
        error(errSyntaxError, getPos(), "Unknown compression method in flate stream");
        return false;
    }
    if ((((cmf << 8) + flg) % 31) != 0) {
        error(errSyntaxError, getPos(), "Bad FCHECK in flate stream");
        return false;
    }
    if (flg & 0x20) {
        error(errSyntaxError, getPos(), "FDICT bit set in flate stream");
        return false;
    }

    eof = false;

    return internalResetResult;
}

int FlateStream::getChar()
{
    if (pred) {
        return pred->getChar();
    }
    return doGetRawChar();
}

int FlateStream::getChars(int nChars, unsigned char *buffer)
{
    if (pred) {
        return pred->getChars(nChars, buffer);
    }
    for (int i = 0; i < nChars; ++i) {
        const int c = doGetRawChar();
        if (likely(c != EOF)) {
            buffer[i] = c;
        } else {
            return i;
        }
    }
    return nChars;
}

int FlateStream::lookChar()
{
    int c;

    if (pred) {
        return pred->lookChar();
    }
    while (remain == 0) {
        if (endOfBlock && eof) {
            return EOF;
        }
        readSome();
    }
    c = buf[index];
    return c;
}

void FlateStream::getRawChars(int nChars, int *buffer)
{
    for (int i = 0; i < nChars; ++i) {
        buffer[i] = doGetRawChar();
    }
}

int FlateStream::getRawChar()
{
    return doGetRawChar();
}

std::optional<std::string> FlateStream::getPSFilter(int psLevel, const char *indent)
{
    std::optional<std::string> s;

    if (psLevel < 3 || pred) {
        return {};
    }
    if (!(s = str->getPSFilter(psLevel, indent))) {
        return {};
    }
    s->append(indent).append("<< >> /FlateDecode filter\n");
    return s;
}

bool FlateStream::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}

void FlateStream::readSome()
{
    int code1, code2;
    int len, dist;
    int i, j, k;
    int c;