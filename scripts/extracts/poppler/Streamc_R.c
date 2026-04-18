// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 18/36



int DCTStream::lookChar()
{
    if (y >= height) {
        return EOF;
    }
    if (progressive || !interleaved) {
        return frameBuf[comp][y * bufWidth + x];
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
        return rowBuf[comp][dy][x];
    }
}

void DCTStream::restart()
{
    int i;

    inputBits = 0;
    restartCtr = restartInterval;
    for (i = 0; i < numComps; ++i) {
        compInfo[i].prevDC = 0;
    }
    eobRun = 0;
}

// Read one row of MCUs from a sequential JPEG stream.
bool DCTStream::readMCURow()
{
    int data1[64];
    unsigned char data2[64];
    unsigned char *p1, *p2;
    int pY, pCb, pCr, pR, pG, pB;
    int h, v, horiz, vert, hSub, vSub;
    int x1, x2, y2, x3, y3, x4, y4, x5, y5, cc, i;
    int c;

    for (x1 = 0; x1 < width; x1 += mcuWidth) {

        // deal with restart marker
        if (restartInterval > 0 && restartCtr == 0) {
            c = readMarker();
            if (c != restartMarker) {
                error(errSyntaxError, getPos(), "Bad DCT data: incorrect restart marker");
                return false;
            }
            if (++restartMarker == 0xd8)
                restartMarker = 0xd0;
            restart();
        }

        // read one MCU
        for (cc = 0; cc < numComps; ++cc) {
            h = compInfo[cc].hSample;
            v = compInfo[cc].vSample;
            horiz = mcuWidth / h;
            vert = mcuHeight / v;
            hSub = horiz / 8;
            vSub = vert / 8;
            for (y2 = 0; y2 < mcuHeight; y2 += vert) {
                for (x2 = 0; x2 < mcuWidth; x2 += horiz) {
                    if (unlikely(scanInfo.dcHuffTable[cc] >= 4) || unlikely(scanInfo.acHuffTable[cc] >= 4)) {
                        return false;
                    }
                    if (!readDataUnit(&dcHuffTables[scanInfo.dcHuffTable[cc]], &acHuffTables[scanInfo.acHuffTable[cc]], &compInfo[cc].prevDC, data1)) {
                        return false;
                    }
                    transformDataUnit(quantTables[compInfo[cc].quantTable], data1, data2);
                    if (hSub == 1 && vSub == 1) {
                        for (y3 = 0, i = 0; y3 < 8; ++y3, i += 8) {
                            p1 = &rowBuf[cc][y2 + y3][x1 + x2];
                            p1[0] = data2[i];
                            p1[1] = data2[i + 1];
                            p1[2] = data2[i + 2];
                            p1[3] = data2[i + 3];
                            p1[4] = data2[i + 4];
                            p1[5] = data2[i + 5];
                            p1[6] = data2[i + 6];
                            p1[7] = data2[i + 7];
                        }
                    } else if (hSub == 2 && vSub == 2) {
                        for (y3 = 0, i = 0; y3 < 16; y3 += 2, i += 8) {
                            p1 = &rowBuf[cc][y2 + y3][x1 + x2];
                            p2 = &rowBuf[cc][y2 + y3 + 1][x1 + x2];
                            p1[0] = p1[1] = p2[0] = p2[1] = data2[i];
                            p1[2] = p1[3] = p2[2] = p2[3] = data2[i + 1];
                            p1[4] = p1[5] = p2[4] = p2[5] = data2[i + 2];
                            p1[6] = p1[7] = p2[6] = p2[7] = data2[i + 3];
                            p1[8] = p1[9] = p2[8] = p2[9] = data2[i + 4];
                            p1[10] = p1[11] = p2[10] = p2[11] = data2[i + 5];
                            p1[12] = p1[13] = p2[12] = p2[13] = data2[i + 6];
                            p1[14] = p1[15] = p2[14] = p2[15] = data2[i + 7];
                        }
                    } else {
                        i = 0;
                        for (y3 = 0, y4 = 0; y3 < 8; ++y3, y4 += vSub) {
                            for (x3 = 0, x4 = 0; x3 < 8; ++x3, x4 += hSub) {
                                for (y5 = 0; y5 < vSub; ++y5)
                                    for (x5 = 0; x5 < hSub; ++x5)
                                        rowBuf[cc][y2 + y4 + y5][x1 + x2 + x4 + x5] = data2[i];
                                ++i;
                            }
                        }
                    }
                }
            }
        }
        --restartCtr;