// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 36/36



    // increment codeLen; generate clear-table code
    if (nextSeq == (1 << codeLen)) {
        ++codeLen;
        if (codeLen == 13) {
            outBuf = (outBuf << 12) | 256;
            outBufLen += 12;
            for (i = 0; i < 256; ++i) {
                table[i].next = nullptr;
                table[i].children = nullptr;
            }
            nextSeq = 258;
            codeLen = 9;
        }
    }

    // generate EOD next time
    if (inBufLen == 0) {
        needEOD = true;
    }
}

//------------------------------------------------------------------------
// CMYKGrayEncoder
//------------------------------------------------------------------------

CMYKGrayEncoder::CMYKGrayEncoder(Stream *strA) : FilterStream(strA)
{
    bufPtr = bufEnd = buf;
    eof = false;
}

CMYKGrayEncoder::~CMYKGrayEncoder()
{
    if (str->isEncoder()) {
        delete str;
    }
}

bool CMYKGrayEncoder::rewind()
{
    bufPtr = bufEnd = buf;
    eof = false;

    return str->rewind();
}

bool CMYKGrayEncoder::fillBuf()
{
    int c0, c1, c2, c3;
    int i;

    if (eof) {
        return false;
    }
    c0 = str->getChar();
    c1 = str->getChar();
    c2 = str->getChar();
    c3 = str->getChar();
    if (c3 == EOF) {
        eof = true;
        return false;
    }
    i = (3 * c0 + 6 * c1 + c2) / 10 + c3;
    if (i > 255) {
        i = 255;
    }
    bufPtr = bufEnd = buf;
    *bufEnd++ = (char)i;
    return true;
}

//------------------------------------------------------------------------
// RGBGrayEncoder
//------------------------------------------------------------------------

RGBGrayEncoder::RGBGrayEncoder(Stream *strA) : FilterStream(strA)
{
    bufPtr = bufEnd = buf;
    eof = false;
}

RGBGrayEncoder::~RGBGrayEncoder()
{
    if (str->isEncoder()) {
        delete str;
    }
}

bool RGBGrayEncoder::rewind()
{
    bufPtr = bufEnd = buf;
    eof = false;

    return str->rewind();
}

bool RGBGrayEncoder::fillBuf()
{
    int c0, c1, c2;
    int i;

    if (eof) {
        return false;
    }
    c0 = str->getChar();
    c1 = str->getChar();
    c2 = str->getChar();
    if (c2 == EOF) {
        eof = true;
        return false;
    }
    i = 255 - (3 * c0 + 6 * c1 + c2) / 10;
    if (i < 0) {
        i = 0;
    }
    bufPtr = bufEnd = buf;
    *bufEnd++ = (char)i;
    return true;
}

//------------------------------------------------------------------------
// SplashBitmapCMYKEncoder
//------------------------------------------------------------------------

SplashBitmapCMYKEncoder::SplashBitmapCMYKEncoder(SplashBitmap *bitmapA) : bitmap(bitmapA)
{
    width = (size_t)4 * bitmap->getWidth();
    height = bitmap->getHeight();
    buf.resize(width);
    bufPtr = width;
    curLine = height - 1;
}

SplashBitmapCMYKEncoder::~SplashBitmapCMYKEncoder() = default;

bool SplashBitmapCMYKEncoder::rewind()
{
    bufPtr = width;
    curLine = height - 1;

    return true;
}

int SplashBitmapCMYKEncoder::lookChar()
{
    if (bufPtr >= width && !fillBuf()) {
        return EOF;
    }
    return buf[bufPtr];
}

int SplashBitmapCMYKEncoder::getChar()
{
    int ret = lookChar();
    bufPtr++;
    return ret;
}

bool SplashBitmapCMYKEncoder::fillBuf()
{
    if (curLine < 0) {
        return false;
    }

    if (bufPtr < width) {
        return true;
    }

    bitmap->getCMYKLine(curLine, buf.data());
    bufPtr = 0;
    curLine--;
    return true;
}

Goffset SplashBitmapCMYKEncoder::getPos()
{
    return (height - 1 - curLine) * width + bufPtr;
}

void SplashBitmapCMYKEncoder::setPos(Goffset pos, int dir)
{
    // This code is mostly untested!
    if (dir < 0) {
        curLine = pos / width;
    } else {
        curLine = height - 1 - pos / width;
    }

    bufPtr = width;
    fillBuf();

    if (dir < 0) {
        bufPtr = width - 1 - pos % width;
    } else {
        bufPtr = pos % width;
    }
}
