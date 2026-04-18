// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 35/36



//
// When fillBuf finishes, buf[] looks like this:
//   +-----+--------------+-----------------+--
//   + tag | ... data ... | next 0, 1, or 2 |
//   +-----+--------------+-----------------+--
//    ^                    ^                 ^
//    bufPtr               bufEnd            nextEnd
//
bool RunLengthEncoder::fillBuf()
{
    int c, c1, c2;
    int n;

    // already hit EOF?
    if (eof) {
        return false;
    }

    // grab two bytes
    if (nextEnd < bufEnd + 1) {
        if ((c1 = str->getChar()) == EOF) {
            eof = true;
            return false;
        }
    } else {
        c1 = bufEnd[0] & 0xff;
    }
    if (nextEnd < bufEnd + 2) {
        if ((c2 = str->getChar()) == EOF) {
            eof = true;
            buf[0] = 0;
            buf[1] = c1;
            bufPtr = buf;
            bufEnd = &buf[2];
            return true;
        }
    } else {
        c2 = bufEnd[1] & 0xff;
    }

    // check for repeat
    c = 0; // make gcc happy
    if (c1 == c2) {
        n = 2;
        while (n < 128 && (c = str->getChar()) == c1) {
            ++n;
        }
        buf[0] = (char)(257 - n);
        buf[1] = c1;
        bufEnd = &buf[2];
        if (c == EOF) {
            eof = true;
        } else if (n < 128) {
            buf[2] = c;
            nextEnd = &buf[3];
        } else {
            nextEnd = bufEnd;
        }

        // get up to 128 chars
    } else {
        buf[1] = c1;
        buf[2] = c2;
        n = 2;
        while (n < 128) {
            if ((c = str->getChar()) == EOF) {
                eof = true;
                break;
            }
            ++n;
            buf[n] = c;
            if (buf[n] == buf[n - 1]) {
                break;
            }
        }
        if (buf[n] == buf[n - 1]) {
            buf[0] = (char)(n - 2 - 1);
            bufEnd = &buf[n - 1];
            nextEnd = &buf[n + 1];
        } else {
            buf[0] = (char)(n - 1);
            bufEnd = nextEnd = &buf[n + 1];
        }
    }
    bufPtr = buf;
    return true;
}

//------------------------------------------------------------------------
// LZWEncoder
//------------------------------------------------------------------------

LZWEncoder::LZWEncoder(Stream *strA) : FilterStream(strA)
{
    inBufLen = 0;
    outBufLen = 0;
}

LZWEncoder::~LZWEncoder()
{
    if (str->isEncoder()) {
        delete str;
    }
}

bool LZWEncoder::rewind()
{
    int i;

    bool success = str->rewind();

    // initialize code table
    for (i = 0; i < 256; ++i) {
        table[i].byte = i;
        table[i].next = nullptr;
        table[i].children = nullptr;
    }
    nextSeq = 258;
    codeLen = 9;

    // initialize input buffer
    inBufLen = str->doGetChars(sizeof(inBuf), inBuf);

    // initialize output buffer with a clear-table code
    outBuf = 256;
    outBufLen = 9;
    needEOD = false;

    return success;
}

int LZWEncoder::getChar()
{
    int ret;

    if (inBufLen == 0 && !needEOD && outBufLen == 0) {
        return EOF;
    }
    if (outBufLen < 8 && (inBufLen > 0 || needEOD)) {
        fillBuf();
    }
    if (outBufLen >= 8) {
        ret = (outBuf >> (outBufLen - 8)) & 0xff;
        outBufLen -= 8;
    } else {
        ret = (outBuf << (8 - outBufLen)) & 0xff;
        outBufLen = 0;
    }
    return ret;
}

int LZWEncoder::lookChar()
{
    if (inBufLen == 0 && !needEOD && outBufLen == 0) {
        return EOF;
    }
    if (outBufLen < 8 && (inBufLen > 0 || needEOD)) {
        fillBuf();
    }
    if (outBufLen >= 8) {
        return (outBuf >> (outBufLen - 8)) & 0xff;
    }
    return (outBuf << (8 - outBufLen)) & 0xff;
}

// On input, outBufLen < 8.
// This function generates, at most, 2 12-bit codes
//   --> outBufLen < 8 + 12 + 12 = 32
void LZWEncoder::fillBuf()
{
    LZWEncoderNode *p0, *p1;
    int seqLen, code, i;

    if (needEOD) {
        outBuf = (outBuf << codeLen) | 257;
        outBufLen += codeLen;
        needEOD = false;
        return;
    }

    // find longest matching sequence (if any)
    p0 = table + inBuf[0];
    seqLen = 1;
    while (inBufLen > seqLen) {
        for (p1 = p0->children; p1; p1 = p1->next) {
            if (p1->byte == inBuf[seqLen]) {
                break;
            }
        }
        if (!p1) {
            break;
        }
        p0 = p1;
        ++seqLen;
    }
    code = (int)(p0 - table);

    // generate an output code
    outBuf = (outBuf << codeLen) | code;
    outBufLen += codeLen;

    // update the table
    table[nextSeq].byte = seqLen < inBufLen ? inBuf[seqLen] : 0;
    table[nextSeq].children = nullptr;
    if (table[code].children) {
        table[nextSeq].next = table[code].children;
    } else {
        table[nextSeq].next = nullptr;
    }
    table[code].children = table + nextSeq;
    ++nextSeq;

    // update the input buffer
    memmove(inBuf, inBuf + seqLen, inBufLen - seqLen);
    inBufLen -= seqLen;
    inBufLen += str->doGetChars(sizeof(inBuf) - inBufLen, inBuf + inBufLen);