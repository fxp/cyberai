// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 28/31



    // horizontal (row) transforms
    if (r == tileComp->nDecompLevels) {
        offset = 3 + (tileComp->x0 & 1);
    } else {
        offset = 3 + (tileComp->resLevels[r + 1].x0 & 1);
    }
    for (y = 0, dataPtr = tileComp->data; y < ny2; ++y, dataPtr += tileComp->w) {
        if (precinct->subbands[0].x0 == precinct->subbands[1].x0) {
            // fetch LL/LH
            for (x = 0, bufPtr = tileComp->buf + offset; x < nx1; ++x, bufPtr += 2) {
                *bufPtr = dataPtr[x];
            }
            // fetch HL/HH
            for (x = nx1, bufPtr = tileComp->buf + offset + 1; x < nx2; ++x, bufPtr += 2) {
                *bufPtr = dataPtr[x];
            }
        } else {
            // fetch LL/LH
            for (x = 0, bufPtr = tileComp->buf + offset + 1; x < nx1; ++x, bufPtr += 2) {
                *bufPtr = dataPtr[x];
            }
            // fetch HL/HH
            for (x = nx1, bufPtr = tileComp->buf + offset; x < nx2; ++x, bufPtr += 2) {
                *bufPtr = dataPtr[x];
            }
        }
        if (tileComp->x1 - tileComp->x0 > tileComp->y1 - tileComp->y0) {
            x = tileComp->x1 - tileComp->x0 + 5;
        } else {
            x = tileComp->y1 - tileComp->y0 + 5;
        }
        if (offset + nx2 > x || nx2 == 0) {
            error(errSyntaxError, getPos(), "Invalid call of inverseTransform1D in inverseTransformLevel in JPX stream");
            return;
        }
        inverseTransform1D(tileComp, tileComp->buf, offset, nx2);
        for (x = 0, bufPtr = tileComp->buf + offset; x < nx2; ++x, ++bufPtr) {
            dataPtr[x] = *bufPtr;
        }
    }

    // vertical (column) transforms
    if (r == tileComp->nDecompLevels) {
        offset = 3 + (tileComp->y0 & 1);
    } else {
        offset = 3 + (tileComp->resLevels[r + 1].y0 & 1);
    }
    for (x = 0, dataPtr = tileComp->data; x < nx2; ++x, ++dataPtr) {
        if (precinct->subbands[1].y0 == precinct->subbands[0].y0) {
            // fetch LL/HL
            for (y = 0, bufPtr = tileComp->buf + offset; y < ny1; ++y, bufPtr += 2) {
                *bufPtr = dataPtr[y * tileComp->w];
            }
            // fetch LH/HH
            for (y = ny1, bufPtr = tileComp->buf + offset + 1; y < ny2; ++y, bufPtr += 2) {
                *bufPtr = dataPtr[y * tileComp->w];
            }
        } else {
            // fetch LL/HL
            for (y = 0, bufPtr = tileComp->buf + offset + 1; y < ny1; ++y, bufPtr += 2) {
                *bufPtr = dataPtr[y * tileComp->w];
            }
            // fetch LH/HH
            for (y = ny1, bufPtr = tileComp->buf + offset; y < ny2; ++y, bufPtr += 2) {
                *bufPtr = dataPtr[y * tileComp->w];
            }
        }
        if (tileComp->x1 - tileComp->x0 > tileComp->y1 - tileComp->y0) {
            y = tileComp->x1 - tileComp->x0 + 5;
        } else {
            y = tileComp->y1 - tileComp->y0 + 5;
        }
        if (offset + ny2 > y || ny2 == 0) {
            error(errSyntaxError, getPos(), "Invalid call of inverseTransform1D in inverseTransformLevel in JPX stream");
            return;
        }
        inverseTransform1D(tileComp, tileComp->buf, offset, ny2);
        for (y = 0, bufPtr = tileComp->buf + offset; y < ny2; ++y, ++bufPtr) {
            dataPtr[y * tileComp->w] = *bufPtr;
        }
    }
}

void JPXStream::inverseTransform1D(JPXTileComp *tileComp, int *data, unsigned int offset, unsigned int n)
{
    unsigned int end, i;

    //----- special case for length = 1
    if (n == 1) {
        cover(79);
        if (offset == 4) {
            cover(104);
            *data >>= 1;
        }

    } else {
        cover(80);

        end = offset + n;

        //----- extend right
        data[end] = data[end - 2];
        if (n == 2) {
            cover(81);
            data[end + 1] = data[offset + 1];
            data[end + 2] = data[offset];
            data[end + 3] = data[offset + 1];
        } else {
            cover(82);
            data[end + 1] = data[end - 3];
            if (n == 3) {
                cover(105);
                data[end + 2] = data[offset + 1];
                data[end + 3] = data[offset + 2];
            } else {
                cover(106);
                data[end + 2] = data[end - 4];
                if (n == 4) {
                    cover(107);
                    data[end + 3] = data[offset + 1];
                } else {
                    cover(108);
                    data[end + 3] = data[end - 5];
                }
            }
        }

        //----- extend left
        data[offset - 1] = data[offset + 1];
        data[offset - 2] = data[offset + 2];
        data[offset - 3] = data[offset + 3];
        if (offset == 4) {
            cover(83);
            data[0] = data[offset + 4];
        }

        //----- 9-7 irreversible filter