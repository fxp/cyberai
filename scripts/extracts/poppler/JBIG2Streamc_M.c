// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 13/38



        // referred-to segment count and retention flags
        if (!readUByte(&refFlags)) {
            error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
            return false;
        }
        nRefSegs = refFlags >> 5;
        if (nRefSegs == 7) {
            if ((c1 = curStr->getChar()) == EOF || (c2 = curStr->getChar()) == EOF || (c3 = curStr->getChar()) == EOF) {
                error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
                return false;
            }
            refFlags = (refFlags << 24) | (c1 << 16) | (c2 << 8) | c3;
            nRefSegs = refFlags & 0x1fffffff;
            const unsigned int nCharsToRead = (nRefSegs + 9) >> 3;
            for (unsigned int i = 0; i < nCharsToRead; ++i) {
                if ((c1 = curStr->getChar()) == EOF) {
                    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
                    return false;
                }
            }
        }

        // referred-to segment numbers
        if (sizeIsBiggerThanVectorMaxSize(nRefSegs, refSegs)) {
            return false;
        }
        refSegs.resize(nRefSegs);
        if (segNum <= 256) {
            for (unsigned int i = 0; i < nRefSegs; ++i) {
                if (!readUByte(&refSegs[i])) {
                    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
                    return false;
                }
            }
        } else if (segNum <= 65536) {
            for (unsigned int i = 0; i < nRefSegs; ++i) {
                if (!readUWord(&refSegs[i])) {
                    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
                    return false;
                }
            }
        } else {
            for (unsigned int i = 0; i < nRefSegs; ++i) {
                if (!readULong(&refSegs[i])) {
                    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
                    return false;
                }
            }
        }

        // segment page association
        if (segFlags & 0x40) {
            if (!readULong(&page)) {
                error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
                return false;
            }
        } else {
            if (!readUByte(&page)) {
                error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
                return false;
            }
        }

        // segment data length
        if (!readULong(&segLength)) {
            error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
            return false;
        }

        // check for missing page information segment
        if (!pageBitmap && ((segType >= 4 && segType <= 7) || (segType >= 20 && segType <= 43))) {
            error(errSyntaxError, curStr->getPos(), "First JBIG2 segment associated with a page must be a page information segment");
            return false;
        }