// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 18/31



    //----- initialize the tile, precincts, and code-blocks
    if (tilePartIdx == 0) {
        tile = &img.tiles[tileIdx];
        tile->init = true;
        i = tileIdx / img.nXTiles;
        j = tileIdx % img.nXTiles;
        if ((tile->x0 = img.xTileOffset + j * img.xTileSize) < img.xOffset) {
            tile->x0 = img.xOffset;
        }
        if ((tile->y0 = img.yTileOffset + i * img.yTileSize) < img.yOffset) {
            tile->y0 = img.yOffset;
        }
        if ((tile->x1 = img.xTileOffset + (j + 1) * img.xTileSize) > img.xSize) {
            tile->x1 = img.xSize;
        }
        if ((tile->y1 = img.yTileOffset + (i + 1) * img.yTileSize) > img.ySize) {
            tile->y1 = img.ySize;
        }
        tile->comp = 0;
        tile->res = 0;
        tile->precinct = 0;
        tile->layer = 0;
        tile->maxNDecompLevels = 0;
        for (comp = 0; comp < img.nComps; ++comp) {
            tileComp = &tile->tileComps[comp];
            if (tileComp->nDecompLevels > tile->maxNDecompLevels) {
                tile->maxNDecompLevels = tileComp->nDecompLevels;
            }
            tileComp->x0 = jpxCeilDiv(tile->x0, tileComp->hSep);
            tileComp->y0 = jpxCeilDiv(tile->y0, tileComp->vSep);
            tileComp->x1 = jpxCeilDiv(tile->x1, tileComp->hSep);
            tileComp->y1 = jpxCeilDiv(tile->y1, tileComp->vSep);
            tileComp->w = tileComp->x1 - tileComp->x0;
            tileComp->cbW = 1 << tileComp->codeBlockW;
            tileComp->cbH = 1 << tileComp->codeBlockH;
            tileComp->data = (int *)gmallocn((tileComp->x1 - tileComp->x0) * (tileComp->y1 - tileComp->y0), sizeof(int));
            if (tileComp->x1 - tileComp->x0 > tileComp->y1 - tileComp->y0) {
                n = tileComp->x1 - tileComp->x0;
            } else {
                n = tileComp->y1 - tileComp->y0;
            }
            tileComp->buf = (int *)gmallocn(n + 8, sizeof(int));
            for (r = 0; r <= tileComp->nDecompLevels; ++r) {
                resLevel = &tileComp->resLevels[r];
                k = r == 0 ? tileComp->nDecompLevels : tileComp->nDecompLevels - r + 1;
                resLevel->x0 = jpxCeilDivPow2(tileComp->x0, k);
                resLevel->y0 = jpxCeilDivPow2(tileComp->y0, k);
                resLevel->x1 = jpxCeilDivPow2(tileComp->x1, k);
                resLevel->y1 = jpxCeilDivPow2(tileComp->y1, k);
                if (r == 0) {
                    resLevel->bx0[0] = resLevel->x0;
                    resLevel->by0[0] = resLevel->y0;
                    resLevel->bx1[0] = resLevel->x1;
                    resLevel->by1[0] = resLevel->y1;
                } else {
                    resLevel->bx0[0] = jpxCeilDivPow2(tileComp->x0 - (1 << (k - 1)), k);
                    resLevel->by0[0] = resLevel->y0;
                    resLevel->bx1[0] = jpxCeilDivPow2(tileComp->x1 - (1 << (k - 1)), k);
                    resLevel->by1[0] = resLevel->y1;
                    resLevel->bx0[1] = resLevel->x0;
                    resLevel->by0[1] = jpxCeilDivPow2(tileComp->y0 - (1 << (k - 1)), k);
                    resLevel->bx1[1] = resLevel->x1;
                    resLevel->by1[1] = jpxCeilDivPow2(tileComp->y1 - (1 << (k - 1)), k);
                    resLevel->bx0[2] = jpxCeilDivPow2(tileComp->x0 - (1 << (k - 1)), k);
                    resLevel->by0[2] = jpxCeilDivPow2(tileComp->y0 - (1 << (k - 1)), k);
                    resLevel->bx1[2] = jpxCeilDivPow2(tileComp->x1 - (1 << (k - 1)), k);
                    resLevel->by1[2] = jpxCeilDivPow2(tileComp->y1 - (1 << (k - 1)), k);
                }
                resLevel->precincts = (JPXPrecinct *)gmallocn(1, sizeof(JPXPrecinct));
                for (pre = 0; pre < 1; ++pre) {
                    precinct = &resLevel->precincts[pre];
                    precinct->x0 = resLevel->x0;
                    precinct->y0 = resLevel->y0;
                    precinct->x1 = resLevel->x1;
                    precinct->y1 = resLevel->y1;
                    nSBs = r == 0 ? 1 : 3;
                    precinct->subbands = (JPXSubband *)gmallocn(nSBs, sizeof(JPXSubband));
                    for (sb = 0; sb < nSBs; ++sb) {
                        subband = &precinct->subbands[sb];
                        subband->x0 = resLevel->bx0[sb];
                        subband->y0 = resLevel->by0[sb];
                        subband->x1 = resLevel->bx1[sb];
                        subband->y1 = resLevel->by1[sb];
                        subband->nXCBs = jpxCeilDivPow2(subband->x1, tileComp->codeBlockW) - jpxFloorDivPow2(subband->x0, tileComp->codeBlockW);
                        subband->nYCBs = jpxCeilDivPow2(subband->y1, tileComp->codeBlockH) - jpxFloorDivPow2(subband->y0, tileComp->codeBlockH);
                        n = subband->nXCBs > subband->nYCBs ? subband->nXCBs : subband->nYCBs;
                        for (subband->maxTTLevel = 0, --n; n; ++subband->maxTTLevel, n >>= 1)
                            ;
                        n = 0;
    