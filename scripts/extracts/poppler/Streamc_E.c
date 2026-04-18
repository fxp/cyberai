// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 5/36



    nVals = width * nComps;
    inputLineSize = (nVals * nBits + 7) >> 3;
    if (nComps <= 0 || nBits <= 0 || nVals > INT_MAX / nBits - 7 || width > INT_MAX / nComps) {
        inputLineSize = -1;
    }
    inputLine = (unsigned char *)gmallocn_checkoverflow(inputLineSize, sizeof(char));
    if (nBits == 8) {
        imgLine = inputLine;
    } else {
        if (nBits == 1) {
            imgLineSize = (nVals + 7) & ~7;
        } else {
            imgLineSize = nVals;
        }
        if (nComps <= 0 || width > INT_MAX / nComps) {
            imgLineSize = -1;
        }
        imgLine = (unsigned char *)gmallocn_checkoverflow(imgLineSize, sizeof(unsigned char));
    }
    imgIdx = nVals;
}

ImageStream::~ImageStream()
{
    if (imgLine != inputLine) {
        gfree(imgLine);
    }
    gfree(inputLine);
}

bool ImageStream::rewind()
{
    return str->rewind();
}

void ImageStream::close()
{
    str->close();
}

bool ImageStream::getPixel(unsigned char *pix)
{
    int i;

    if (imgIdx >= nVals) {
        if (!getLine()) {
            return false;
        }
        imgIdx = 0;
    }
    for (i = 0; i < nComps; ++i) {
        pix[i] = imgLine[imgIdx++];
    }
    return true;
}

unsigned char *ImageStream::getLine()
{
    if (unlikely(inputLine == nullptr || imgLine == nullptr)) {
        return nullptr;
    }

    int readChars = str->doGetChars(inputLineSize, inputLine);
    if (unlikely(readChars == -1)) {
        readChars = 0;
    }
    for (; readChars < inputLineSize; readChars++) {
        inputLine[readChars] = EOF;
    }
    if (nBits == 1) {
        unsigned char *p = inputLine;
        for (int i = 0; i < nVals; i += 8) {
            const int c = *p++;
            imgLine[i + 0] = (unsigned char)((c >> 7) & 1);
            imgLine[i + 1] = (unsigned char)((c >> 6) & 1);
            imgLine[i + 2] = (unsigned char)((c >> 5) & 1);
            imgLine[i + 3] = (unsigned char)((c >> 4) & 1);
            imgLine[i + 4] = (unsigned char)((c >> 3) & 1);
            imgLine[i + 5] = (unsigned char)((c >> 2) & 1);
            imgLine[i + 6] = (unsigned char)((c >> 1) & 1);
            imgLine[i + 7] = (unsigned char)(c & 1);
        }
    } else if (nBits == 8) {
        // special case: imgLine == inputLine
    } else if (nBits == 16) {
        // this is a hack to support 16 bits images, everywhere
        // we assume a component fits in 8 bits, with this hack
        // we treat 16 bit images as 8 bit ones until it's fixed correctly.
        // The hack has another part on GfxImageColorMap::GfxImageColorMap
        unsigned char *p = inputLine;
        for (int i = 0; i < nVals; ++i) {
            imgLine[i] = *p++;
            p++;
        }
    } else {
        const unsigned long bitMask = (1 << nBits) - 1;
        unsigned long buf = 0;
        int bits = 0;
        unsigned char *p = inputLine;
        for (int i = 0; i < nVals; ++i) {
            while (bits < nBits) {
                buf = (buf << 8) | (*p++ & 0xff);
                bits += 8;
            }
            imgLine[i] = (unsigned char)((buf >> (bits - nBits)) & bitMask);
            bits -= nBits;
        }
    }
    return imgLine;
}

void ImageStream::skipLine()
{
    str->doGetChars(inputLineSize, inputLine);
}

//------------------------------------------------------------------------
// StreamPredictor
//------------------------------------------------------------------------

StreamPredictor::StreamPredictor(Stream *strA, int predictorA, int widthA, int nCompsA, int nBitsA)
{
    str = strA;
    predictor = predictorA;
    width = widthA;
    nComps = nCompsA;
    nBits = nBitsA;
    predLine = nullptr;
    ok = false;

    if (checkedMultiply(width, nComps, &nVals)) {
        return;
    }
    if (width <= 0 || nComps <= 0 || nBits <= 0 || nComps > gfxColorMaxComps || nBits > 16 || nVals >= (INT_MAX - 7) / nBits) { // check for overflow in rowBytes
        return;
    }
    pixBytes = (nComps * nBits + 7) >> 3;
    rowBytes = ((nVals * nBits + 7) >> 3) + pixBytes;
    predLine = (unsigned char *)gmalloc(rowBytes);
    memset(predLine, 0, rowBytes);
    predIdx = rowBytes;

    ok = true;
}

StreamPredictor::~StreamPredictor()
{
    gfree(predLine);
}

int StreamPredictor::lookChar()
{
    if (predIdx >= rowBytes) {
        if (!getNextLine()) {
            return EOF;
        }
    }
    return predLine[predIdx];
}

int StreamPredictor::getChar()
{
    if (predIdx >= rowBytes) {
        if (!getNextLine()) {
            return EOF;
        }
    }
    return predLine[predIdx++];
}

int StreamPredictor::getChars(int nChars, unsigned char *buffer)
{
    int n, m;

    n = 0;
    while (n < nChars) {
        if (predIdx >= rowBytes) {
            if (!getNextLine()) {
                break;
            }
        }
        m = rowBytes - predIdx;
        if (m > nChars - n) {
            m = nChars - n;
        }
        memcpy(buffer + n, predLine + predIdx, m);
        predIdx += m;
        n += m;
    }
    return n;
}