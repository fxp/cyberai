// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 9/31

[0].tileComps[0].codeBlockStyle) || !readUByte(&img.tiles[0].tileComps[0].transform)) {
                error(errSyntaxError, getPos(), "Error in JPX COD marker segment");
                return false;
            }
            if (nDecompLevels > 32 || img.tiles[0].tileComps[0].codeBlockW > 8 || img.tiles[0].tileComps[0].codeBlockH > 8) {
                error(errSyntaxError, getPos(), "Error in JPX COD marker segment");
                return false;
            }
            img.tiles[0].tileComps[0].nDecompLevels = nDecompLevels;
            img.tiles[0].tileComps[0].codeBlockW += 2;
            img.tiles[0].tileComps[0].codeBlockH += 2;
            for (i = 0; i < img.nXTiles * img.nYTiles; ++i) {
                if (i != 0) {
                    img.tiles[i].progOrder = img.tiles[0].progOrder;
                    img.tiles[i].nLayers = img.tiles[0].nLayers;
                    img.tiles[i].multiComp = img.tiles[0].multiComp;
                }
                for (comp = 0; comp < img.nComps; ++comp) {
                    if (!(i == 0 && comp == 0)) {
                        img.tiles[i].tileComps[comp].style = img.tiles[0].tileComps[0].style;
                        img.tiles[i].tileComps[comp].nDecompLevels = img.tiles[0].tileComps[0].nDecompLevels;
                        img.tiles[i].tileComps[comp].codeBlockW = img.tiles[0].tileComps[0].codeBlockW;
                        img.tiles[i].tileComps[comp].codeBlockH = img.tiles[0].tileComps[0].codeBlockH;
                        img.tiles[i].tileComps[comp].codeBlockStyle = img.tiles[0].tileComps[0].codeBlockStyle;
                        img.tiles[i].tileComps[comp].transform = img.tiles[0].tileComps[0].transform;
                    }
                    img.tiles[i].tileComps[comp].resLevels = (JPXResLevel *)gmallocn_checkoverflow((img.tiles[i].tileComps[comp].nDecompLevels + 1), sizeof(JPXResLevel));
                    if (img.tiles[i].tileComps[comp].resLevels == nullptr) {
                        error(errSyntaxError, getPos(), "Error in JPX COD marker segment");
                        return false;
                    }
                    for (r = 0; r <= img.tiles[i].tileComps[comp].nDecompLevels; ++r) {
                        img.tiles[i].tileComps[comp].resLevels[r].precincts = nullptr;
                    }
                }
            }
            for (r = 0; r <= img.tiles[0].tileComps[0].nDecompLevels; ++r) {
                if (img.tiles[0].tileComps[0].style & 0x01) {
                    cover(91);
                    if (!readUByte(&precinctSize)) {
                        error(errSyntaxError, getPos(), "Error in JPX COD marker segment");
                        return false;
                    }
                    img.tiles[0].tileComps[0].resLevels[r].precinctWidth = precinctSize & 0x0f;
                    img.tiles[0].tileComps[0].resLevels[r].precinctHeight = (precinctSize >> 4) & 0x0f;
                } else {
                    img.tiles[0].tileComps[0].resLevels[r].precinctWidth = 15;
                    img.tiles[0].tileComps[0].resLevels[r].precinctHeight = 15;
                }
            }
            for (i = 0; i < img.nXTiles * img.nYTiles; ++i) {
                for (comp = 0; comp < img.nComps; ++comp) {
                    if (!(i == 0 && comp == 0)) {
                        for (r = 0; r <= img.tiles[i].tileComps[comp].nDecompLevels; ++r) {
                            img.tiles[i].tileComps[comp].resLevels[r].precinctWidth = img.tiles[0].tileComps[0].resLevels[r].precinctWidth;
                            img.tiles[i].tileComps[comp].resLevels[r].precinctHeight = img.tiles[0].tileComps[0].resLevels[r].precinctHeight;
                        }
                    }
                }
            }
            haveCOD = true;
            break;
        case 0x53: // COC - coding style component
            cover(22);
            if (!haveCOD) {
                error(errSyntaxError, getPos(), "JPX COC marker segment before COD segment");
                return false;
            }
            if ((img.nComps > 256 && !readUWord(&comp)) || (img.nComps <= 256 && !readUByte(&comp)) || comp >= img.nComps || !readUByte(&style) || !readUByte(&nDecompLevels) || !readUByte(&img.tiles[0].tileComps[comp].codeBlockW)
                || !readUByte(&img.tiles[0].tileComps[comp].codeBlockH) || !readUByte(&img.tiles[0].tileComps[comp].codeBlockStyle) || !readUByte(&img.tiles[0].tileComps[comp].transform)) {
                error(errSyntaxError, getPos(), "Error in JPX COC marker segment");
                return false;
            }
            if (nDecompLevels > 32 || img.tiles[0].tileComps[comp].codeBlockW > 8 || img.tiles[0].tileComps[comp].codeBlockH > 8) {
                error(errSyntaxError, getPos(), "Error in JPX COC marker segment");
                return false;
            }
            img.tiles[0].tileComps[comp].nDecompLevels = nDecompLevels;
            img.tiles[0].tileComps[comp].style = (img.tiles[0].tile