// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 3/31



JPXCover::~JPXCover()
{
    int i;

    printf("JPX coverage:\n");
    for (i = 0; i <= used; ++i) {
        printf("  %4d: %8d\n", i, data[i]);
    }
    gfree(data);
}

void JPXCover::incr(int idx)
{
    if (idx < size) {
        ++data[idx];
        if (idx > used) {
            used = idx;
        }
    }
}

JPXCover jpxCover(150);

#    define cover(idx) jpxCover.incr(idx)

#endif //----- coverage tracking

//------------------------------------------------------------------------

JPXStream::JPXStream(std::unique_ptr<Stream> strA) : FilterStream(strA.get())
{
    bufStr = std::make_unique<BufStream>(std::move(strA), 2);

    nComps = 0;
    bpc = nullptr;
    width = height = 0;
    haveCS = false;
    havePalette = false;
    haveCompMap = false;
    haveChannelDefn = false;

    img.tiles = nullptr;
    bitBuf = 0;
    bitBufLen = 0;
    bitBufSkip = false;
    byteCount = 0;

    curX = curY = 0;
    curComp = 0;
    readBufLen = 0;
}

JPXStream::~JPXStream()
{
    close();
}

bool JPXStream::rewind()
{
    bool ret = bufStr->rewind();
    if (readBoxes()) {
        curY = img.yOffset;
    } else {
        // readBoxes reported an error, so we go immediately to EOF
        curY = img.ySize;
        ret = false;
    }
    curX = img.xOffset;
    curComp = 0;
    readBufLen = 0;

    return ret;
}

void JPXStream::close()
{
    JPXTile *tile;
    JPXTileComp *tileComp;
    JPXResLevel *resLevel;
    JPXPrecinct *precinct;
    JPXSubband *subband;
    JPXCodeBlock *cb;
    unsigned int comp, i, k, r, pre, sb;

    gfree(bpc);
    bpc = nullptr;
    if (havePalette) {
        gfree(palette.bpc);
        gfree(palette.c);
        havePalette = false;
    }
    if (haveCompMap) {
        gfree(compMap.comp);
        gfree(compMap.type);
        gfree(compMap.pComp);
        haveCompMap = false;
    }
    if (haveChannelDefn) {
        gfree(channelDefn.idx);
        gfree(channelDefn.type);
        gfree(channelDefn.assoc);
        haveChannelDefn = false;
    }

    if (img.tiles) {
        for (i = 0; i < img.nXTiles * img.nYTiles; ++i) {
            tile = &img.tiles[i];
            if (tile->tileComps) {
                for (comp = 0; comp < img.nComps; ++comp) {
                    tileComp = &tile->tileComps[comp];
                    gfree(tileComp->quantSteps);
                    gfree(tileComp->data);
                    gfree(tileComp->buf);
                    if (tileComp->resLevels) {
                        for (r = 0; r <= tileComp->nDecompLevels; ++r) {
                            resLevel = &tileComp->resLevels[r];
                            if (resLevel->precincts) {
                                for (pre = 0; pre < 1; ++pre) {
                                    precinct = &resLevel->precincts[pre];
                                    if (precinct->subbands) {
                                        for (sb = 0; sb < (unsigned int)(r == 0 ? 1 : 3); ++sb) {
                                            subband = &precinct->subbands[sb];
                                            gfree(subband->inclusion);
                                            gfree(subband->zeroBitPlane);
                                            if (subband->cbs) {
                                                for (k = 0; k < subband->nXCBs * subband->nYCBs; ++k) {
                                                    cb = &subband->cbs[k];
                                                    gfree(cb->dataLen);
                                                    gfree(cb->touched);
                                                    if (cb->arithDecoder) {
                                                        delete cb->arithDecoder;
                                                    }
                                                    if (cb->stats) {
                                                        delete cb->stats;
                                                    }
                                                }
                                                gfree(subband->cbs);
                                            }
                                        }
                                        gfree(precinct->subbands);
                                    }
                                }
                                gfree(img.tiles[i].tileComps[comp].resLevels[r].precincts);
                            }
                        }
                        gfree(img.tiles[i].tileComps[comp].resLevels);
                    }
                }
                gfree(img.tiles[i].tileComps);
            }
        }
        gfree(img.tiles);
        img.tiles = nullptr;
    }
    bufStr->close();
}

int JPXStream::getChar()
{
    int c;