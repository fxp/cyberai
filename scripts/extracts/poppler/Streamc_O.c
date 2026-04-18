// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 15/36



    // get a byte
    if (outputBits >= 8) {
        buf = (a0i & 1) ? 0x00 : 0xff;
        outputBits -= 8;
        if (outputBits == 0 && codingLine[a0i] < columns) {
            ++a0i;
            outputBits = codingLine[a0i] - codingLine[a0i - 1];
        }
    } else {
        bits = 8;
        buf = 0;
        do {
            if (outputBits > bits) {
                buf <<= bits;
                if (!(a0i & 1)) {
                    buf |= 0xff >> (8 - bits);
                }
                outputBits -= bits;
                bits = 0;
            } else {
                buf <<= outputBits;
                if (!(a0i & 1)) {
                    buf |= 0xff >> (8 - outputBits);
                }
                bits -= outputBits;
                outputBits = 0;
                if (codingLine[a0i] < columns) {
                    ++a0i;
                    if (unlikely(a0i > columns)) {
                        error(errSyntaxError, getPos(), "Bad bits {0:04x} in CCITTFax stream", bits);
                        err = true;
                        break;
                    }
                    outputBits = codingLine[a0i] - codingLine[a0i - 1];
                } else if (bits > 0) {
                    buf <<= bits;
                    bits = 0;
                }
            }
        } while (bits);
    }
    if (black) {
        buf ^= 0xff;
    }
    return buf;
}

short CCITTFaxStream::getTwoDimCode()
{
    int code;
    const CCITTCode *p;
    int n;

    code = 0; // make gcc happy
    if (endOfBlock) {
        if ((code = lookBits(7)) != EOF) {
            p = &twoDimTab1[code];
            if (p->bits > 0) {
                eatBits(p->bits);
                return p->n;
            }
        }
    } else {
        for (n = 1; n <= 7; ++n) {
            if ((code = lookBits(n)) == EOF) {
                break;
            }
            if (n < 7) {
                code <<= 7 - n;
            }
            p = &twoDimTab1[code];
            if (p->bits == n) {
                eatBits(n);
                return p->n;
            }
        }
    }
    error(errSyntaxError, getPos(), "Bad two dim code ({0:04x}) in CCITTFax stream", code);
    return EOF;
}

short CCITTFaxStream::getWhiteCode()
{
    short code;
    const CCITTCode *p;
    int n;

    code = 0; // make gcc happy
    if (endOfBlock) {
        code = lookBits(12);
        if (code == EOF) {
            return 1;
        }
        if ((code >> 5) == 0) {
            p = &whiteTab1[code];
        } else {
            p = &whiteTab2[code >> 3];
        }
        if (p->bits > 0) {
            eatBits(p->bits);
            return p->n;
        }
    } else {
        for (n = 1; n <= 9; ++n) {
            code = lookBits(n);
            if (code == EOF) {
                return 1;
            }
            if (n < 9) {
                code <<= 9 - n;
            }
            p = &whiteTab2[code];
            if (p->bits == n) {
                eatBits(n);
                return p->n;
            }
        }
        for (n = 11; n <= 12; ++n) {
            code = lookBits(n);
            if (code == EOF) {
                return 1;
            }
            if (n < 12) {
                code <<= 12 - n;
            }
            p = &whiteTab1[code];
            if (p->bits == n) {
                eatBits(n);
                return p->n;
            }
        }
    }
    error(errSyntaxError, getPos(), "Bad white code ({0:04x}) in CCITTFax stream", code);
    // eat a bit and return a positive number so that the caller doesn't
    // go into an infinite loop
    eatBits(1);
    return 1;
}

short CCITTFaxStream::getBlackCode()
{
    short code;
    const CCITTCode *p;
    int n;