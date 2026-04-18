// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 7/38



unsigned int JBIG2HuffmanDecoder::readBits(unsigned int n, bool *eof)
{
    unsigned int x, mask, nLeft;

    mask = (n == 32) ? 0xffffffff : ((1 << n) - 1);
    if (bufLen >= n) {
        x = (buf >> (bufLen - n)) & mask;
        bufLen -= n;
    } else {
        x = buf & ((1 << bufLen) - 1);
        nLeft = n - bufLen;
        bufLen = 0;
        while (nLeft >= 8) {
            const int c = str->getChar();
            if (eof) {
                *eof = *eof || c == EOF;
            }
            x = (x << 8) | (c & 0xff);
            ++byteCounter;
            nLeft -= 8;
        }
        if (nLeft > 0) {
            const int c = str->getChar();
            if (eof) {
                *eof = *eof || c == EOF;
            }
            buf = c;
            ++byteCounter;
            bufLen = 8 - nLeft;
            x = (x << nLeft) | ((buf >> bufLen) & ((1 << nLeft) - 1));
        }
    }
    return x;
}

unsigned int JBIG2HuffmanDecoder::readBit()
{
    if (bufLen == 0) {
        buf = str->getChar();
        ++byteCounter;
        bufLen = 8;
    }
    --bufLen;
    return (buf >> bufLen) & 1;
}

bool JBIG2HuffmanDecoder::buildTable(JBIG2HuffmanTable *table, unsigned int len)
{
    unsigned int i, j;

    // stable selection sort:
    // - entries with prefixLen > 0, in ascending prefixLen order
    // - entry with prefixLen = 0, rangeLen = EOT
    // - all other entries with prefixLen = 0
    // (on entry, table[len] has prefixLen = 0, rangeLen = EOT)
    for (i = 0; i < len; ++i) {
        for (j = i; j < len && table[j].prefixLen == 0; ++j) {
            ;
        }
        if (j == len) {
            break;
        }
        for (unsigned int k = j + 1; k < len; ++k) {
            if (table[k].prefixLen > 0 && table[k].prefixLen < table[j].prefixLen) {
                j = k;
            }
        }
        if (j != i) {
            const JBIG2HuffmanTable tab = table[j];
            for (unsigned int k = j; k > i; --k) {
                table[k] = table[k - 1];
            }
            table[i] = tab;
        }
    }
    table[i] = table[len];

    // assign prefixes
    if (table[0].rangeLen != jbig2HuffmanEOT) {
        i = 0;
        unsigned int prefix = 0;
        table[i++].prefix = prefix++;
        for (; table[i].rangeLen != jbig2HuffmanEOT; ++i) {
            const size_t bitsToShift = table[i].prefixLen - table[i - 1].prefixLen;
            if (bitsToShift >= intNBits) {
                error(errSyntaxError, -1, "Failed to build table for JBIG2 stream");
                return false;
            }
            prefix <<= bitsToShift;

            table[i].prefix = prefix++;
        }
    }

    return true;
}

//------------------------------------------------------------------------
// JBIG2MMRDecoder
//------------------------------------------------------------------------

class JBIG2MMRDecoder
{
public:
    JBIG2MMRDecoder();
    ~JBIG2MMRDecoder() = default;
    void setStream(Stream *strA) { str = strA; }
    void reset();
    int get2DCode();
    int getBlackCode();
    int getWhiteCode();
    unsigned int get24Bits();
    void resetByteCounter() { byteCounter = 0; }
    unsigned int getByteCounter() const { return byteCounter; }
    [[nodiscard]] bool skipTo(unsigned int length);

private:
    Stream *str;
    unsigned int buf;
    unsigned int bufLen;
    unsigned int nBytesRead;
    unsigned int byteCounter;
};

JBIG2MMRDecoder::JBIG2MMRDecoder()
{
    str = nullptr;
    byteCounter = 0;
    reset();
}

void JBIG2MMRDecoder::reset()
{
    buf = 0;
    bufLen = 0;
    nBytesRead = 0;
}

int JBIG2MMRDecoder::get2DCode()
{
    const CCITTCode *p = nullptr;

    if (bufLen == 0) {
        buf = str->getChar() & 0xff;
        bufLen = 8;
        ++nBytesRead;
        ++byteCounter;
        p = &twoDimTab1[(buf >> 1) & 0x7f];
    } else if (bufLen == 8) {
        p = &twoDimTab1[(buf >> 1) & 0x7f];
    } else if (bufLen < 8) {
        p = &twoDimTab1[(buf << (7 - bufLen)) & 0x7f];
        if (p->bits < 0 || p->bits > (int)bufLen) {
            buf = (buf << 8) | (str->getChar() & 0xff);
            bufLen += 8;
            ++nBytesRead;
            ++byteCounter;
            p = &twoDimTab1[(buf >> (bufLen - 7)) & 0x7f];
        }
    }
    if (p == nullptr || p->bits < 0) {
        error(errSyntaxError, str->getPos(), "Bad two dim code in JBIG2 MMR stream");
        return EOF;
    }
    bufLen -= p->bits;
    return p->n;
}

int JBIG2MMRDecoder::getWhiteCode()
{
    const CCITTCode *p;
    unsigned int code;