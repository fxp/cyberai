// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 33/41



    // image or mask?
    obj1 = dict->lookup("ImageMask");
    if (obj1.isNull()) {
        obj1 = dict->lookup("IM");
    }
    mask = false;
    if (obj1.isBool()) {
        mask = obj1.getBool();
    } else if (!obj1.isNull()) {
        goto err1;
    }

    // bit depth
    if (bits == 0) {
        obj1 = dict->lookup("BitsPerComponent");
        if (obj1.isNull()) {
            obj1 = dict->lookup("BPC");
        }
        if (obj1.isInt()) {
            bits = obj1.getInt();
        } else if (mask) {
            bits = 1;
        } else {
            goto err1;
        }
    }

    // display a mask
    if (mask) {

        // check for inverted mask
        if (bits != 1) {
            goto err1;
        }
        invert = false;
        obj1 = dict->lookup("Decode");
        if (obj1.isNull()) {
            obj1 = dict->lookup("D");
        }
        if (obj1.isArray()) {
            Object obj2;
            obj2 = obj1.arrayGet(0);
            // Table 4.39 says /Decode must be [1 0] or [0 1]. Adobe
            // accepts [1.0 0.0] as well.
            if (obj2.isNum() && obj2.getNum() >= 0.9) {
                invert = true;
            }
        } else if (!obj1.isNull()) {
            goto err1;
        }

        // if drawing is disabled, skip over inline image data
        if (!ocState || !out->needNonText()) {
            if (!str->rewind()) {
                goto err1;
            }
            n = height * ((width + 7) / 8);
            for (i = 0; i < n; ++i) {
                str->getChar();
            }
            str->close();

            // draw it
        } else {
            if (state->getFillColorSpace()->getMode() == csPattern) {
                doPatternImageMask(ref, str, width, height, invert, inlineImg);
            } else {
                out->drawImageMask(state, ref, str, width, height, invert, interpolate, inlineImg);
            }
        }
    } else {
        if (bits == 0) {
            goto err1;
        }

        // get color space and color map
        obj1 = dict->lookup("ColorSpace");
        if (obj1.isNull()) {
            obj1 = dict->lookup("CS");
        }
        bool haveColorSpace = !obj1.isNull();
        bool haveRGBA = false;
        if (str->getKind() == strJPX && out->supportJPXtransparency() && (csMode == streamCSDeviceRGB || csMode == streamCSDeviceCMYK)) {
            // Case of transparent JPX image, they may contain RGBA data
            // when have no ColorSpace or when SMaskInData=1 · Issue #1486
            if (!haveColorSpace) {
                haveRGBA = hasAlpha;
            } else {
                Object smaskInData = dict->lookup("SMaskInData");
                if (smaskInData.isInt() && smaskInData.getInt()) {
                    haveRGBA = true;
                }
            }
        }

        if (obj1.isName() && inlineImg) {
            Object obj2 = res->lookupColorSpace(obj1.getName());
            if (!obj2.isNull()) {
                obj1 = std::move(obj2);
            }
        }
        std::unique_ptr<GfxColorSpace> colorSpace;