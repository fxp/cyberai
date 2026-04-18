// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 36/41



            if (maskStr == nullptr) {
                maskStr = maskObj.getStream();
                maskDict = maskObj.streamGetDict();
            }
            obj1 = maskDict->lookup("Width");
            if (obj1.isNull()) {
                obj1 = maskDict->lookup("W");
            }
            if (!obj1.isInt()) {
                goto err1;
            }
            maskWidth = obj1.getInt();
            obj1 = maskDict->lookup("Height");
            if (obj1.isNull()) {
                obj1 = maskDict->lookup("H");
            }
            if (!obj1.isInt()) {
                goto err1;
            }
            maskHeight = obj1.getInt();
            obj1 = maskDict->lookup("Interpolate");
            if (obj1.isNull()) {
                obj1 = maskDict->lookup("I");
            }
            if (obj1.isBool()) {
                maskInterpolate = obj1.getBool();
            } else {
                maskInterpolate = false;
            }

            obj1 = maskDict->lookup("ImageMask");
            if (obj1.isNull()) {
                obj1 = maskDict->lookup("IM");
            }
            if (!haveMaskImage && (!obj1.isBool() || !obj1.getBool())) {
                goto err1;
            }

            maskInvert = false;
            obj1 = maskDict->lookup("Decode");
            if (obj1.isNull()) {
                obj1 = maskDict->lookup("D");
            }
            if (obj1.isArray()) {
                Object obj2 = obj1.arrayGet(0);
                // Table 4.39 says /Decode must be [1 0] or [0 1]. Adobe
                // accepts [1.0 0.0] as well.
                if (obj2.isNum() && obj2.getNum() >= 0.9) {
                    maskInvert = true;
                }
            } else if (!obj1.isNull()) {
                goto err1;
            }

            haveExplicitMask = true;
        }

        // if drawing is disabled, skip over inline image data
        if (!ocState || !out->needNonText() || singular_matrix) {
            if (!str->rewind()) {
                goto err1;
            }
            n = height * ((width * colorMap.getNumPixelComps() * colorMap.getBits() + 7) / 8);
            for (i = 0; i < n; ++i) {
                str->getChar();
            }
            str->close();

            // draw it
        } else {
            if (haveSoftMask) {
                out->drawSoftMaskedImage(state, ref, str, width, height, &colorMap, interpolate, maskStr, maskWidth, maskHeight, maskColorMap.get(), maskInterpolate);
            } else if (haveExplicitMask) {
                out->drawMaskedImage(state, ref, str, width, height, &colorMap, interpolate, maskStr, maskWidth, maskHeight, maskInvert, maskInterpolate);
            } else {
                out->drawImage(state, ref, str, width, height, &colorMap, interpolate, haveColorKeyMask ? maskColors : nullptr, inlineImg);
            }
        }
    }

    if ((i = width * height) > 1000) {
        i = 1000;
    }
    updateLevel += i;

    return;

err1:
    error(errSyntaxError, getPos(), "Bad image parameters");
}

bool Gfx::checkTransparencyGroup(Dict *resDict)
{
    // check the effect of compositing objects as a group:
    // look for ExtGState entries with ca != 1 or CA != 1 or BM != normal
    bool transpGroup = false;
    double opac;

    if (resDict == nullptr) {
        return false;
    }
    pushResources(resDict);
    Object extGStates = resDict->lookup("ExtGState");
    if (extGStates.isDict()) {
        Dict *dict = extGStates.getDict();
        for (int i = 0; i < dict->getLength() && !transpGroup; i++) {
            GfxBlendMode mode;