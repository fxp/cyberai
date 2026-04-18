// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 16/31

s <= 256 && !readUByte(&comp)) || comp >= img.nComps || !readUByte(&img.tiles[tileIdx].tileComps[comp].quantStyle)) {
                error(errSyntaxError, getPos(), "Error in JPX QCC marker segment");
                return false;
            }
            if ((img.tiles[tileIdx].tileComps[comp].quantStyle & 0x1f) == 0x00) {
                if (segLen <= (img.nComps > 256 ? 5U : 4U)) {
                    error(errSyntaxError, getPos(), "Error in JPX QCC marker segment");
                    return false;
                }
                img.tiles[tileIdx].tileComps[comp].nQuantSteps = segLen - (img.nComps > 256 ? 5 : 4);
                img.tiles[tileIdx].tileComps[comp].quantSteps = (unsigned int *)greallocn(img.tiles[tileIdx].tileComps[comp].quantSteps, img.tiles[tileIdx].tileComps[comp].nQuantSteps, sizeof(unsigned int));
                for (i = 0; i < img.tiles[tileIdx].tileComps[comp].nQuantSteps; ++i) {
                    if (!readUByte(&img.tiles[tileIdx].tileComps[comp].quantSteps[i])) {
                        error(errSyntaxError, getPos(), "Error in JPX QCC marker segment");
                        return false;
                    }
                }
            } else if ((img.tiles[tileIdx].tileComps[comp].quantStyle & 0x1f) == 0x01) {
                img.tiles[tileIdx].tileComps[comp].nQuantSteps = 1;
                img.tiles[tileIdx].tileComps[comp].quantSteps = (unsigned int *)greallocn(img.tiles[tileIdx].tileComps[comp].quantSteps, img.tiles[tileIdx].tileComps[comp].nQuantSteps, sizeof(unsigned int));
                if (!readUWord(&img.tiles[tileIdx].tileComps[comp].quantSteps[0])) {
                    error(errSyntaxError, getPos(), "Error in JPX QCC marker segment");
                    return false;
                }
            } else if ((img.tiles[tileIdx].tileComps[comp].quantStyle & 0x1f) == 0x02) {
                if (segLen < (img.nComps > 256 ? 5U : 4U) + 2) {
                    error(errSyntaxError, getPos(), "Error in JPX QCC marker segment");
                    return false;
                }
                img.tiles[tileIdx].tileComps[comp].nQuantSteps = (segLen - (img.nComps > 256 ? 5 : 4)) / 2;
                img.tiles[tileIdx].tileComps[comp].quantSteps = (unsigned int *)greallocn(img.tiles[tileIdx].tileComps[comp].quantSteps, img.tiles[tileIdx].tileComps[comp].nQuantSteps, sizeof(unsigned int));
                for (i = 0; i < img.tiles[tileIdx].tileComps[comp].nQuantSteps; ++i) {
                    if (!readUWord(&img.tiles[tileIdx].tileComps[comp].quantSteps[i])) {
                        error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                        return false;
                    }
                }
            } else {
                error(errSyntaxError, getPos(), "Error in JPX QCC marker segment");
                return false;
            }
            break;
        case 0x5e: // RGN - region of interest
            cover(38);
#if 1 //~ ROI is unimplemented
            error(errUnimplemented, -1, "Got a JPX RGN segment");
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Error in JPX RGN marker segment");
                    return false;
                }
            }
#else
            if ((img.nComps > 256 && !readUWord(&comp)) || (img.nComps <= 256 && !readUByte(&comp)) || comp >= img.nComps || !readUByte(&compInfo[comp].roi.style) || !readUByte(&compInfo[comp].roi.shift)) {
                error(errSyntaxError, getPos(), "Error in JPX RGN marker segment");
                return false;
            }
#endif
            break;
        case 0x5f: // POC - progression order change
            cover(39);
#if 1 //~ progression order changes are unimplemented
            error(errUnimplemented, -1, "Got a JPX POC segment");
            for (i = 0; i < segLen - 2; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Error in JPX POC marker segment");
                    return false;
                }
            }
#else
            nTileProgs = (segLen - 2) / (img.nComps > 256 ? 9 : 7);
            tileProgs = (JPXProgOrder *)gmallocn(nTileProgs, sizeof(JPXProgOrder));
            for (i = 0; i < nTileProgs; ++i) {
                if (!readUByte(&tileProgs[i].startRes) || !(img.nComps > 256 && readUWord(&tileProgs[i].startComp)) || !(img.nComps <= 256 && readUByte(&tileProgs[i].startComp)) || !readUWord(&tileProgs[i].endLayer)
                    || !readUByte(&tileProgs[i].endRes) || !(img.nComps > 256 && readUWord(&tileProgs[i].endComp)) || !(img.nComps <= 256 && readUByte(&tileProgs[i].endComp)) || !readUByte(&tileProgs[i].progOrder)) {
                    error(errSyntaxError, getPos(), "Error in JPX POC marker segment");
                    return false;
                }
            }
#endif
            break;
        case 0x61: // PPT - packed packet headers, til