// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 24/36



        // check for all-zero AC coefficients
        if (p[1 * 8] == 0 && p[2 * 8] == 0 && p[3 * 8] == 0 && p[4 * 8] == 0 && p[5 * 8] == 0 && p[6 * 8] == 0 && p[7 * 8] == 0) {
            t = (dctSqrt2 * dataIn[i + 0] + 8192) >> 14;
            p[0 * 8] = t;
            p[1 * 8] = t;
            p[2 * 8] = t;
            p[3 * 8] = t;
            p[4 * 8] = t;
            p[5 * 8] = t;
            p[6 * 8] = t;
            p[7 * 8] = t;
            continue;
        }

        // stage 4
        v0 = (dctSqrt2 * p[0 * 8] + 2048) >> 12;
        v1 = (dctSqrt2 * p[4 * 8] + 2048) >> 12;
        v2 = p[2 * 8];
        v3 = p[6 * 8];
        v4 = (dctSqrt1d2 * (p[1 * 8] - p[7 * 8]) + 2048) >> 12;
        v7 = (dctSqrt1d2 * (p[1 * 8] + p[7 * 8]) + 2048) >> 12;
        v5 = p[3 * 8];
        v6 = p[5 * 8];

        // stage 3
        t = (v0 - v1 + 1) >> 1;
        v0 = (v0 + v1 + 1) >> 1;
        v1 = t;
        t = (v2 * dctSin6 + v3 * dctCos6 + 2048) >> 12;
        v2 = (v2 * dctCos6 - v3 * dctSin6 + 2048) >> 12;
        v3 = t;
        t = (v4 - v6 + 1) >> 1;
        v4 = (v4 + v6 + 1) >> 1;
        v6 = t;
        t = (v7 + v5 + 1) >> 1;
        v5 = (v7 - v5 + 1) >> 1;
        v7 = t;

        // stage 2
        t = (v0 - v3 + 1) >> 1;
        v0 = (v0 + v3 + 1) >> 1;
        v3 = t;
        t = (v1 - v2 + 1) >> 1;
        v1 = (v1 + v2 + 1) >> 1;
        v2 = t;
        t = (v4 * dctSin3 + v7 * dctCos3 + 2048) >> 12;
        v4 = (v4 * dctCos3 - v7 * dctSin3 + 2048) >> 12;
        v7 = t;
        t = (v5 * dctSin1 + v6 * dctCos1 + 2048) >> 12;
        v5 = (v5 * dctCos1 - v6 * dctSin1 + 2048) >> 12;
        v6 = t;

        // stage 1
        p[0 * 8] = v0 + v7;
        p[7 * 8] = v0 - v7;
        p[1 * 8] = v1 + v6;
        p[6 * 8] = v1 - v6;
        p[2 * 8] = v2 + v5;
        p[5 * 8] = v2 - v5;
        p[3 * 8] = v3 + v4;
        p[4 * 8] = v3 - v4;
    }

    // convert to 8-bit integers
    for (i = 0; i < 64; ++i) {
        const int ix = dctClipOffset + 128 + ((dataIn[i] + 8) >> 4);
        if (unlikely(ix < 0 || ix >= dctClipLength)) {
            dataOut[i] = 0;
        } else {
            dataOut[i] = dctClip[ix];
        }
    }
}

int DCTStream::readHuffSym(DCTHuffTable *table)
{
    unsigned short code;
    int bit;
    int codeBits;

    code = 0;
    codeBits = 0;
    do {
        // add a bit to the code
        if ((bit = readBit()) == EOF) {
            return 9999;
        }
        code = (code << 1) + bit;
        ++codeBits;

        // look up code
        if (code < table->firstCode[codeBits]) {
            break;
        }
        if (code - table->firstCode[codeBits] < table->numCodes[codeBits]) {
            code -= table->firstCode[codeBits];
            return table->sym[table->firstSym[codeBits] + code];
        }
    } while (codeBits < 16);

    error(errSyntaxError, getPos(), "Bad Huffman code in DCT stream");
    return 9999;
}

int DCTStream::readAmp(int size)
{
    int amp, bit;
    int bits;

    amp = 0;
    for (bits = 0; bits < size; ++bits) {
        if ((bit = readBit()) == EOF)
            return 9999;
        amp = (amp << 1) + bit;
    }
    if (amp < (1 << (size - 1)))
        amp -= (1 << size) - 1;
    return amp;
}

int DCTStream::readBit()
{
    int bit;
    int c, c2;

    if (inputBits == 0) {
        if ((c = str->getChar()) == EOF)
            return EOF;
        if (c == 0xff) {
            do {
                c2 = str->getChar();
            } while (c2 == 0xff);
            if (c2 != 0x00) {
                error(errSyntaxError, getPos(), "Bad DCT data: missing 00 after ff");
                return EOF;
            }
        }
        inputBuf = c;
        inputBits = 8;
    }
    bit = (inputBuf >> (inputBits - 1)) & 1;
    --inputBits;
    return bit;
}

bool DCTStream::readHeader()
{
    bool doScan;
    int n;
    int c = 0;
    int i;