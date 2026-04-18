// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 21/36



            // zero run and one AC coefficient
        } else {
            run = (c >> 4) & 0x0f;
            size = c & 0x0f;
            if ((amp = readAmp(size)) == 9999) {
                return false;
            }
            j = 0; // make gcc happy
            for (k = 0; k <= run && i <= scanInfo.lastCoeff; ++k) {
                j = dctZigZag[i++];
                while (data[j] != 0 && i <= scanInfo.lastCoeff) {
                    if ((bit = readBit()) == EOF) {
                        return false;
                    }
                    if (bit) {
                        data[j] += 1 << scanInfo.al;
                    }
                    j = dctZigZag[i++];
                }
            }
            data[j] = amp << scanInfo.al;
        }
    }

    return true;
}

// Decode a progressive JPEG image.
void DCTStream::decodeImage()
{
    int dataIn[64];
    unsigned char dataOut[64];
    unsigned short *quantTable;
    int pY, pCb, pCr, pR, pG, pB;
    int x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, cc, i;
    int h, v, horiz, vert, hSub, vSub;
    int *p0, *p1, *p2;

    for (y1 = 0; y1 < bufHeight; y1 += mcuHeight) {
        for (x1 = 0; x1 < bufWidth; x1 += mcuWidth) {
            for (cc = 0; cc < numComps; ++cc) {
                quantTable = quantTables[compInfo[cc].quantTable];
                h = compInfo[cc].hSample;
                v = compInfo[cc].vSample;
                horiz = mcuWidth / h;
                vert = mcuHeight / v;
                hSub = horiz / 8;
                vSub = vert / 8;
                for (y2 = 0; y2 < mcuHeight; y2 += vert) {
                    for (x2 = 0; x2 < mcuWidth; x2 += horiz) {

                        // pull out the coded data unit
                        p1 = &frameBuf[cc][(y1 + y2) * bufWidth + (x1 + x2)];
                        for (y3 = 0, i = 0; y3 < 8; ++y3, i += 8) {
                            dataIn[i] = p1[0];
                            dataIn[i + 1] = p1[1];
                            dataIn[i + 2] = p1[2];
                            dataIn[i + 3] = p1[3];
                            dataIn[i + 4] = p1[4];
                            dataIn[i + 5] = p1[5];
                            dataIn[i + 6] = p1[6];
                            dataIn[i + 7] = p1[7];
                            p1 += bufWidth * vSub;
                        }

                        // transform
                        transformDataUnit(quantTable, dataIn, dataOut);