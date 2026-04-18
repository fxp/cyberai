// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 14/38



        // read the segment data
        arithDecoder->resetByteCounter();
        huffDecoder->resetByteCounter();
        mmrDecoder->resetByteCounter();
        byteCounter = 0;
        switch (segType) {
        case 0:
            if (!readSymbolDictSeg(segNum, refSegs)) {
                return false;
            }
            break;
        case 4:
            if (!readTextRegionSeg(segNum, false, refSegs)) {
                return false;
            }
            break;
        case 6:
        case 7:
            if (!readTextRegionSeg(segNum, true, refSegs)) {
                return false;
            }
            break;
        case 16:
            if (!readPatternDictSeg(segNum, segLength)) {
                return false;
            }
            break;
        case 20:
            if (!readHalftoneRegionSeg(segNum, false, refSegs)) {
                return false;
            }
            break;
        case 22:
        case 23:
            if (!readHalftoneRegionSeg(segNum, true, refSegs)) {
                return false;
            }
            break;
        case 36:
            if (!readGenericRegionSeg(segNum, false, segLength)) {
                return false;
            }
            break;
        case 38:
        case 39:
            if (!readGenericRegionSeg(segNum, true, segLength)) {
                return false;
            }
            break;
        case 40:
            if (!readGenericRefinementRegionSeg(segNum, false, refSegs)) {
                return false;
            }
            break;
        case 42:
        case 43:
            if (!readGenericRefinementRegionSeg(segNum, true, refSegs)) {
                return false;
            }
            break;
        case 48:
            if (!readPageInfoSeg()) {
                return false;
            }
            break;
        case 50:
            if (!readEndOfStripeSeg(segLength)) {
                return false;
            }
            break;
        case 51:
            // end of file segment
            done = true;
            break;
        case 52:
            if (!readProfilesSeg(segLength)) {
                return false;
            }
            break;
        case 53:
            if (!readCodeTableSeg(segNum)) {
                return false;
            }
            break;
        case 62:
            if (!readExtensionSeg(segLength)) {
                return false;
            }
            break;
        default:
            error(errSyntaxError, curStr->getPos(), "Unknown segment type in JBIG2 stream");
            for (unsigned int i = 0; i < segLength; ++i) {
                if ((c1 = curStr->getChar()) == EOF) {
                    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
                    return false;
                }
            }
            break;
        }

        // Make sure the segment handler read all of the bytes in the
        // segment data, unless this segment is marked as having an
        // unknown length (section 7.2.7 of the JBIG2 Final Committee Draft)

        if (segType != 38 || segLength != 0xffffffff) {

            byteCounter += arithDecoder->getByteCounter();
            byteCounter += huffDecoder->getByteCounter();
            byteCounter += mmrDecoder->getByteCounter();

            if (segLength > byteCounter) {
                const unsigned int segExtraBytes = segLength - byteCounter;

                // If we didn't read all of the bytes in the segment data,
                // indicate an error, and throw away the rest of the data.

                // v.3.1.01.13 of the LuraTech PDF Compressor Server will
                // sometimes generate an extraneous NULL byte at the end of
                // arithmetic-coded symbol dictionary segments when numNewSyms
                // == 0.  Segments like this often occur for blank pages.

                error(errSyntaxError, curStr->getPos(), "{0:ud} extraneous byte{1:s} after segment", segExtraBytes, (segExtraBytes > 1) ? "s" : "");
                byteCounter += curStr->discardChars(segExtraBytes);
            } else if (segLength < byteCounter) {

                // If we read more bytes than we should have, according to the
                // segment length field, note an error.

                error(errSyntaxError, curStr->getPos(), "Previous segment handler read too many bytes");
                return false;
            }
        }
    }

    return true;
}

// compute symbol code length, per 6.5.8.2.3
//  symCodeLen = ceil( log2( numInputSyms + numNewSyms ) )
static unsigned int calculateSymCodeLen(const unsigned int numInputSyms, const unsigned int numNewSyms, const unsigned int huff)
{
    unsigned int i = numInputSyms + numNewSyms;
    if (i <= 1) {
        return huff ? 1 : 0;
    }

    --i;
    unsigned int symCodeLen = 0;
    // i = floor((numSyms-1) / 2^symCodeLen)
    while (i > 0) {
        ++symCodeLen;
        i >>= 1;
    }
    return symCodeLen;
}