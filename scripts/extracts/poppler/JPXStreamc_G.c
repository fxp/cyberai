// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 7/31

);
                return false;
            }
            channelDefn.idx = (unsigned int *)gmallocn(channelDefn.nChannels, sizeof(unsigned int));
            channelDefn.type = (unsigned int *)gmallocn(channelDefn.nChannels, sizeof(unsigned int));
            channelDefn.assoc = (unsigned int *)gmallocn(channelDefn.nChannels, sizeof(unsigned int));
            for (i = 0; i < channelDefn.nChannels; ++i) {
                if (!readUWord(&channelDefn.idx[i]) || !readUWord(&channelDefn.type[i]) || !readUWord(&channelDefn.assoc[i])) {
                    error(errSyntaxError, getPos(), "Unexpected EOF in JPX stream");
                    return false;
                }
            }
            haveChannelDefn = true;
            break;
        case 0x6A703263: // contiguous codestream
            cover(15);
            if (!bpc) {
                error(errSyntaxError, getPos(), "JPX stream is missing the image header box");
            }
            if (!haveCS) {
                error(errSyntaxError, getPos(), "JPX stream has no supported color spec");
            }
            if (!readCodestream()) {
                return false;
            }
            break;
        default:
            cover(16);
            for (i = 0; i < dataLen; ++i) {
                if (bufStr->getChar() == EOF) {
                    error(errSyntaxError, getPos(), "Unexpected EOF in JPX stream");
                    return false;
                }
            }
            break;
        }
    }
    return true;
}

bool JPXStream::readColorSpecBox(unsigned int dataLen)
{
    JPXColorSpec newCS;
    unsigned int csApprox, csEnum;
    unsigned int i;
    bool ok;

    ok = false;
    if (!readUByte(&newCS.meth) || !readByte(&newCS.prec) || !readUByte(&csApprox)) {
        goto err;
    }
    switch (newCS.meth) {
    case 1: // enumerated colorspace
        cover(17);
        if (!readULong(&csEnum)) {
            goto err;
        }
        newCS.enumerated.type = (JPXColorSpaceType)csEnum;
        switch (newCS.enumerated.type) {
        case jpxCSBiLevel:
            ok = true;
            break;
        case jpxCSYCbCr1:
            ok = true;
            break;
        case jpxCSYCbCr2:
            ok = true;
            break;
        case jpxCSYCBCr3:
            ok = true;
            break;
        case jpxCSPhotoYCC:
            ok = true;
            break;
        case jpxCSCMY:
            ok = true;
            break;
        case jpxCSCMYK:
            ok = true;
            break;
        case jpxCSYCCK:
            ok = true;
            break;
        case jpxCSCIELab:
            if (dataLen == 7 + 7 * 4) {
                if (!readULong(&newCS.enumerated.cieLab.rl) || !readULong(&newCS.enumerated.cieLab.ol) || !readULong(&newCS.enumerated.cieLab.ra) || !readULong(&newCS.enumerated.cieLab.oa) || !readULong(&newCS.enumerated.cieLab.rb)
                    || !readULong(&newCS.enumerated.cieLab.ob) || !readULong(&newCS.enumerated.cieLab.il)) {
                    goto err;
                }
            } else if (dataLen == 7) {
                //~ this assumes the 8-bit case
                cover(92);
                newCS.enumerated.cieLab.rl = 100;
                newCS.enumerated.cieLab.ol = 0;
                newCS.enumerated.cieLab.ra = 255;
                newCS.enumerated.cieLab.oa = 128;
                newCS.enumerated.cieLab.rb = 255;
                newCS.enumerated.cieLab.ob = 96;
                newCS.enumerated.cieLab.il = 0x00443530;
            } else {
                goto err;
            }
            ok = true;
            break;
        case jpxCSsRGB:
            ok = true;
            break;
        case jpxCSGrayscale:
            ok = true;
            break;
        case jpxCSBiLevel2:
            ok = true;
            break;
        case jpxCSCIEJab:
            // not allowed in PDF
            goto err;
        case jpxCSCISesRGB:
            ok = true;
            break;
        case jpxCSROMMRGB:
            ok = true;
            break;
        case jpxCSsRGBYCbCr:
            ok = true;
            break;
        case jpxCSYPbPr1125:
            ok = true;
            break;
        case jpxCSYPbPr1250:
            ok = true;
            break;
        default:
            goto err;
        }
        break;
    case 2: // restricted ICC profile
    case 3: // any ICC profile (JPX)
    case 4: // vendor color (JPX)
        cover(18);
        for (i = 0; i < dataLen - 3; ++i) {
            if (bufStr->getChar() == EOF) {
                goto err;
            }
        }
        break;
    }

    if (ok && (!haveCS || newCS.prec > cs.prec)) {
        cs = newCS;
        haveCS = true;
    }

    return true;

err:
    error(errSyntaxError, getPos(), "Error in JPX color spec");
    return false;
}