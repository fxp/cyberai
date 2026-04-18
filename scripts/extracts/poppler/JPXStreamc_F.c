// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 6/31



    // check for a naked JPEG 2000 codestream (without the JP2/JPX
    // wrapper) -- this appears to be a violation of the PDF spec, but
    // Acrobat allows it
    if (bufStr->lookChar() == 0xff) {
        cover(7);
        error(errSyntaxWarning, getPos(), "Naked JPEG 2000 codestream, missing JP2/JPX wrapper");
        if (!readCodestream()) {
            return false;
        }
        nComps = img.nComps;
        bpc = (unsigned int *)gmallocn(nComps, sizeof(unsigned int));
        for (i = 0; i < nComps; ++i) {
            bpc[i] = img.tiles[0].tileComps[i].prec;
        }
        width = img.xSize - img.xOffset;
        height = img.ySize - img.yOffset;
        return true;
    }

    while (readBoxHdr(&boxType, &boxLen, &dataLen)) {
        switch (boxType) {
        case 0x6a703268: // JP2 header
            // this is a grouping box ('superbox') which has no real
            // contents and doesn't appear to be used consistently, i.e.,
            // some things which should be subboxes of the JP2 header box
            // show up outside of it - so we simply ignore the JP2 header
            // box
            cover(8);
            break;
        case 0x69686472: // image header
            cover(9);
            if (!readULong(&height) || !readULong(&width) || !readUWord(&nComps) || !readUByte(&bpc1) || !readUByte(&compression) || !readUByte(&unknownColorspace) || !readUByte(&ipr)) {
                error(errSyntaxError, getPos(), "Unexpected EOF in JPX stream");
                return false;
            }
            if (compression != 7) {
                error(errSyntaxError, getPos(), "Unknown compression type in JPX stream");
                return false;
            }
            bpc = (unsigned int *)gmallocn(nComps, sizeof(unsigned int));
            for (i = 0; i < nComps; ++i) {
                bpc[i] = bpc1;
            }
            haveImgHdr = true;
            break;
        case 0x62706363: // bits per component
            cover(10);
            if (!haveImgHdr) {
                error(errSyntaxError, getPos(), "Found bits per component box before image header box in JPX stream");
                return false;
            }
            if (dataLen != nComps) {
                error(errSyntaxError, getPos(), "Invalid bits per component box in JPX stream");
                return false;
            }
            for (i = 0; i < nComps; ++i) {
                if (!readUByte(&bpc[i])) {
                    error(errSyntaxError, getPos(), "Unexpected EOF in JPX stream");
                    return false;
                }
            }
            break;
        case 0x636F6C72: // color specification
            cover(11);
            if (!readColorSpecBox(dataLen)) {
                return false;
            }
            break;
        case 0x70636c72: // palette
            cover(12);
            if (!readUWord(&palette.nEntries) || !readUByte(&palette.nComps)) {
                error(errSyntaxError, getPos(), "Unexpected EOF in JPX stream");
                return false;
            }
            palette.bpc = (unsigned int *)gmallocn(palette.nComps, sizeof(unsigned int));
            palette.c = (int *)gmallocn(palette.nEntries * palette.nComps, sizeof(int));
            for (i = 0; i < palette.nComps; ++i) {
                if (!readUByte(&palette.bpc[i])) {
                    error(errSyntaxError, getPos(), "Unexpected EOF in JPX stream");
                    return false;
                }
                ++palette.bpc[i];
            }
            for (i = 0; i < palette.nEntries; ++i) {
                for (j = 0; j < palette.nComps; ++j) {
                    if (!readNBytes(((palette.bpc[j] & 0x7f) + 7) >> 3, (palette.bpc[j] & 0x80) ? true : false, &palette.c[i * palette.nComps + j])) {
                        error(errSyntaxError, getPos(), "Unexpected EOF in JPX stream");
                        return false;
                    }
                }
            }
            havePalette = true;
            break;
        case 0x636d6170: // component mapping
            cover(13);
            compMap.nChannels = dataLen / 4;
            compMap.comp = (unsigned int *)gmallocn(compMap.nChannels, sizeof(unsigned int));
            compMap.type = (unsigned int *)gmallocn(compMap.nChannels, sizeof(unsigned int));
            compMap.pComp = (unsigned int *)gmallocn(compMap.nChannels, sizeof(unsigned int));
            for (i = 0; i < compMap.nChannels; ++i) {
                if (!readUWord(&compMap.comp[i]) || !readUByte(&compMap.type[i]) || !readUByte(&compMap.pComp[i])) {
                    error(errSyntaxError, getPos(), "Unexpected EOF in JPX stream");
                    return false;
                }
            }
            haveCompMap = true;
            break;
        case 0x63646566: // channel definition
            cover(14);
            if (!readUWord(&channelDefn.nChannels)) {
                error(errSyntaxError, getPos(), "Unexpected EOF in JPX stream"