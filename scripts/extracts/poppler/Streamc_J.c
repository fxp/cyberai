// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 10/36



    // process the next code
    nextLength = seqLength + 1;
    if (code < 256) {
        seqBuf[0] = code;
        seqLength = 1;
    } else if (code < nextCode) {
        seqLength = table[code].length;
        for (i = seqLength - 1, j = code; i > 0; --i) {
            seqBuf[i] = table[j].tail;
            j = table[j].head;
        }
        seqBuf[0] = j;
    } else if (code == nextCode) {
        seqBuf[seqLength] = newChar;
        ++seqLength;
    } else {
        error(errSyntaxError, getPos(), "Bad LZW stream - unexpected code");
        eof = true;
        return false;
    }
    newChar = seqBuf[0];
    if (first) {
        first = false;
    } else {
        if (nextCode < 4097) {
            table[nextCode].length = nextLength;
            table[nextCode].head = prevCode;
            table[nextCode].tail = newChar;
            ++nextCode;
        }
        if (nextCode + early == 512) {
            nextBits = 10;
        } else if (nextCode + early == 1024) {
            nextBits = 11;
        } else if (nextCode + early == 2048) {
            nextBits = 12;
        }
    }
    prevCode = code;

    // rewind buffer
    seqIndex = 0;

    return true;
}

void LZWStream::clearTable()
{
    nextCode = 258;
    nextBits = 9;
    seqIndex = seqLength = 0;
    first = true;
    newChar = 0;
}

int LZWStream::getCode()
{
    int c;
    int code;

    while (inputBits < nextBits) {
        if ((c = str->getChar()) == EOF) {
            return EOF;
        }
        inputBuf = (inputBuf << 8) | static_cast<unsigned>(c & 0xff);
        inputBits += 8;
    }
    code = static_cast<signed>((inputBuf >> (inputBits - nextBits)) & ((1 << nextBits) - 1));
    inputBits -= nextBits;
    return code;
}

std::optional<std::string> LZWStream::getPSFilter(int psLevel, const char *indent)
{
    std::optional<std::string> s;

    if (psLevel < 2 || pred) {
        return {};
    }
    if (!(s = str->getPSFilter(psLevel, indent))) {
        return {};
    }
    s->append(indent).append("<< ");
    if (!early) {
        s->append("/EarlyChange 0 ");
    }
    s->append(">> /LZWDecode filter\n");
    return s;
}

bool LZWStream::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}

//------------------------------------------------------------------------
// RunLengthStream
//------------------------------------------------------------------------

RunLengthStream::RunLengthStream(std::unique_ptr<Stream> strA) : OwnedFilterStream(std::move(strA))
{
    bufPtr = bufEnd = buf;
    eof = false;
}

RunLengthStream::~RunLengthStream() = default;

bool RunLengthStream::rewind()
{
    bufPtr = bufEnd = buf;
    eof = false;

    return str->rewind();
}

int RunLengthStream::getChars(int nChars, unsigned char *buffer)
{
    int n, m;

    n = 0;
    while (n < nChars) {
        if (bufPtr >= bufEnd) {
            if (!fillBuf()) {
                break;
            }
        }
        m = (int)(bufEnd - bufPtr);
        if (m > nChars - n) {
            m = nChars - n;
        }
        memcpy(buffer + n, bufPtr, m);
        bufPtr += m;
        n += m;
    }
    return n;
}

std::optional<std::string> RunLengthStream::getPSFilter(int psLevel, const char *indent)
{
    std::optional<std::string> s;

    if (psLevel < 2) {
        return {};
    }
    if (!(s = str->getPSFilter(psLevel, indent))) {
        return {};
    }
    s->append(indent).append("/RunLengthDecode filter\n");
    return s;
}

bool RunLengthStream::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}

bool RunLengthStream::fillBuf()
{
    int c;
    int n, i;

    if (eof) {
        return false;
    }
    c = str->getChar();
    if (c == 0x80 || c == EOF) {
        eof = true;
        return false;
    }
    if (c < 0x80) {
        n = c + 1;
        for (i = 0; i < n; ++i) {
            buf[i] = (char)str->getChar();
        }
    } else {
        n = 0x101 - c;
        c = str->getChar();
        for (i = 0; i < n; ++i) {
            buf[i] = (char)c;
        }
    }
    bufPtr = buf;
    bufEnd = buf + n;
    return true;
}

//------------------------------------------------------------------------
// CCITTFaxStream
//------------------------------------------------------------------------