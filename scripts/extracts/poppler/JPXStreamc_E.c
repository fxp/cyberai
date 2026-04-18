// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 5/31



    csPrec = 0; // make gcc happy
    *hasAlpha = false;
    haveBPC = haveCSMode = false;
    (void)bufStr->rewind();
    if (bufStr->lookChar() == 0xff) {
        getImageParams2(bitsPerComponent, csMode);
    } else {
        while (readBoxHdr(&boxType, &boxLen, &dataLen)) {
            if (boxType == 0x6a703268) { // JP2 header
                cover(0);
                // skip the superbox
            } else if (boxType == 0x69686472) { // image header
                cover(1);
                if (readULong(&dummy) && readULong(&dummy) && readUWord(&dummy) && readUByte(&bpc1) && readUByte(&dummy) && readUByte(&dummy) && readUByte(&dummy)) {
                    *bitsPerComponent = bpc1 + 1;
                    haveBPC = true;
                }
            } else if (boxType == 0x636F6C72) { // color specification
                cover(2);
                if (readByte(&csMeth) && readByte(&csPrec1) && readByte(&dummy2)) {
                    if (csMeth == 1) {
                        if (readULong(&csEnum)) {
                            csMode1 = streamCSNone;
                            if (csEnum == jpxCSBiLevel || csEnum == jpxCSGrayscale) {
                                csMode1 = streamCSDeviceGray;
                            } else if (csEnum == jpxCSCMYK) {
                                csMode1 = streamCSDeviceCMYK;
                            } else if (csEnum == jpxCSsRGB || csEnum == jpxCSCISesRGB || csEnum == jpxCSROMMRGB) {
                                csMode1 = streamCSDeviceRGB;
                            }
                            if (csMode1 != streamCSNone && (!haveCSMode || csPrec1 > csPrec)) {
                                *csMode = csMode1;
                                csPrec = csPrec1;
                                haveCSMode = true;
                            }
                            if (dataLen >= 7) {
                                for (i = 0; i < dataLen - 7; ++i) {
                                    if (bufStr->getChar() == EOF)
                                        break;
                                }
                            }
                        }
                    } else {
                        if (dataLen >= 3) {
                            for (i = 0; i < dataLen - 3; ++i) {
                                if (bufStr->getChar() == EOF)
                                    break;
                            }
                        }
                    }
                }
            } else if (boxType == 0x6A703263) { // codestream
                cover(3);
                if (!(haveBPC && haveCSMode)) {
                    getImageParams2(bitsPerComponent, csMode);
                }
                break;
            } else {
                cover(4);
                for (i = 0; i < dataLen; ++i) {
                    if (unlikely(bufStr->getChar() == EOF)) {
                        error(errSyntaxError, getPos(), "Unexpected EOF in getImageParams in JPX stream");
                        break;
                    }
                }
            }
        }
    }
    bufStr->close();
}

// Get image parameters from the codestream.
void JPXStream::getImageParams2(int *bitsPerComponent, StreamColorSpaceMode *csMode)
{
    int segType;
    unsigned int segLen, nComps1, bpc1, dummy, i;

    while (readMarkerHdr(&segType, &segLen)) {
        if (segType == 0x51) { // SIZ - image and tile size
            cover(5);
            if (readUWord(&dummy) && readULong(&dummy) && readULong(&dummy) && readULong(&dummy) && readULong(&dummy) && readULong(&dummy) && readULong(&dummy) && readULong(&dummy) && readULong(&dummy) && readUWord(&nComps1)
                && readUByte(&bpc1)) {
                *bitsPerComponent = (bpc1 & 0x7f) + 1;
                // if there's no color space info, take a guess
                if (nComps1 == 1) {
                    *csMode = streamCSDeviceGray;
                } else if (nComps1 == 3) {
                    *csMode = streamCSDeviceRGB;
                } else if (nComps1 == 4) {
                    *csMode = streamCSDeviceCMYK;
                }
            }
            break;
        } else {
            cover(6);
            if (segLen > 2) {
                for (i = 0; i < segLen - 2; ++i) {
                    bufStr->getChar();
                }
            }
        }
    }
}

bool JPXStream::readBoxes()
{
    unsigned int boxType, boxLen, dataLen;
    unsigned int bpc1, compression, unknownColorspace, ipr;
    unsigned int i, j;

    haveImgHdr = false;

    // initialize in case there is a parse error
    img.xSize = img.ySize = 0;
    img.xOffset = img.yOffset = 0;
    img.xTileSize = img.yTileSize = 0;
    img.xTileOffset = img.yTileOffset = 0;
    img.nComps = 0;