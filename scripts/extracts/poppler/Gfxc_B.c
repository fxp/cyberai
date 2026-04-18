// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 34/41



        if (!obj1.isNull() && !haveRGBA) {
            char *tempIntent = nullptr;
            Object objIntent = dict->lookup("Intent");
            if (objIntent.isName()) {
                const char *stateIntent = state->getRenderingIntent();
                if (stateIntent != nullptr) {
                    tempIntent = strdup(stateIntent);
                }
                state->setRenderingIntent(objIntent.getName());
            }
            colorSpace = GfxColorSpace::parse(res, &obj1, out, state);
            if (objIntent.isName()) {
                state->setRenderingIntent(tempIntent);
                free(tempIntent);
            }
        } else if (csMode == streamCSDeviceGray) {
            Object objCS = res->lookupColorSpace("DefaultGray");
            if (objCS.isNull()) {
                colorSpace = std::make_unique<GfxDeviceGrayColorSpace>();
            } else {
                colorSpace = GfxColorSpace::parse(res, &objCS, out, state);
            }
        } else if (csMode == streamCSDeviceRGB) {
            if (haveRGBA) {
                colorSpace = std::make_unique<GfxDeviceRGBAColorSpace>();
            } else {
                Object objCS = res->lookupColorSpace("DefaultRGB");
                if (objCS.isNull()) {
                    colorSpace = std::make_unique<GfxDeviceRGBColorSpace>();
                } else {
                    colorSpace = GfxColorSpace::parse(res, &objCS, out, state);
                }
            }
        } else if (csMode == streamCSDeviceCMYK) {
            if (haveRGBA) {
                colorSpace = std::make_unique<GfxDeviceRGBAColorSpace>();
            } else {
                Object objCS = res->lookupColorSpace("DefaultCMYK");
                if (objCS.isNull()) {
                    colorSpace = std::make_unique<GfxDeviceCMYKColorSpace>();
                } else {
                    colorSpace = GfxColorSpace::parse(res, &objCS, out, state);
                }
            }
        }
        if (!colorSpace) {
            goto err1;
        }
        obj1 = dict->lookup("Decode");
        if (obj1.isNull()) {
            obj1 = dict->lookup("D");
        }
        GfxImageColorMap colorMap(bits, &obj1, std::move(colorSpace));
        if (!colorMap.isOk()) {
            goto err1;
        }

        // get the mask
        bool haveMaskImage = false;
        haveColorKeyMask = haveExplicitMask = haveSoftMask = false;
        maskStr = nullptr; // make gcc happy
        maskWidth = maskHeight = 0; // make gcc happy
        maskInvert = false; // make gcc happy
        std::unique_ptr<GfxImageColorMap> maskColorMap;
        Object maskObj = dict->lookup("Mask");
        Object smaskObj = dict->lookup("SMask");

        if (maskObj.isStream()) {
            maskStr = maskObj.getStream();
            maskDict = maskObj.streamGetDict();
            // if Type is XObject and Subtype is Image
            // then the way the softmask is drawn will draw
            // correctly, if it falls through to the explicit
            // mask code then you get an error and no image
            // drawn because it expects maskDict to have an entry
            // of Mask or IM that is boolean...
            Object tobj = maskDict->lookup("Type");
            if (!tobj.isNull() && tobj.isName() && tobj.isName("XObject")) {
                Object sobj = maskDict->lookup("Subtype");
                if (!sobj.isNull() && sobj.isName() && sobj.isName("Image")) {
                    // ensure that this mask does not include an ImageMask entry
                    // which signifies the explicit mask
                    obj1 = maskDict->lookup("ImageMask");
                    if (obj1.isNull()) {
                        obj1 = maskDict->lookup("IM");
                    }
                    if (obj1.isNull() || !obj1.isBool()) {
                        haveMaskImage = true;
                    }
                }
            }
        }