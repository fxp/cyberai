// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 11/31

            error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                        return false;
                    }
                }
            } else {
                error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                return false;
            }
            for (i = 0; i < img.nXTiles * img.nYTiles; ++i) {
                for (comp = 0; comp < img.nComps; ++comp) {
                    if (!(i == 0 && comp == 0)) {
                        img.tiles[i].tileComps[comp].quantStyle = img.tiles[0].tileComps[0].quantStyle;
                        img.tiles[i].tileComps[comp].nQuantSteps = img.tiles[0].tileComps[0].nQuantSteps;
                        img.tiles[i].tileComps[comp].quantSteps = (unsigned int *)greallocn(img.tiles[i].tileComps[comp].quantSteps, img.tiles[0].tileComps[0].nQuantSteps, sizeof(unsigned int));
                        for (j = 0; j < img.tiles[0].tileComps[0].nQuantSteps; ++j) {
                            img.tiles[i].tileComps[comp].quantSteps[j] = img.tiles[0].tileComps[0].quantSteps[j];
                        }
                    }
                }
            }
            haveQCD = true;
            break;
        case 0x5d: // QCC - quantization component
            cover(24);
            if (!haveQCD) {
                error(errSyntaxError, getPos(), "JPX QCC marker segment before QCD segment");
                return false;
            }
            if ((img.nComps > 256 && !readUWord(&comp)) || (img.nComps <= 256 && !readUByte(&comp)) || comp >= img.nComps || !readUByte(&img.tiles[0].tileComps[comp].quantStyle)) {
                error(errSyntaxError, getPos(), "Error in JPX QCC marker segment");
                return false;
            }
            if ((img.tiles[0].tileComps[comp].quantStyle & 0x1f) == 0x00) {
                if (segLen <= (img.nComps > 256 ? 5U : 4U)) {
                    error(errSyntaxError, getPos(), "Error in JPX QCC marker segment");
                    return false;
                }
                img.tiles[0].tileComps[comp].nQuantSteps = segLen - (img.nComps > 256 ? 5 : 4);
                img.tiles[0].tileComps[comp].quantSteps = (unsigned int *)greallocn(img.tiles[0].tileComps[comp].quantSteps, img.tiles[0].tileComps[comp].nQuantSteps, sizeof(unsigned int));
                for (i = 0; i < img.tiles[0].tileComps[comp].nQuantSteps; ++i) {
                    if (!readUByte(&img.tiles[0].tileComps[comp].quantSteps[i])) {
                        error(errSyntaxError, getPos(), "Error in JPX QCC marker segment");
                        return false;
                    }
                }
            } else if ((img.tiles[0].tileComps[comp].quantStyle & 0x1f) == 0x01) {
                img.tiles[0].tileComps[comp].nQuantSteps = 1;
                img.tiles[0].tileComps[comp].quantSteps = (unsigned int *)greallocn(img.tiles[0].tileComps[comp].quantSteps, img.tiles[0].tileComps[comp].nQuantSteps, sizeof(unsigned int));
                if (!readUWord(&img.tiles[0].tileComps[comp].quantSteps[0])) {
                    error(errSyntaxError, getPos(), "Error in JPX QCC marker segment");
                    return false;
                }
            } else if ((img.tiles[0].tileComps[comp].quantStyle & 0x1f) == 0x02) {
                if (segLen < (img.nComps > 256 ? 5U : 4U) + 2) {
                    error(errSyntaxError, getPos(), "Error in JPX QCC marker segment");
                    return false;
                }
                img.tiles[0].tileComps[comp].nQuantSteps = (segLen - (img.nComps > 256 ? 5 : 4)) / 2;
                img.tiles[0].tileComps[comp].quantSteps = (unsigned int *)greallocn(img.tiles[0].tileComps[comp].quantSteps, img.tiles[0].tileComps[comp].nQuantSteps, sizeof(unsigned int));
                for (i = 0; i < img.tiles[0].tileComps[comp].nQuantSteps; ++i) {
                    if (!readUWord(&img.tiles[0].tileComps[comp].quantSteps[i])) {
                        error(errSyntaxError, getPos(), "Error in JPX QCD marker segment");
                        return false;
                    }
                }
            } else {
                error(errSyntaxError, getPos(), "Error in JPX QCC marker segment");
                return false;
            }
            for (i = 1; i < img.nXTiles * img.nYTiles; ++i) {
                img.tiles[i].tileComps[comp].quantStyle = img.tiles[0].tileComps[comp].quantStyle;
                img.tiles[i].tileComps[comp].nQuantSteps = img.tiles[0].tileComps[comp].nQuantSteps;
                img.tiles[i].tileComps[comp].quantSteps = (unsigned int *)greallocn(img.tiles[i].tileComps[comp].quantSteps, img.tiles[0].tileComps[comp].nQuantSteps, sizeof(unsigned int));
                for (j = 0; j < img.tiles[0].tileComps[comp].nQuantSteps; ++j) {
                    img.tiles[i].tileComps[comp].quantSteps[j] = img.tiles[0].tileComps[comp].quantSteps[j];
                }
            }
            break;
        case 0x5e: // RGN - regi