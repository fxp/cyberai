// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 20/36



                        // add the data unit into frameBuf
                        p1 = &frameBuf[cc][(y1 + y2) * bufWidth + (x1 + x2)];
                        for (y3 = 0, i = 0; y3 < 8; ++y3, i += 8) {
                            p1[0] = data[i];
                            p1[1] = data[i + 1];
                            p1[2] = data[i + 2];
                            p1[3] = data[i + 3];
                            p1[4] = data[i + 4];
                            p1[5] = data[i + 5];
                            p1[6] = data[i + 6];
                            p1[7] = data[i + 7];
                            p1 += bufWidth * vSub;
                        }
                    }
                }
            }
            --restartCtr;
        }
    }
}

// Read one data unit from a sequential JPEG stream.
bool DCTStream::readDataUnit(DCTHuffTable *dcHuffTable, DCTHuffTable *acHuffTable, int *prevDC, int data[64])
{
    int run, size, amp;
    int c;
    int i, j;

    if ((size = readHuffSym(dcHuffTable)) == 9999) {
        return false;
    }
    if (size > 0) {
        if ((amp = readAmp(size)) == 9999) {
            return false;
        }
    } else {
        amp = 0;
    }
    data[0] = *prevDC += amp;
    for (i = 1; i < 64; ++i) {
        data[i] = 0;
    }
    i = 1;
    while (i < 64) {
        run = 0;
        while ((c = readHuffSym(acHuffTable)) == 0xf0 && run < 0x30) {
            run += 0x10;
        }
        if (c == 9999) {
            return false;
        }
        if (c == 0x00) {
            break;
        } else {
            run += (c >> 4) & 0x0f;
            size = c & 0x0f;
            amp = readAmp(size);
            if (amp == 9999) {
                return false;
            }
            i += run;
            if (i < 64) {
                j = dctZigZag[i++];
                data[j] = amp;
            }
        }
    }
    return true;
}

// Read one data unit from a sequential JPEG stream.
bool DCTStream::readProgressiveDataUnit(DCTHuffTable *dcHuffTable, DCTHuffTable *acHuffTable, int *prevDC, int data[64])
{
    int run, size, amp, bit, c;
    int i, j, k;

    // get the DC coefficient
    i = scanInfo.firstCoeff;
    if (i == 0) {
        if (scanInfo.ah == 0) {
            if ((size = readHuffSym(dcHuffTable)) == 9999) {
                return false;
            }
            if (size > 0) {
                if ((amp = readAmp(size)) == 9999) {
                    return false;
                }
            } else {
                amp = 0;
            }
            data[0] += (*prevDC += amp) << scanInfo.al;
        } else {
            if ((bit = readBit()) == 9999) {
                return false;
            }
            data[0] += bit << scanInfo.al;
        }
        ++i;
    }
    if (scanInfo.lastCoeff == 0) {
        return true;
    }

    // check for an EOB run
    if (eobRun > 0) {
        while (i <= scanInfo.lastCoeff) {
            j = dctZigZag[i++];
            if (data[j] != 0) {
                if ((bit = readBit()) == EOF) {
                    return false;
                }
                if (bit) {
                    data[j] += 1 << scanInfo.al;
                }
            }
        }
        --eobRun;
        return true;
    }

    // read the AC coefficients
    while (i <= scanInfo.lastCoeff) {
        if ((c = readHuffSym(acHuffTable)) == 9999) {
            return false;
        }

        // ZRL
        if (c == 0xf0) {
            k = 0;
            while (k < 16 && i <= scanInfo.lastCoeff) {
                j = dctZigZag[i++];
                if (data[j] == 0) {
                    ++k;
                } else {
                    if ((bit = readBit()) == EOF) {
                        return false;
                    }
                    if (bit) {
                        data[j] += 1 << scanInfo.al;
                    }
                }
            }

            // EOB run
        } else if ((c & 0x0f) == 0x00) {
            j = c >> 4;
            eobRun = 0;
            for (k = 0; k < j; ++k) {
                if ((bit = readBit()) == EOF) {
                    return false;
                }
                eobRun = (eobRun << 1) | bit;
            }
            eobRun += 1 << j;
            while (i <= scanInfo.lastCoeff) {
                j = dctZigZag[i++];
                if (data[j] != 0) {
                    if ((bit = readBit()) == EOF) {
                        return false;
                    }
                    if (bit) {
                        data[j] += 1 << scanInfo.al;
                    }
                }
            }
            --eobRun;
            break;