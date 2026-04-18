// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 17/31

e-part hdr
            cover(40);
#if 1 //~ packed packet headers are unimplemented
            error(errUnimplemented, -1, "Got a JPX PPT segment");
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Error in JPX PPT marker segment");
                    return false;
                }
            }
#endif
        case 0x58: // PLT - packet length, tile-part header
            // skipped
            cover(41);
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Error in JPX PLT marker segment");
                    return false;
                }
            }
            break;
        case 0x64: // COM - comment
            // skipped
            cover(42);
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Error in JPX COM marker segment");
                    return false;
                }
            }
            break;
        case 0x93: // SOD - start of data
            cover(43);
            haveSOD = true;
            break;
        default:
            cover(44);
            error(errSyntaxError, getPos(), "Unknown marker segment {0:02x} in JPX tile-part stream", segType);
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    break;
                }
            }
            break;
        }
    } while (!haveSOD);