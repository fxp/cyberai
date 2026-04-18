// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 10/31

Comps[comp].style & ~1) | (style & 1);
            img.tiles[0].tileComps[comp].codeBlockW += 2;
            img.tiles[0].tileComps[comp].codeBlockH += 2;
            for (i = 0; i < img.nXTiles * img.nYTiles; ++i) {
                if (i != 0) {
                    img.tiles[i].tileComps[comp].style = img.tiles[0].tileComps[comp].style;
                    img.tiles[i].tileComps[comp].nDecompLevels = img.tiles[0].tileComps[comp].nDecompLevels;
                    img.tiles[i].tileComps[comp].codeBlockW = img.tiles[0].tileComps[comp].codeBlockW;
                    img.tiles[i].tileComps[comp].codeBlockH = img.tiles[0].tileComps[comp].codeBlockH;
                    img.tiles[i].tileComps[comp].codeBlockStyle = img.tiles[0].tileComps[comp].codeBlockStyle;
                    img.tiles[i].tileComps[comp].transform = img.tiles[0].tileComps[comp].transform;
                }
                img.tiles[i].tileComps[comp].resLevels = (JPXResLevel *)greallocn(img.tiles[i].tileComps[comp].resLevels, (img.tiles[i].tileComps[comp].nDecompLevels + 1), sizeof(JPXResLevel));
                for (r = 0; r <= img.tiles[i].tileComps[comp].nDecompLevels; ++r) {
                    img.tiles[i].tileComps[comp].resLevels[r].precincts = nullptr;
                }
            }
            for (r = 0; r <= img.tiles[0].tileComps[comp].nDecompLevels; ++r) {
                if (img.tiles[0].tileComps[comp].style & 0x01) {
                    if (!readUByte(&precinctSize)) {
                        error(errSyntaxError, getPos(), "Error in JPX COD marker segment");
                        return false;
                    }
                    img.tiles[0].tileComps[comp].resLevels[r].precinctWidth = precinctSize & 0x0f;
                    img.tiles[0].tileComps[comp].resLevels[r].precinctHeight = (precinctSize >> 4) & 0x0f;
                } else {
                    img.tiles[0].tileComps[comp].resLevels[r].precinctWidth = 15;
                    img.tiles[0].tileComps[comp].resLevels[r].precinctHeight = 15;
                }
            }
            for (i = 1; i < img.nXTiles * img.nYTiles; ++i) {
                for (r = 0; r <= img.tiles[i].tileComps[comp].nDecompLevels; ++r) {
                    img.tiles[i].tileComps[comp].resLevels[r].precinctWidth = img.tiles[0].tileComps[comp].resLevels[r].precinctWidth;
                    img.tiles[i].tileComps[comp].resLevels[r].precinctHeight = img.tiles[0].tileComps[comp].resLevels[r].precinctHeight;
                }
            }
            break;
        case 0x5c: // QCD - quantization default
            cover(23);
            if (!haveSIZ) {
                error(errSyntaxError, getPos(), "JPX QCD marker segment before SIZ segment");
                return false;
            }
            if (!readUByte(&img.tiles[0].tileComps[0].quantStyle)) {
                error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                return false;
            }
            if ((img.tiles[0].tileComps[0].quantStyle & 0x1f) == 0x00) {
                if (segLen <= 3) {
                    error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                    return false;
                }
                img.tiles[0].tileComps[0].nQuantSteps = segLen - 3;
                img.tiles[0].tileComps[0].quantSteps = (unsigned int *)greallocn(img.tiles[0].tileComps[0].quantSteps, img.tiles[0].tileComps[0].nQuantSteps, sizeof(unsigned int));
                for (i = 0; i < img.tiles[0].tileComps[0].nQuantSteps; ++i) {
                    if (!readUByte(&img.tiles[0].tileComps[0].quantSteps[i])) {
                        error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                        return false;
                    }
                }
            } else if ((img.tiles[0].tileComps[0].quantStyle & 0x1f) == 0x01) {
                img.tiles[0].tileComps[0].nQuantSteps = 1;
                img.tiles[0].tileComps[0].quantSteps = (unsigned int *)greallocn(img.tiles[0].tileComps[0].quantSteps, img.tiles[0].tileComps[0].nQuantSteps, sizeof(unsigned int));
                if (!readUWord(&img.tiles[0].tileComps[0].quantSteps[0])) {
                    error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                    return false;
                }
            } else if ((img.tiles[0].tileComps[0].quantStyle & 0x1f) == 0x02) {
                if (segLen < 5) {
                    error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                    return false;
                }
                img.tiles[0].tileComps[0].nQuantSteps = (segLen - 3) / 2;
                img.tiles[0].tileComps[0].quantSteps = (unsigned int *)greallocn(img.tiles[0].tileComps[0].quantSteps, img.tiles[0].tileComps[0].nQuantSteps, sizeof(unsigned int));
                for (i = 0; i < img.tiles[0].tileComps[0].nQuantSteps; ++i) {
                    if (!readUWord(&img.tiles[0].tileComps[0].quantSteps[i])) {
            