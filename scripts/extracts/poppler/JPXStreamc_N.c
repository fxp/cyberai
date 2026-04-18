// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 14/31



    haveSOD = false;
    do {
        if (!readMarkerHdr(&segType, &segLen)) {
            error(errSyntaxError, getPos(), "Error in JPX tile-part codestream");
            return false;
        }
        tilePartLen -= 2 + segLen;
        switch (segType) {
        case 0x52: // COD - coding style default
            cover(34);
            if (!readUByte(&img.tiles[tileIdx].tileComps[0].style) || !readUByte(&img.tiles[tileIdx].progOrder) || !readUWord(&img.tiles[tileIdx].nLayers) || !readUByte(&img.tiles[tileIdx].multiComp) || !readUByte(&nDecompLevels)
                || !readUByte(&img.tiles[tileIdx].tileComps[0].codeBlockW) || !readUByte(&img.tiles[tileIdx].tileComps[0].codeBlockH) || !readUByte(&img.tiles[tileIdx].tileComps[0].codeBlockStyle)
                || !readUByte(&img.tiles[tileIdx].tileComps[0].transform)) {
                error(errSyntaxError, getPos(), "Error in JPX COD marker segment");
                return false;
            }
            if (nDecompLevels > 32 || img.tiles[tileIdx].tileComps[0].codeBlockW > 8 || img.tiles[tileIdx].tileComps[0].codeBlockH > 8) {
                error(errSyntaxError, getPos(), "Error in JPX COD marker segment");
                return false;
            }
            img.tiles[tileIdx].tileComps[0].nDecompLevels = nDecompLevels;
            img.tiles[tileIdx].tileComps[0].codeBlockW += 2;
            img.tiles[tileIdx].tileComps[0].codeBlockH += 2;
            for (comp = 0; comp < img.nComps; ++comp) {
                if (comp != 0) {
                    img.tiles[tileIdx].tileComps[comp].style = img.tiles[tileIdx].tileComps[0].style;
                    img.tiles[tileIdx].tileComps[comp].nDecompLevels = img.tiles[tileIdx].tileComps[0].nDecompLevels;
                    img.tiles[tileIdx].tileComps[comp].codeBlockW = img.tiles[tileIdx].tileComps[0].codeBlockW;
                    img.tiles[tileIdx].tileComps[comp].codeBlockH = img.tiles[tileIdx].tileComps[0].codeBlockH;
                    img.tiles[tileIdx].tileComps[comp].codeBlockStyle = img.tiles[tileIdx].tileComps[0].codeBlockStyle;
                    img.tiles[tileIdx].tileComps[comp].transform = img.tiles[tileIdx].tileComps[0].transform;
                }
                img.tiles[tileIdx].tileComps[comp].resLevels = (JPXResLevel *)greallocn(img.tiles[tileIdx].tileComps[comp].resLevels, (img.tiles[tileIdx].tileComps[comp].nDecompLevels + 1), sizeof(JPXResLevel));
                for (r = 0; r <= img.tiles[tileIdx].tileComps[comp].nDecompLevels; ++r) {
                    img.tiles[tileIdx].tileComps[comp].resLevels[r].precincts = nullptr;
                }
            }
            for (r = 0; r <= img.tiles[tileIdx].tileComps[0].nDecompLevels; ++r) {
                if (img.tiles[tileIdx].tileComps[0].style & 0x01) {
                    if (!readUByte(&precinctSize)) {
                        error(errSyntaxError, getPos(), "Error in JPX COD marker segment");
                        return false;
                    }
                    img.tiles[tileIdx].tileComps[0].resLevels[r].precinctWidth = precinctSize & 0x0f;
                    img.tiles[tileIdx].tileComps[0].resLevels[r].precinctHeight = (precinctSize >> 4) & 0x0f;
                } else {
                    img.tiles[tileIdx].tileComps[0].resLevels[r].precinctWidth = 15;
                    img.tiles[tileIdx].tileComps[0].resLevels[r].precinctHeight = 15;
                }
            }
            for (comp = 1; comp < img.nComps; ++comp) {
                for (r = 0; r <= img.tiles[tileIdx].tileComps[comp].nDecompLevels; ++r) {
                    img.tiles[tileIdx].tileComps[comp].resLevels[r].precinctWidth = img.tiles[tileIdx].tileComps[0].resLevels[r].precinctWidth;
                    img.tiles[tileIdx].tileComps[comp].resLevels[r].precinctHeight = img.tiles[tileIdx].tileComps[0].resLevels[r].precinctHeight;
                }
            }
            break;
        case 0x53: // COC - coding style component
            cover(35);
            if ((img.nComps > 256 && !readUWord(&comp)) || (img.nComps <= 256 && !readUByte(&comp)) || comp >= img.nComps || !readUByte(&style) || !readUByte(&nDecompLevels) || !readUByte(&img.tiles[tileIdx].tileComps[comp].codeBlockW)
                || !readUByte(&img.tiles[tileIdx].tileComps[comp].codeBlockH) || !readUByte(&img.tiles[tileIdx].tileComps[comp].codeBlockStyle) || !readUByte(&img.tiles[tileIdx].tileComps[comp].transform)) {
                error(errSyntaxError, getPos(), "Error in JPX COC marker segment");
                return false;
            }
            if (nDecompLevels > 32 || img.tiles[tileIdx].tileComps[comp].codeBlockW > 8 || img.tiles[tileIdx].tileComps[comp].codeBlockH > 8) {
                error(errSyntaxError, getPos(), "Error in JPX COD marker segment");
                return false;
            }
            img.tiles[tileIdx].tileComps[comp].nDecompLevels = nDecompLevels;
            img.tiles[tileIdx].tileComps[comp].style = (img.tiles[tileIdx].tileComps[comp