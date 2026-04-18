// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 8/36



    if (reusable) {
        // If the EmbedStream is reusable, rewinding switches back to reading the recorded data
        replay = true;
        bufPos = 0;
        return true;
    }

    return false;
}

std::unique_ptr<BaseStream> EmbedStream::copy()
{
    error(errInternal, -1, "Called copy() on EmbedStream");
    return nullptr;
}

std::unique_ptr<Stream> EmbedStream::makeSubStream(Goffset /*startA*/, bool /*limitedA*/, Goffset /*lengthA*/, Object && /*dictA*/)
{
    error(errInternal, -1, "Called makeSubStream() on EmbedStream");
    return nullptr;
}

Goffset EmbedStream::getPos()
{
    if (replay) {
        return bufPos;
    }
    return str->getPos();
}

int EmbedStream::getChar()
{
    if (replay) {
        if (bufPos < bufLen) {
            return bufData[bufPos++];
        }
        replay = false;
        return EOF;
    }
    if (limited && !length) {
        return EOF;
    }
    int c = str->getChar();
    --length;
    if (reusable) {
        bufData[bufLen] = c;
        bufLen++;
        if (bufLen >= bufMax) {
            bufMax *= 2;
            bufData = (unsigned char *)grealloc(bufData, bufMax);
        }
    }
    return c;
}

int EmbedStream::lookChar()
{
    if (replay) {
        if (bufPos < bufLen) {
            return bufData[bufPos];
        }
        return EOF;
    }
    if (limited && !length) {
        return EOF;
    }
    return str->lookChar();
}

int EmbedStream::getChars(int nChars, unsigned char *buffer)
{
    if (nChars <= 0) {
        return 0;
    }
    if (replay) {
        if (bufPos >= bufLen) {
            replay = false;
            return EOF;
        }
        const long len = bufLen - bufPos;
        if (nChars > len) {
            nChars = len;
        }
        memcpy(buffer, &bufData[bufPos], nChars);
        bufPos += nChars;
        return nChars;
    }
    if (limited && length < nChars) {
        nChars = length;
    }
    const int len = str->doGetChars(nChars, buffer);
    length -= len;
    if (reusable) {
        if (bufLen + len >= bufMax) {
            while (bufLen + len >= bufMax) {
                bufMax *= 2;
            }
            bufData = (unsigned char *)grealloc(bufData, bufMax);
        }
        memcpy(bufData + bufLen, buffer, len);
        bufLen += len;
    }
    return len;
}

void EmbedStream::setPos(Goffset /*pos*/, int /*dir*/)
{
    error(errInternal, -1, "Internal: called setPos() on EmbedStream");
}

Goffset EmbedStream::getStart()
{
    error(errInternal, -1, "Internal: called getStart() on EmbedStream");
    return 0;
}

void EmbedStream::moveStart(Goffset /*delta*/)
{
    error(errInternal, -1, "Internal: called moveStart() on EmbedStream");
}

//------------------------------------------------------------------------
// ASCIIHexStream
//------------------------------------------------------------------------

ASCIIHexStream::ASCIIHexStream(std::unique_ptr<Stream> strA) : OwnedFilterStream(std::move(strA))
{
    buf = EOF;
    eof = false;
}

ASCIIHexStream::~ASCIIHexStream() = default;

bool ASCIIHexStream::rewind()
{
    buf = EOF;
    eof = false;

    return str->rewind();
}

int ASCIIHexStream::lookChar()
{
    int c1, c2, x;

    if (buf != EOF) {
        return buf;
    }
    if (eof) {
        buf = EOF;
        return EOF;
    }
    do {
        c1 = str->getChar();
    } while (isspace(c1));
    if (c1 == '>') {
        eof = true;
        buf = EOF;
        return buf;
    }
    do {
        c2 = str->getChar();
    } while (isspace(c2));
    if (c2 == '>') {
        eof = true;
        c2 = '0';
    }
    if (c1 >= '0' && c1 <= '9') {
        x = (c1 - '0') << 4;
    } else if (c1 >= 'A' && c1 <= 'F') {
        x = (c1 - 'A' + 10) << 4;
    } else if (c1 >= 'a' && c1 <= 'f') {
        x = (c1 - 'a' + 10) << 4;
    } else if (c1 == EOF) {
        eof = true;
        x = 0;
    } else {
        error(errSyntaxError, getPos(), "Illegal character <{0:02x}> in ASCIIHex stream", c1);
        x = 0;
    }
    if (c2 >= '0' && c2 <= '9') {
        x += c2 - '0';
    } else if (c2 >= 'A' && c2 <= 'F') {
        x += c2 - 'A' + 10;
    } else if (c2 >= 'a' && c2 <= 'f') {
        x += c2 - 'a' + 10;
    } else if (c2 == EOF) {
        eof = true;
        x = 0;
    } else {
        error(errSyntaxError, getPos(), "Illegal character <{0:02x}> in ASCIIHex stream", c2);
    }
    buf = x & 0xff;
    return buf;
}

std::optional<std::string> ASCIIHexStream::getPSFilter(int psLevel, const char *indent)
{
    std::optional<std::string> s;

    if (psLevel < 2) {
        return {};
    }
    if (!(s = str->getPSFilter(psLevel, indent))) {
        return {};
    }
    s->append(indent).append("/ASCIIHexDecode filter\n");
    return s;
}

bool ASCIIHexStream::isBinary(bool /*last*/) const
{
    return str->isBinary(false);
}

//------------------------------------------------------------------------
// ASCII85Stream
//------------------------------------------------------------------------