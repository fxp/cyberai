// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 17/36



    colorXform = colorXformA;
    progressive = interleaved = false;
    width = height = 0;
    mcuWidth = mcuHeight = 0;
    numComps = 0;
    comp = 0;
    x = y = dy = 0;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 32; ++j) {
            rowBuf[i][j] = nullptr;
        }
        frameBuf[i] = nullptr;
    }

    if (!dctClipInit) {
        for (i = -256; i < 0; ++i)
            dctClip[dctClipOffset + i] = 0;
        for (i = 0; i < 256; ++i)
            dctClip[dctClipOffset + i] = i;
        for (i = 256; i < 512; ++i)
            dctClip[dctClipOffset + i] = 255;
        dctClipInit = 1;
    }
}

DCTStream::~DCTStream()
{
    close();
}

bool DCTStream::dctRewind(bool unfiltered)
{
    progressive = interleaved = false;
    width = height = 0;
    numComps = 0;
    numQuantTables = 0;
    numDCHuffTables = 0;
    numACHuffTables = 0;
    gotJFIFMarker = false;
    gotAdobeMarker = false;
    restartInterval = 0;
    if (unfiltered)
        return str->unfilteredRewind();
    else
        return str->rewind();
}

bool DCTStream::unfilteredRewind()
{
    return dctRewind(true);
}

bool DCTStream::rewind()
{
    int i, j;

    bool resetResult = dctRewind(false);

    if (!readHeader()) {
        y = height;
        return false;
    }

    // compute MCU size
    if (numComps == 1) {
        compInfo[0].hSample = compInfo[0].vSample = 1;
    }
    mcuWidth = compInfo[0].hSample;
    mcuHeight = compInfo[0].vSample;
    for (i = 1; i < numComps; ++i) {
        if (compInfo[i].hSample > mcuWidth) {
            mcuWidth = compInfo[i].hSample;
        }
        if (compInfo[i].vSample > mcuHeight) {
            mcuHeight = compInfo[i].vSample;
        }
    }
    mcuWidth *= 8;
    mcuHeight *= 8;

    // figure out color transform
    if (colorXform == -1) {
        if (numComps == 3) {
            if (gotJFIFMarker) {
                colorXform = 1;
            } else if (compInfo[0].id == 82 && compInfo[1].id == 71 && compInfo[2].id == 66) { // ASCII "RGB"
                colorXform = 0;
            } else {
                colorXform = 1;
            }
        } else {
            colorXform = 0;
        }
    }

    if (progressive || !interleaved) {

        // allocate a buffer for the whole image
        bufWidth = ((width + mcuWidth - 1) / mcuWidth) * mcuWidth;
        bufHeight = ((height + mcuHeight - 1) / mcuHeight) * mcuHeight;
        if (bufWidth <= 0 || bufHeight <= 0 || bufWidth > INT_MAX / bufWidth / (int)sizeof(int)) {
            error(errSyntaxError, getPos(), "Invalid image size in DCT stream");
            y = height;
            return false;
        }
        for (i = 0; i < numComps; ++i) {
            frameBuf[i] = (int *)gmallocn(bufWidth * bufHeight, sizeof(int));
            memset(frameBuf[i], 0, bufWidth * bufHeight * sizeof(int));
        }

        // read the image data
        do {
            restartMarker = 0xd0;
            restart();
            readScan();
        } while (readHeader());

        // decode
        decodeImage();

        // initialize counters
        comp = 0;
        x = 0;
        y = 0;

    } else {

        // allocate a buffer for one row of MCUs
        bufWidth = ((width + mcuWidth - 1) / mcuWidth) * mcuWidth;
        for (i = 0; i < numComps; ++i) {
            for (j = 0; j < mcuHeight; ++j) {
                rowBuf[i][j] = (unsigned char *)gmallocn(bufWidth, sizeof(unsigned char));
            }
        }

        // initialize counters
        comp = 0;
        x = 0;
        y = 0;
        dy = mcuHeight;

        restartMarker = 0xd0;
        restart();
    }

    return resetResult;
}

void DCTStream::close()
{
    int i, j;

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 32; ++j) {
            gfree(rowBuf[i][j]);
            rowBuf[i][j] = nullptr;
        }
        gfree(frameBuf[i]);
        frameBuf[i] = nullptr;
    }
    FilterStream::close();
}

int DCTStream::getChar()
{
    int c;

    if (y >= height) {
        return EOF;
    }
    if (progressive || !interleaved) {
        c = frameBuf[comp][y * bufWidth + x];
        if (++comp == numComps) {
            comp = 0;
            if (++x == width) {
                x = 0;
                ++y;
            }
        }
    } else {
        if (dy >= mcuHeight) {
            if (!readMCURow()) {
                y = height;
                return EOF;
            }
            comp = 0;
            x = 0;
            dy = 0;
        }
        c = rowBuf[comp][dy][x];
        if (++comp == numComps) {
            comp = 0;
            if (++x == width) {
                x = 0;
                ++y;
                ++dy;
                if (y == height) {
                    readTrailer();
                }
            }
        }
    }
    return c;
}