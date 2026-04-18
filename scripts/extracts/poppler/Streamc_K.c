// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 11/36



CCITTFaxStream::CCITTFaxStream(std::unique_ptr<Stream> strA, int encodingA, bool endOfLineA, bool byteAlignA, int columnsA, int rowsA, bool endOfBlockA, bool blackA, int damagedRowsBeforeErrorA) : OwnedFilterStream(std::move(strA))
{
    encoding = encodingA;
    endOfLine = endOfLineA;
    byteAlign = byteAlignA;
    columns = columnsA;
    damagedRowsBeforeError = damagedRowsBeforeErrorA;
    if (columns < 1) {
        columns = 1;
    } else if (columns > INT_MAX - 2) {
        columns = INT_MAX - 2;
    }
    rows = rowsA;
    endOfBlock = endOfBlockA;
    black = blackA;
    // 0 <= codingLine[0] < codingLine[1] < ... < codingLine[n] = columns
    // ---> max codingLine size = columns + 1
    // refLine has one extra guard entry at the end
    // ---> max refLine size = columns + 2
    codingLine = (int *)gmallocn_checkoverflow(columns + 1, sizeof(int));
    refLine = (int *)gmallocn_checkoverflow(columns + 2, sizeof(int));

    if (codingLine != nullptr && refLine != nullptr) {
        eof = false;
        codingLine[0] = columns;
    } else {
        eof = true;
    }
    row = 0;
    nextLine2D = encoding < 0;
    inputBits = 0;
    a0i = 0;
    outputBits = 0;

    buf = EOF;
}

CCITTFaxStream::~CCITTFaxStream()
{
    gfree(refLine);
    gfree(codingLine);
}

bool CCITTFaxStream::ccittRewind(bool unfiltered)
{
    row = 0;
    nextLine2D = encoding < 0;
    inputBits = 0;
    a0i = 0;
    outputBits = 0;
    buf = EOF;

    if (unfiltered) {
        return str->unfilteredRewind();
    }
    return str->rewind();
}

bool CCITTFaxStream::unfilteredRewind()
{
    return ccittRewind(true);
}

bool CCITTFaxStream::rewind()
{
    int code1;

    bool rewindSuccess = ccittRewind(false);

    if (codingLine != nullptr && refLine != nullptr) {
        eof = false;
        codingLine[0] = columns;
    } else {
        eof = true;
    }

    // skip any initial zero bits and end-of-line marker, and get the 2D
    // encoding tag
    while ((code1 = lookBits(12)) == 0) {
        eatBits(1);
    }
    if (code1 == 0x001) {
        eatBits(12);
        endOfLine = true;
    }
    if (encoding > 0) {
        nextLine2D = !lookBits(1);
        eatBits(1);
    }

    return rewindSuccess;
}

inline void CCITTFaxStream::addPixels(int a1, int blackPixels)
{
    if (a1 > codingLine[a0i]) {
        if (a1 > columns) {
            error(errSyntaxError, getPos(), "CCITTFax row is wrong length ({0:d})", a1);
            err = true;
            a1 = columns;
        }
        if ((a0i & 1) ^ blackPixels) {
            ++a0i;
        }
        codingLine[a0i] = a1;
    }
}

inline void CCITTFaxStream::addPixelsNeg(int a1, int blackPixels)
{
    if (a1 > codingLine[a0i]) {
        if (a1 > columns) {
            error(errSyntaxError, getPos(), "CCITTFax row is wrong length ({0:d})", a1);
            err = true;
            a1 = columns;
        }
        if ((a0i & 1) ^ blackPixels) {
            ++a0i;
        }
        codingLine[a0i] = a1;
    } else if (a1 < codingLine[a0i]) {
        if (a1 < 0) {
            error(errSyntaxError, getPos(), "Invalid CCITTFax code");
            err = true;
            a1 = columns;
        }
        while (a0i > 0 && a1 <= codingLine[a0i - 1]) {
            --a0i;
        }
        codingLine[a0i] = a1;
    }
}

int CCITTFaxStream::lookChar()
{
    int code1, code2, code3;
    int b1i, blackPixels, i, bits;
    bool gotEOL;

    if (buf != EOF) {
        return buf;
    }

    // read the next row
    if (outputBits == 0) {

        // if at eof just return EOF
        if (eof) {
            return EOF;
        }

        err = false;