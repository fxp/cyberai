// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 6/36



bool StreamPredictor::getNextLine()
{
    int curPred;
    unsigned char upLeftBuf[gfxColorMaxComps * 2 + 1];
    int left, up, upLeft, p, pa, pb, pc;
    int c;
    unsigned long inBuf, outBuf;
    int inBits, outBits;
    int i, j, k, kk;

    // get PNG optimum predictor number
    if (predictor >= 10) {
        if ((curPred = str->getRawChar()) == EOF) {
            return false;
        }
        curPred += 10;
    } else {
        curPred = predictor;
    }

    // read the raw line, apply PNG (byte) predictor
    int *rawCharLine = new int[rowBytes - pixBytes];
    str->getRawChars(rowBytes - pixBytes, rawCharLine);
    memset(upLeftBuf, 0, pixBytes + 1);
    for (i = pixBytes; i < rowBytes; ++i) {
        for (j = pixBytes; j > 0; --j) {
            upLeftBuf[j] = upLeftBuf[j - 1];
        }
        upLeftBuf[0] = predLine[i];
        if ((c = rawCharLine[i - pixBytes]) == EOF) {
            if (i > pixBytes) {
                // this ought to return false, but some (broken) PDF files
                // contain truncated image data, and Adobe apparently reads the
                // last partial line
                break;
            }
            delete[] rawCharLine;
            return false;
        }
        switch (curPred) {
        case 11: // PNG sub
            predLine[i] = predLine[i - pixBytes] + (unsigned char)c;
            break;
        case 12: // PNG up
            predLine[i] = predLine[i] + (unsigned char)c;
            break;
        case 13: // PNG average
            predLine[i] = ((predLine[i - pixBytes] + predLine[i]) >> 1) + (unsigned char)c;
            break;
        case 14: // PNG Paeth
            left = predLine[i - pixBytes];
            up = predLine[i];
            upLeft = upLeftBuf[pixBytes];
            p = left + up - upLeft;
            if ((pa = p - left) < 0) {
                pa = -pa;
            }
            if ((pb = p - up) < 0) {
                pb = -pb;
            }
            if ((pc = p - upLeft) < 0) {
                pc = -pc;
            }
            if (pa <= pb && pa <= pc) {
                predLine[i] = left + (unsigned char)c;
            } else if (pb <= pc) {
                predLine[i] = up + (unsigned char)c;
            } else {
                predLine[i] = upLeft + (unsigned char)c;
            }
            break;
        case 10: // PNG none
        default: // no predictor or TIFF predictor
            predLine[i] = (unsigned char)c;
            break;
        }
    }
    delete[] rawCharLine;

    // apply TIFF (component) predictor
    if (predictor == 2) {
        if (nBits == 1 && nComps == 1) {
            inBuf = predLine[pixBytes - 1];
            for (i = pixBytes; i < rowBytes; ++i) {
                c = predLine[i] ^ inBuf;
                c ^= c >> 1;
                c ^= c >> 2;
                c ^= c >> 4;
                inBuf = (c & 1) << 7;
                predLine[i] = c;
            }
        } else if (nBits == 8) {
            for (i = pixBytes; i < rowBytes; ++i) {
                predLine[i] += predLine[i - nComps];
            }
        } else {
            memset(upLeftBuf, 0, nComps + 1);
            const unsigned long bitMask = (1 << nBits) - 1;
            inBuf = outBuf = 0;
            inBits = outBits = 0;
            j = k = pixBytes;
            for (i = 0; i < width; ++i) {
                for (kk = 0; kk < nComps; ++kk) {
                    while (inBits < nBits) {
                        inBuf = (inBuf << 8) | (predLine[j++] & 0xff);
                        inBits += 8;
                    }
                    upLeftBuf[kk] = (unsigned char)((upLeftBuf[kk] + (inBuf >> (inBits - nBits))) & bitMask);
                    inBits -= nBits;
                    outBuf = (outBuf << nBits) | upLeftBuf[kk];
                    outBits += nBits;
                    if (outBits >= 8) {
                        predLine[k++] = (unsigned char)(outBuf >> (outBits - 8));
                        outBits -= 8;
                    }
                }
            }
            if (outBits > 0) {
                predLine[k++] = (unsigned char)((outBuf << (8 - outBits)) + (inBuf & ((1 << (8 - outBits)) - 1)));
            }
        }
    }

    // rewind to start of line
    predIdx = pixBytes;

    return true;
}

//------------------------------------------------------------------------
// FileStream
//------------------------------------------------------------------------

FileStream::FileStream(GooFile *fileA, Goffset startA, bool limitedA, Goffset lengthA, Object &&dictA) : BaseStream(std::move(dictA), lengthA)
{
    file = fileA;
    offset = start = startA;
    limited = limitedA;
    length = lengthA;
    bufPtr = bufEnd = buf;
    bufPos = start;
    savePos = 0;
    saved = false;
    needsEncryptionOnSave = false;
}

FileStream::~FileStream()
{
    close();
}

std::unique_ptr<BaseStream> FileStream::copy()
{
    return std::make_unique<FileStream>(file, start, limited, length, dict.copy());
}