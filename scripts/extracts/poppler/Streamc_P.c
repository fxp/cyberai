// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 16/36



    code = 0; // make gcc happy
    if (endOfBlock) {
        code = lookBits(13);
        if (code == EOF) {
            return 1;
        }
        if ((code >> 7) == 0) {
            p = &blackTab1[code];
        } else if ((code >> 9) == 0 && (code >> 7) != 0) {
            p = &blackTab2[(code >> 1) - 64];
        } else {
            p = &blackTab3[code >> 7];
        }
        if (p->bits > 0) {
            eatBits(p->bits);
            return p->n;
        }
    } else {
        for (n = 2; n <= 6; ++n) {
            code = lookBits(n);
            if (code == EOF) {
                return 1;
            }
            if (n < 6) {
                code <<= 6 - n;
            }
            p = &blackTab3[code];
            if (p->bits == n) {
                eatBits(n);
                return p->n;
            }
        }
        for (n = 7; n <= 12; ++n) {
            code = lookBits(n);
            if (code == EOF) {
                return 1;
            }
            if (n < 12) {
                code <<= 12 - n;
            }
            if (code >= 64) {
                p = &blackTab2[code - 64];
                if (p->bits == n) {
                    eatBits(n);
                    return p->n;
                }
            }
        }
        for (n = 10; n <= 13; ++n) {
            code = lookBits(n);
            if (code == EOF) {
                return 1;
            }
            if (n < 13) {
                code <<= 13 - n;
            }
            p = &blackTab1[code];
            if (p->bits == n) {
                eatBits(n);
                return p->n;
            }
        }
    }
    error(errSyntaxError, getPos(), "Bad black code ({0:04x}) in CCITTFax stream", code);
    // eat a bit and return a positive number so that the caller doesn't
    // go into an infinite loop
    eatBits(1);
    return 1;
}

short CCITTFaxStream::lookBits(int n)
{
    int c;

    while (inputBits < n) {
        if ((c = str->getChar()) == EOF) {
            if (inputBits == 0) {
                return EOF;
            }
            // near the end of the stream, the caller may ask for more bits
            // than are available, but there may still be a valid code in
            // however many bits are available -- we need to return correct
            // data in this case
            return (inputBuf << (n - inputBits)) & (0xffffffff >> (32 - n));
        }
        inputBuf = (inputBuf << 8) + c;
        inputBits += 8;
    }
    return (inputBuf >> (inputBits - n)) & (0xffffffff >> (32 - n));
}

std::optional<std::string> CCITTFaxStream::getPSFilter(int psLevel, const char *indent)
{
    std::optional<std::string> s;
    char s1[50];

    if (psLevel < 2) {
        return {};
    }
    if (!(s = str->getPSFilter(psLevel, indent))) {
        return {};
    }
    s->append(indent).append("<< ");
    if (encoding != 0) {
        sprintf(s1, "/K %d ", encoding);
        s->append(s1);
    }
    if (endOfLine) {
        s->append("/EndOfLine true ");
    }
    if (byteAlign) {
        s->append("/EncodedByteAlign true ");
    }
    sprintf(s1, "/Columns %d ", columns);
    s->append(s1);
    if (rows != 0) {
        sprintf(s1, "/Rows %d ", rows);
        s->append(s1);
    }
    if (!endOfBlock) {
        s->append("/EndOfBlock false ");
    }
    if (black) {
        s->append("/BlackIs1 true ");
    }
    s->append(">> /CCITTFaxDecode filter\n");
    return s;
}

bool CCITTFaxStream::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}

#if !ENABLE_LIBJPEG

//------------------------------------------------------------------------
// DCTStream
//------------------------------------------------------------------------

// IDCT constants (20.12 fixed point format)
#    define dctCos1 4017 // cos(pi/16)
#    define dctSin1 799 // sin(pi/16)
#    define dctCos3 3406 // cos(3*pi/16)
#    define dctSin3 2276 // sin(3*pi/16)
#    define dctCos6 1567 // cos(6*pi/16)
#    define dctSin6 3784 // sin(6*pi/16)
#    define dctSqrt2 5793 // sqrt(2)
#    define dctSqrt1d2 2896 // sqrt(2) / 2

// color conversion parameters (16.16 fixed point format)
#    define dctCrToR 91881 //  1.4020
#    define dctCbToG -22553 // -0.3441363
#    define dctCrToG -46802 // -0.71413636
#    define dctCbToB 116130 //  1.772

// clip [-256,511] --> [0,255]
#    define dctClipOffset 256
#    define dctClipLength 768
static unsigned char dctClip[dctClipLength];
static int dctClipInit = 0;

// zig zag decode map
static const int dctZigZag[64] = { 0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
                                   35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63 };

DCTStream::DCTStream(std::unique_ptr<Stream> strA, int colorXformA, Dict * /*dict*/, int /*recursion*/) : OwnedFilterStream(std::move(strA))
{
    int i, j;