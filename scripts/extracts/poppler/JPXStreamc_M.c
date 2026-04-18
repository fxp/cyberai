// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 13/31



    //----- read the tile-parts
    while (1) {
        if (!readTilePart()) {
            return false;
        }
        if (!readMarkerHdr(&segType, &segLen)) {
            error(errSyntaxError, getPos(), "Error in JPX codestream");
            return false;
        }
        if (segType != 0x90) { // SOT - start of tile
            break;
        }
    }

    if (segType != 0xd9) { // EOC - end of codestream
        error(errSyntaxError, getPos(), "Missing EOC marker in JPX codestream");
        return false;
    }

    //----- finish decoding the image
    for (i = 0; i < img.nXTiles * img.nYTiles; ++i) {
        tile = &img.tiles[i];
        if (!tile->init) {
            error(errSyntaxError, getPos(), "Uninitialized tile in JPX codestream");
            return false;
        }
        for (comp = 0; comp < img.nComps; ++comp) {
            tileComp = &tile->tileComps[comp];
            inverseTransform(tileComp);
        }
        if (!inverseMultiCompAndDC(tile)) {
            return false;
        }
    }

    //~ can free memory below tileComps here, and also tileComp.buf

    return true;
}

bool JPXStream::readTilePart()
{
    JPXTile *tile;
    JPXTileComp *tileComp;
    JPXResLevel *resLevel;
    JPXPrecinct *precinct;
    JPXSubband *subband;
    JPXCodeBlock *cb;
    int *sbCoeffs;
    bool haveSOD;
    unsigned int tileIdx, tilePartLen, tilePartIdx, nTileParts;
    bool tilePartToEOC;
    unsigned int precinctSize, style, nDecompLevels;
    unsigned int n, nSBs, nx, ny, sbx0, sby0, comp, segLen;
    unsigned int i, j, k, cbX, cbY, r, pre, sb, cbi, cbj;
    int segType, level;

    // process the SOT marker segment
    if (!readUWord(&tileIdx) || !readULong(&tilePartLen) || !readUByte(&tilePartIdx) || !readUByte(&nTileParts)) {
        error(errSyntaxError, getPos(), "Error in JPX SOT marker segment");
        return false;
    }

    if (tileIdx >= img.nXTiles * img.nYTiles || (tilePartIdx > 0 && !img.tiles[tileIdx].init)) {
        error(errSyntaxError, getPos(), "Weird tile index in JPX stream");
        return false;
    }

    tilePartToEOC = tilePartLen == 0;
    tilePartLen -= 12; // subtract size of SOT segment