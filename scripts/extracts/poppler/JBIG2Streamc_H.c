// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 8/38



    if (bufLen == 0) {
        buf = str->getChar() & 0xff;
        bufLen = 8;
        ++nBytesRead;
        ++byteCounter;
    }
    while (true) {
        if (bufLen >= 11 && ((buf >> (bufLen - 7)) & 0x7f) == 0) {
            if (bufLen <= 12) {
                code = buf << (12 - bufLen);
            } else {
                code = buf >> (bufLen - 12);
            }
            p = &whiteTab1[code & 0x1f];
        } else {
            if (bufLen <= 9) {
                code = buf << (9 - bufLen);
            } else {
                code = buf >> (bufLen - 9);
            }
            p = &whiteTab2[code & 0x1ff];
        }
        if (p->bits > 0 && p->bits <= (int)bufLen) {
            bufLen -= p->bits;
            return p->n;
        }
        if (bufLen >= 12) {
            break;
        }
        buf = (buf << 8) | (str->getChar() & 0xff);
        bufLen += 8;
        ++nBytesRead;
        ++byteCounter;
    }
    error(errSyntaxError, str->getPos(), "Bad white code in JBIG2 MMR stream");
    // eat a bit and return a positive number so that the caller doesn't
    // go into an infinite loop
    --bufLen;
    return 1;
}

int JBIG2MMRDecoder::getBlackCode()
{
    const CCITTCode *p;
    unsigned int code;

    if (bufLen == 0) {
        buf = str->getChar() & 0xff;
        bufLen = 8;
        ++nBytesRead;
        ++byteCounter;
    }
    while (true) {
        if (bufLen >= 10 && ((buf >> (bufLen - 6)) & 0x3f) == 0) {
            if (bufLen <= 13) {
                code = buf << (13 - bufLen);
            } else {
                code = buf >> (bufLen - 13);
            }
            p = &blackTab1[code & 0x7f];
        } else if (bufLen >= 7 && ((buf >> (bufLen - 4)) & 0x0f) == 0 && ((buf >> (bufLen - 6)) & 0x03) != 0) {
            if (bufLen <= 12) {
                code = buf << (12 - bufLen);
            } else {
                code = buf >> (bufLen - 12);
            }
            if (unlikely((code & 0xff) < 64)) {
                break;
            }
            p = &blackTab2[(code & 0xff) - 64];
        } else {
            if (bufLen <= 6) {
                code = buf << (6 - bufLen);
            } else {
                code = buf >> (bufLen - 6);
            }
            p = &blackTab3[code & 0x3f];
        }
        if (p->bits > 0 && p->bits <= (int)bufLen) {
            bufLen -= p->bits;
            return p->n;
        }
        if (bufLen >= 13) {
            break;
        }
        buf = (buf << 8) | (str->getChar() & 0xff);
        bufLen += 8;
        ++nBytesRead;
        ++byteCounter;
    }
    error(errSyntaxError, str->getPos(), "Bad black code in JBIG2 MMR stream");
    // eat a bit and return a positive number so that the caller doesn't
    // go into an infinite loop
    --bufLen;
    return 1;
}

unsigned int JBIG2MMRDecoder::get24Bits()
{
    while (bufLen < 24) {
        buf = (buf << 8) | (str->getChar() & 0xff);
        bufLen += 8;
        ++nBytesRead;
        ++byteCounter;
    }
    return (buf >> (bufLen - 24)) & 0xffffff;
}

bool JBIG2MMRDecoder::skipTo(unsigned int length)
{
    const unsigned int nBytesToDiscard = length - nBytesRead;
    const unsigned int n = str->discardChars(nBytesToDiscard);
    nBytesRead += n;
    byteCounter += n;
    return n == nBytesToDiscard;
}

//------------------------------------------------------------------------
// JBIG2Segment
//------------------------------------------------------------------------

enum JBIG2SegmentType
{
    jbig2SegBitmap,
    jbig2SegSymbolDict,
    jbig2SegPatternDict,
    jbig2SegCodeTable
};

class JBIG2Segment
{
public:
    explicit JBIG2Segment(unsigned int segNumA) { segNum = segNumA; }
    virtual ~JBIG2Segment();
    JBIG2Segment(const JBIG2Segment &) = delete;
    JBIG2Segment &operator=(const JBIG2Segment &) = delete;
    void setSegNum(unsigned int segNumA) { segNum = segNumA; }
    unsigned int getSegNum() const { return segNum; }
    virtual JBIG2SegmentType getType() const = 0;

private:
    unsigned int segNum;
};

JBIG2Segment::~JBIG2Segment() = default;

//------------------------------------------------------------------------
// JBIG2Bitmap
//------------------------------------------------------------------------

struct JBIG2BitmapPtr
{
    unsigned char *p;
    int shift;
    int x;
};