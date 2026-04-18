// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 35/41



        if (smaskObj.isStream() || haveMaskImage) {
            // soft mask
            if (inlineImg) {
                goto err1;
            }
            if (!haveMaskImage) {
                maskStr = smaskObj.getStream();
                maskDict = smaskObj.streamGetDict();
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
            obj1 = maskDict->lookup("BitsPerComponent");
            if (obj1.isNull()) {
                obj1 = maskDict->lookup("BPC");
            }
            if (!obj1.isInt()) {
                goto err1;
            }
            maskBits = obj1.getInt();
            obj1 = maskDict->lookup("ColorSpace");
            if (obj1.isNull()) {
                obj1 = maskDict->lookup("CS");
            }
            if (obj1.isName()) {
                Object obj2 = res->lookupColorSpace(obj1.getName());
                if (!obj2.isNull()) {
                    obj1 = std::move(obj2);
                }
            }
            // Here, we parse manually instead of using GfxColorSpace::parse,
            // since we explicitly need DeviceGray and not some DefaultGray color space
            if (!obj1.isName("DeviceGray") && !obj1.isName("G")) {
                goto err1;
            }
            obj1 = maskDict->lookup("Decode");
            if (obj1.isNull()) {
                obj1 = maskDict->lookup("D");
            }
            maskColorMap = std::make_unique<GfxImageColorMap>(maskBits, &obj1, std::make_unique<GfxDeviceGrayColorSpace>());
            if (!maskColorMap->isOk()) {
                goto err1;
            }
            // handle the Matte entry
            obj1 = maskDict->lookup("Matte");
            if (obj1.isArray()) {
                if (obj1.getArray()->getLength() != colorMap.getColorSpace()->getNComps()) {
                    error(errSyntaxError, -1, "Matte entry should have {0:d} components but has {1:d}", colorMap.getColorSpace()->getNComps(), obj1.getArray()->getLength());
                } else if (maskWidth != width || maskHeight != height) {
                    error(errSyntaxError, -1, "Softmask with matte entry {0:d} x {1:d} must have same geometry as the image {2:d} x {3:d}", maskWidth, maskHeight, width, height);
                } else {
                    GfxColor matteColor;
                    for (i = 0; i < colorMap.getColorSpace()->getNComps(); i++) {
                        Object obj2 = obj1.getArray()->get(i);
                        if (!obj2.isNum()) {
                            error(errSyntaxError, -1, "Matte entry {0:d} should be a number but it's of type {1:d}", i, obj2.getType());

                            break;
                        }
                        matteColor.c[i] = dblToCol(obj2.getNum());
                    }
                    if (i == colorMap.getColorSpace()->getNComps()) {
                        maskColorMap->setMatteColor(&matteColor);
                    }
                }
            }
            haveSoftMask = true;
        } else if (maskObj.isArray()) {
            // color key mask
            for (i = 0; i < maskObj.arrayGetLength() && i < 2 * gfxColorMaxComps; ++i) {
                obj1 = maskObj.arrayGet(i);
                if (obj1.isInt()) {
                    maskColors[i] = obj1.getInt();
                } else if (obj1.isReal()) {
                    error(errSyntaxError, -1, "Mask entry should be an integer but it's a real, trying to use it");
                    maskColors[i] = (int)obj1.getReal();
                } else {
                    error(errSyntaxError, -1, "Mask entry should be an integer but it's of type {0:d}", obj1.getType());
                    goto err1;
                }
            }
            haveColorKeyMask = true;
        } else if (maskObj.isStream()) {
            // explicit mask
            if (inlineImg) {
                goto err1;
            }