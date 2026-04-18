// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 9/36



ASCII85Stream::ASCII85Stream(std::unique_ptr<Stream> strA) : OwnedFilterStream(std::move(strA))
{
    index = n = 0;
    eof = false;
}

ASCII85Stream::~ASCII85Stream() = default;

bool ASCII85Stream::rewind()
{
    index = n = 0;
    eof = false;

    return str->rewind();
}

int ASCII85Stream::lookChar()
{
    int k;
    unsigned long t;

    if (index >= n) {
        if (eof) {
            return EOF;
        }
        index = 0;
        do {
            c[0] = str->getChar();
        } while (Lexer::isSpace(c[0]));
        if (c[0] == '~' || c[0] == EOF) {
            eof = true;
            n = 0;
            return EOF;
        }
        if (c[0] == 'z') {
            b[0] = b[1] = b[2] = b[3] = 0;
            n = 4;
        } else {
            for (k = 1; k < 5; ++k) {
                do {
                    c[k] = str->getChar();
                } while (Lexer::isSpace(c[k]));
                if (c[k] == '~' || c[k] == EOF) {
                    break;
                }
            }
            n = k - 1;
            if (k < 5 && (c[k] == '~' || c[k] == EOF)) {
                for (++k; k < 5; ++k) {
                    c[k] = 0x21 + 84;
                }
                eof = true;
            }
            t = 0;
            for (k = 0; k < 5; ++k) {
                t = t * 85 + (c[k] - 0x21);
            }
            for (k = 3; k >= 0; --k) {
                b[k] = (int)(t & 0xff);
                t >>= 8;
            }
        }
    }
    return b[index];
}

std::optional<std::string> ASCII85Stream::getPSFilter(int psLevel, const char *indent)
{
    std::optional<std::string> s;

    if (psLevel < 2) {
        return {};
    }
    if (!(s = str->getPSFilter(psLevel, indent))) {
        return {};
    }
    s->append(indent).append("/ASCII85Decode filter\n");
    return s;
}

bool ASCII85Stream::isBinary(bool /*last*/) const
{
    return str->isBinary(false);
}

//------------------------------------------------------------------------
// LZWStream
//------------------------------------------------------------------------

LZWStream::LZWStream(std::unique_ptr<Stream> strA, int predictor, int columns, int colors, int bits, int earlyA) : OwnedFilterStream(std::move(strA))
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
    early = earlyA;
    eof = false;
    inputBits = 0;
    clearTable();
}

LZWStream::~LZWStream()
{
    delete pred;
}

int LZWStream::getChar()
{
    if (pred) {
        return pred->getChar();
    }
    if (eof) {
        return EOF;
    }
    if (seqIndex >= seqLength) {
        if (!processNextCode()) {
            return EOF;
        }
    }
    return seqBuf[seqIndex++];
}

int LZWStream::lookChar()
{
    if (pred) {
        return pred->lookChar();
    }
    if (eof) {
        return EOF;
    }
    if (seqIndex >= seqLength) {
        if (!processNextCode()) {
            return EOF;
        }
    }
    return seqBuf[seqIndex];
}

void LZWStream::getRawChars(int nChars, int *buffer)
{
    for (int i = 0; i < nChars; ++i) {
        buffer[i] = doGetRawChar();
    }
}

int LZWStream::getRawChar()
{
    return doGetRawChar();
}

int LZWStream::getChars(int nChars, unsigned char *buffer)
{
    int n, m;

    if (pred) {
        return pred->getChars(nChars, buffer);
    }
    if (eof) {
        return 0;
    }
    n = 0;
    while (n < nChars) {
        if (seqIndex >= seqLength) {
            if (!processNextCode()) {
                break;
            }
        }
        m = seqLength - seqIndex;
        if (m > nChars - n) {
            m = nChars - n;
        }
        memcpy(buffer + n, seqBuf + seqIndex, m);
        seqIndex += m;
        n += m;
    }
    return n;
}

bool LZWStream::rewind()
{
    bool success = str->rewind();
    eof = false;
    inputBits = 0;
    clearTable();

    return success;
}

bool LZWStream::processNextCode()
{
    int code;
    int nextLength;
    int i, j;

    // check for EOF
    if (eof) {
        return false;
    }

    // check for eod and clear-table codes
start:
    code = getCode();
    if (code == EOF || code == 257) {
        eof = true;
        return false;
    }
    if (code == 256) {
        clearTable();
        goto start;
    }