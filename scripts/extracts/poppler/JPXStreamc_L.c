// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 12/31

on of interest
            cover(25);
#if 1 //~ ROI is unimplemented
            error(errUnimplemented, -1, "got a JPX RGN segment");
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Error in JPX RGN marker segment");
                    return false;
                }
            }
#else
            if ((img.nComps > 256 && !readUWord(&comp)) || (img.nComps <= 256 && !readUByte(&comp)) || comp >= img.nComps || !readUByte(&compInfo[comp].defROI.style) || !readUByte(&compInfo[comp].defROI.shift)) {
                error(errSyntaxError, getPos(), "Error in JPX RGN marker segment");
                return false;
            }
#endif
            break;
        case 0x5f: // POC - progression order change
            cover(26);
#if 1 //~ progression order changes are unimplemented
            error(errUnimplemented, -1, "got a JPX POC segment");
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Error in JPX POC marker segment");
                    return false;
                }
            }
#else
            nProgs = (segLen - 2) / (img.nComps > 256 ? 9 : 7);
            progs = (JPXProgOrder *)gmallocn(nProgs, sizeof(JPXProgOrder));
            for (i = 0; i < nProgs; ++i) {
                if (!readUByte(&progs[i].startRes) || !(img.nComps > 256 && readUWord(&progs[i].startComp)) || !(img.nComps <= 256 && readUByte(&progs[i].startComp)) || !readUWord(&progs[i].endLayer) || !readUByte(&progs[i].endRes)
                    || !(img.nComps > 256 && readUWord(&progs[i].endComp)) || !(img.nComps <= 256 && readUByte(&progs[i].endComp)) || !readUByte(&progs[i].progOrder)) {
                    error(errSyntaxError, getPos(), "Error in JPX POC marker segment");
                    return false;
                }
            }
#endif
            break;
        case 0x60: // PPM - packed packet headers, main header
            cover(27);
#if 1 //~ packed packet headers are unimplemented
            error(errUnimplemented, -1, "Got a JPX PPM segment");
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Error in JPX PPM marker segment");
                    return false;
                }
            }
#endif
            break;
        case 0x55: // TLM - tile-part lengths
            // skipped
            cover(28);
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Error in JPX TLM marker segment");
                    return false;
                }
            }
            break;
        case 0x57: // PLM - packet length, main header
            // skipped
            cover(29);
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Error in JPX PLM marker segment");
                    return false;
                }
            }
            break;
        case 0x63: // CRG - component registration
            // skipped
            cover(30);
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Error in JPX CRG marker segment");
                    return false;
                }
            }
            break;
        case 0x64: // COM - comment
            // skipped
            cover(31);
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Error in JPX COM marker segment");
                    return false;
                }
            }
            break;
        case 0x90: // SOT - start of tile
            cover(32);
            haveSOT = true;
            break;
        default:
            cover(33);
            error(errSyntaxError, getPos(), "Unknown marker segment {0:02x} in JPX stream", segType);
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    break;
                }
            }
            break;
        }
    } while (!haveSOT);

    if (!haveSIZ) {
        error(errSyntaxError, getPos(), "Missing SIZ marker segment in JPX stream");
        return false;
    }
    if (!haveCOD) {
        error(errSyntaxError, getPos(), "Missing COD marker segment in JPX stream");
        return false;
    }
    if (!haveQCD) {
        error(errSyntaxError, getPos(), "Missing QCD marker segment in JPX stream");
        return false;
    }