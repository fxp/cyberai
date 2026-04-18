// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 34/36



    c = buf[0];
    for (i = 1; i < bufSize; ++i) {
        buf[i - 1] = buf[i];
    }
    buf[bufSize - 1] = str->getChar();
    return c;
}

int BufStream::lookChar()
{
    return buf[0];
}

int BufStream::lookChar(int idx)
{
    return buf[idx];
}

bool BufStream::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}

//------------------------------------------------------------------------
// FixedLengthEncoder
//------------------------------------------------------------------------

FixedLengthEncoder::FixedLengthEncoder(Stream *strA, int lengthA) : FilterStream(strA)
{
    length = lengthA;
    count = 0;
}

FixedLengthEncoder::~FixedLengthEncoder()
{
    if (str->isEncoder()) {
        delete str;
    }
}

bool FixedLengthEncoder::rewind()
{
    count = 0;

    return str->rewind();
}

int FixedLengthEncoder::getChar()
{
    if (length >= 0 && count >= length) {
        return EOF;
    }
    ++count;
    return str->getChar();
}

int FixedLengthEncoder::lookChar()
{
    if (length >= 0 && count >= length) {
        return EOF;
    }
    return str->getChar();
}

bool FixedLengthEncoder::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}

//------------------------------------------------------------------------
// ASCIIHexEncoder
//------------------------------------------------------------------------

ASCIIHexEncoder::ASCIIHexEncoder(Stream *strA) : FilterStream(strA)
{
    bufPtr = bufEnd = buf;
    lineLen = 0;
    eof = false;
}

ASCIIHexEncoder::~ASCIIHexEncoder()
{
    if (str->isEncoder()) {
        delete str;
    }
}

bool ASCIIHexEncoder::rewind()
{
    bufPtr = bufEnd = buf;
    lineLen = 0;
    eof = false;

    return str->rewind();
}

bool ASCIIHexEncoder::fillBuf()
{
    static const char *hex = "0123456789abcdef";
    int c;

    if (eof) {
        return false;
    }
    bufPtr = bufEnd = buf;
    if ((c = str->getChar()) == EOF) {
        *bufEnd++ = '>';
        eof = true;
    } else {
        if (lineLen >= 64) {
            *bufEnd++ = '\n';
            lineLen = 0;
        }
        *bufEnd++ = hex[(c >> 4) & 0x0f];
        *bufEnd++ = hex[c & 0x0f];
        lineLen += 2;
    }
    return true;
}

//------------------------------------------------------------------------
// ASCII85Encoder
//------------------------------------------------------------------------

ASCII85Encoder::ASCII85Encoder(Stream *strA) : FilterStream(strA)
{
    bufPtr = bufEnd = buf;
    lineLen = 0;
    eof = false;
}

ASCII85Encoder::~ASCII85Encoder()
{
    if (str->isEncoder()) {
        delete str;
    }
}

bool ASCII85Encoder::rewind()
{
    bufPtr = bufEnd = buf;
    lineLen = 0;
    eof = false;

    return str->rewind();
}

bool ASCII85Encoder::fillBuf()
{
    unsigned int t;
    char buf1[5];
    int c0, c1, c2, c3;
    int n, i;

    if (eof) {
        return false;
    }
    c0 = str->getChar();
    c1 = str->getChar();
    c2 = str->getChar();
    c3 = str->getChar();
    bufPtr = bufEnd = buf;
    if (c3 == EOF) {
        if (c0 == EOF) {
            n = 0;
            t = 0;
        } else {
            if (c1 == EOF) {
                n = 1;
                t = c0 << 24;
            } else if (c2 == EOF) {
                n = 2;
                t = (c0 << 24) | (c1 << 16);
            } else {
                n = 3;
                t = (c0 << 24) | (c1 << 16) | (c2 << 8);
            }
            for (i = 4; i >= 0; --i) {
                buf1[i] = (char)(t % 85 + 0x21);
                t /= 85;
            }
            for (i = 0; i <= n; ++i) {
                *bufEnd++ = buf1[i];
                if (++lineLen == 65) {
                    *bufEnd++ = '\n';
                    lineLen = 0;
                }
            }
        }
        *bufEnd++ = '~';
        *bufEnd++ = '>';
        eof = true;
    } else {
        t = (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
        if (t == 0) {
            *bufEnd++ = 'z';
            if (++lineLen == 65) {
                *bufEnd++ = '\n';
                lineLen = 0;
            }
        } else {
            for (i = 4; i >= 0; --i) {
                buf1[i] = (char)(t % 85 + 0x21);
                t /= 85;
            }
            for (i = 0; i <= 4; ++i) {
                *bufEnd++ = buf1[i];
                if (++lineLen == 65) {
                    *bufEnd++ = '\n';
                    lineLen = 0;
                }
            }
        }
    }
    return true;
}

//------------------------------------------------------------------------
// RunLengthEncoder
//------------------------------------------------------------------------

RunLengthEncoder::RunLengthEncoder(Stream *strA) : FilterStream(strA)
{
    bufPtr = bufEnd = nextEnd = buf;
    eof = false;
}

RunLengthEncoder::~RunLengthEncoder()
{
    if (str->isEncoder()) {
        delete str;
    }
}

bool RunLengthEncoder::rewind()
{
    bufPtr = bufEnd = nextEnd = buf;
    eof = false;

    return str->rewind();
}