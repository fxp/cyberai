// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 15/31

].style & ~1) | (style & 1);
            img.tiles[tileIdx].tileComps[comp].codeBlockW += 2;
            img.tiles[tileIdx].tileComps[comp].codeBlockH += 2;
            img.tiles[tileIdx].tileComps[comp].resLevels = (JPXResLevel *)greallocn(img.tiles[tileIdx].tileComps[comp].resLevels, (img.tiles[tileIdx].tileComps[comp].nDecompLevels + 1), sizeof(JPXResLevel));
            for (r = 0; r <= img.tiles[tileIdx].tileComps[comp].nDecompLevels; ++r) {
                img.tiles[tileIdx].tileComps[comp].resLevels[r].precincts = nullptr;
            }
            for (r = 0; r <= img.tiles[tileIdx].tileComps[comp].nDecompLevels; ++r) {
                if (img.tiles[tileIdx].tileComps[comp].style & 0x01) {
                    if (!readUByte(&precinctSize)) {
                        error(errSyntaxError, getPos(), "Error in JPX COD marker segment");
                        return false;
                    }
                    img.tiles[tileIdx].tileComps[comp].resLevels[r].precinctWidth = precinctSize & 0x0f;
                    img.tiles[tileIdx].tileComps[comp].resLevels[r].precinctHeight = (precinctSize >> 4) & 0x0f;
                } else {
                    img.tiles[tileIdx].tileComps[comp].resLevels[r].precinctWidth = 15;
                    img.tiles[tileIdx].tileComps[comp].resLevels[r].precinctHeight = 15;
                }
            }
            break;
        case 0x5c: // QCD - quantization default
            cover(36);
            if (!readUByte(&img.tiles[tileIdx].tileComps[0].quantStyle)) {
                error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                return false;
            }
            if ((img.tiles[tileIdx].tileComps[0].quantStyle & 0x1f) == 0x00) {
                if (segLen <= 3) {
                    error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                    return false;
                }
                img.tiles[tileIdx].tileComps[0].nQuantSteps = segLen - 3;
                img.tiles[tileIdx].tileComps[0].quantSteps = (unsigned int *)greallocn(img.tiles[tileIdx].tileComps[0].quantSteps, img.tiles[tileIdx].tileComps[0].nQuantSteps, sizeof(unsigned int));
                for (i = 0; i < img.tiles[tileIdx].tileComps[0].nQuantSteps; ++i) {
                    if (!readUByte(&img.tiles[tileIdx].tileComps[0].quantSteps[i])) {
                        error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                        return false;
                    }
                }
            } else if ((img.tiles[tileIdx].tileComps[0].quantStyle & 0x1f) == 0x01) {
                img.tiles[tileIdx].tileComps[0].nQuantSteps = 1;
                img.tiles[tileIdx].tileComps[0].quantSteps = (unsigned int *)greallocn(img.tiles[tileIdx].tileComps[0].quantSteps, img.tiles[tileIdx].tileComps[0].nQuantSteps, sizeof(unsigned int));
                if (!readUWord(&img.tiles[tileIdx].tileComps[0].quantSteps[0])) {
                    error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                    return false;
                }
            } else if ((img.tiles[tileIdx].tileComps[0].quantStyle & 0x1f) == 0x02) {
                if (segLen < 5) {
                    error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                    return false;
                }
                img.tiles[tileIdx].tileComps[0].nQuantSteps = (segLen - 3) / 2;
                img.tiles[tileIdx].tileComps[0].quantSteps = (unsigned int *)greallocn(img.tiles[tileIdx].tileComps[0].quantSteps, img.tiles[tileIdx].tileComps[0].nQuantSteps, sizeof(unsigned int));
                for (i = 0; i < img.tiles[tileIdx].tileComps[0].nQuantSteps; ++i) {
                    if (!readUWord(&img.tiles[tileIdx].tileComps[0].quantSteps[i])) {
                        error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                        return false;
                    }
                }
            } else {
                error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                return false;
            }
            for (comp = 1; comp < img.nComps; ++comp) {
                img.tiles[tileIdx].tileComps[comp].quantStyle = img.tiles[tileIdx].tileComps[0].quantStyle;
                img.tiles[tileIdx].tileComps[comp].nQuantSteps = img.tiles[tileIdx].tileComps[0].nQuantSteps;
                img.tiles[tileIdx].tileComps[comp].quantSteps = (unsigned int *)greallocn(img.tiles[tileIdx].tileComps[comp].quantSteps, img.tiles[tileIdx].tileComps[0].nQuantSteps, sizeof(unsigned int));
                for (j = 0; j < img.tiles[tileIdx].tileComps[0].nQuantSteps; ++j) {
                    img.tiles[tileIdx].tileComps[comp].quantSteps[j] = img.tiles[tileIdx].tileComps[0].quantSteps[j];
                }
            }
            break;
        case 0x5d: // QCC - quantization component
            cover(37);
            if ((img.nComps > 256 && !readUWord(&comp)) || (img.nComp