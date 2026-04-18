// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 8/31



bool JPXStream::readCodestream()
{
    JPXTile *tile;
    JPXTileComp *tileComp;
    int segType;
    bool haveSIZ, haveCOD, haveQCD, haveSOT;
    unsigned int precinctSize, style, nDecompLevels;
    unsigned int segLen, capabilities, comp, i, j, r;

    //----- main header
    haveSIZ = haveCOD = haveQCD = haveSOT = false;
    do {
        if (!readMarkerHdr(&segType, &segLen)) {
            error(errSyntaxError, getPos(), "Error in JPX codestream");
            return false;
        }
        switch (segType) {
        case 0x4f: // SOC - start of codestream
            // marker only
            cover(19);
            break;
        case 0x51: // SIZ - image and tile size
            cover(20);
            if (haveSIZ) {
                error(errSyntaxError, getPos(), "Duplicate SIZ marker segment in JPX stream");
                return false;
            }
            if (!readUWord(&capabilities) || !readULong(&img.xSize) || !readULong(&img.ySize) || !readULong(&img.xOffset) || !readULong(&img.yOffset) || !readULong(&img.xTileSize) || !readULong(&img.yTileSize)
                || !readULong(&img.xTileOffset) || !readULong(&img.yTileOffset) || !readUWord(&img.nComps)) {
                error(errSyntaxError, getPos(), "Error in JPX SIZ marker segment");
                return false;
            }
            if (haveImgHdr && img.nComps != nComps) {
                error(errSyntaxError, getPos(), "Different number of components in JPX SIZ marker segment");
                return false;
            }
            if (img.xSize == 0 || img.ySize == 0 || img.xOffset >= img.xSize || img.yOffset >= img.ySize || img.xTileSize == 0 || img.yTileSize == 0 || img.xTileOffset > img.xOffset || img.yTileOffset > img.yOffset
                || img.xTileSize + img.xTileOffset <= img.xOffset || img.yTileSize + img.yTileOffset <= img.yOffset) {
                error(errSyntaxError, getPos(), "Error in JPX SIZ marker segment");
                return false;
            }
            img.nXTiles = (img.xSize - img.xTileOffset + img.xTileSize - 1) / img.xTileSize;
            img.nYTiles = (img.ySize - img.yTileOffset + img.yTileSize - 1) / img.yTileSize;
            // check for overflow before allocating memory
            if (img.nXTiles <= 0 || img.nYTiles <= 0 || img.nXTiles >= 65535 / img.nYTiles) {
                error(errSyntaxError, getPos(), "Bad tile count in JPX SIZ marker segment");
                return false;
            }
            img.tiles = (JPXTile *)gmallocn(img.nXTiles * img.nYTiles, sizeof(JPXTile));
            for (i = 0; i < img.nXTiles * img.nYTiles; ++i) {
                img.tiles[i].init = false;
                img.tiles[i].tileComps = (JPXTileComp *)gmallocn(img.nComps, sizeof(JPXTileComp));
                for (comp = 0; comp < img.nComps; ++comp) {
                    img.tiles[i].tileComps[comp].quantSteps = nullptr;
                    img.tiles[i].tileComps[comp].data = nullptr;
                    img.tiles[i].tileComps[comp].buf = nullptr;
                    img.tiles[i].tileComps[comp].resLevels = nullptr;
                }
            }
            for (comp = 0; comp < img.nComps; ++comp) {
                if (!readUByte(&img.tiles[0].tileComps[comp].prec) || !readUByte(&img.tiles[0].tileComps[comp].hSep) || !readUByte(&img.tiles[0].tileComps[comp].vSep)) {
                    error(errSyntaxError, getPos(), "Error in JPX SIZ marker segment");
                    return false;
                }
                if (img.tiles[0].tileComps[comp].hSep == 0 || img.tiles[0].tileComps[comp].vSep == 0) {
                    error(errSyntaxError, getPos(), "Error in JPX SIZ marker segment");
                    return false;
                }
                img.tiles[0].tileComps[comp].sgned = (img.tiles[0].tileComps[comp].prec & 0x80) ? true : false;
                img.tiles[0].tileComps[comp].prec = (img.tiles[0].tileComps[comp].prec & 0x7f) + 1;
                for (i = 1; i < img.nXTiles * img.nYTiles; ++i) {
                    img.tiles[i].tileComps[comp] = img.tiles[0].tileComps[comp];
                }
            }
            haveSIZ = true;
            break;
        case 0x52: // COD - coding style default
            cover(21);
            if (!haveSIZ) {
                error(errSyntaxError, getPos(), "JPX COD marker segment before SIZ segment");
                return false;
            }
            if (img.tiles == nullptr || img.nXTiles * img.nYTiles == 0 || img.tiles[0].tileComps == nullptr) {
                error(errSyntaxError, getPos(), "Error in JPX COD marker segment");
                return false;
            }
            if (!readUByte(&img.tiles[0].tileComps[0].style) || !readUByte(&img.tiles[0].progOrder) || !readUWord(&img.tiles[0].nLayers) || !readUByte(&img.tiles[0].multiComp) || !readUByte(&nDecompLevels)
                || !readUByte(&img.tiles[0].tileComps[0].codeBlockW) || !readUByte(&img.tiles[0].tileComps[0].codeBlockH) || !readUByte(&img.tiles