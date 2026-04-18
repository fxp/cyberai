// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 37/41



            Object obj1 = res->lookupGState(dict->getKey(i));
            if (obj1.isDict()) {
                Object obj2 = obj1.dictLookup("BM");
                if (!obj2.isNull()) {
                    if (GfxState::parseBlendMode(&obj2, &mode)) {
                        if (mode != gfxBlendNormal) {
                            transpGroup = true;
                        }
                    } else {
                        error(errSyntaxError, getPos(), "Invalid blend mode in ExtGState");
                    }
                }
                obj2 = obj1.dictLookup("ca");
                if (obj2.isNum()) {
                    opac = obj2.getNum();
                    opac = opac < 0 ? 0 : opac > 1 ? 1 : opac;
                    if (opac != 1) {
                        transpGroup = true;
                    }
                }
                obj2 = obj1.dictLookup("CA");
                if (obj2.isNum()) {
                    opac = obj2.getNum();
                    opac = opac < 0 ? 0 : opac > 1 ? 1 : opac;
                    if (opac != 1) {
                        transpGroup = true;
                    }
                }
                // alpha is shape
                obj2 = obj1.dictLookup("AIS");
                if (!transpGroup && obj2.isBool()) {
                    transpGroup = obj2.getBool();
                }
                // soft mask
                obj2 = obj1.dictLookup("SMask");
                if (!transpGroup && !obj2.isNull()) {
                    if (!obj2.isName("None")) {
                        transpGroup = true;
                    }
                }
            }
        }
    }
    popResources();
    return transpGroup;
}

void Gfx::doForm(Object *str)
{
    Dict *dict;
    bool transpGroup, isolated, knockout;
    std::array<double, 6> m;
    std::array<double, 4> bbox;
    Dict *resDict;
    bool ocSaved;
    Object obj1;
    int i;

    // get stream dict
    dict = str->streamGetDict();

    // check form type
    obj1 = dict->lookup("FormType");
    if (!obj1.isNull() && (!obj1.isInt() || obj1.getInt() != 1)) {
        error(errSyntaxError, getPos(), "Unknown form type");
    }

    Object lengthObj = dict->lookup("Length");
    if (lengthObj.isInt() && (lengthObj.getInt() == 0)) {
        return;
    }

    // check for optional content key
    ocSaved = ocState;
    const Object &objOC = dict->lookupNF("OC");
    if (catalog->getOptContentConfig() && !catalog->getOptContentConfig()->optContentIsVisible(&objOC)) {
        if (out->needCharCount()) {
            ocState = false;
        } else {
            return;
        }
    }

    // get bounding box
    Object bboxObj = dict->lookup("BBox");
    if (!bboxObj.isArray()) {
        error(errSyntaxError, getPos(), "Bad form bounding box");
        ocState = ocSaved;
        return;
    }
    for (i = 0; i < 4; ++i) {
        obj1 = bboxObj.arrayGet(i);
        if (likely(obj1.isNum())) {
            bbox[i] = obj1.getNum();
        } else {
            error(errSyntaxError, getPos(), "Bad form bounding box value");
            return;
        }
    }

    // get matrix
    Object matrixObj = dict->lookup("Matrix");
    if (matrixObj.isArray()) {
        for (i = 0; i < 6; ++i) {
            obj1 = matrixObj.arrayGet(i);
            if (likely(obj1.isNum())) {
                m[i] = obj1.getNum();
            } else {
                m[i] = 0;
            }
        }
    } else {
        m[0] = 1;
        m[1] = 0;
        m[2] = 0;
        m[3] = 1;
        m[4] = 0;
        m[5] = 0;
    }

    // get resources
    Object resObj = dict->lookup("Resources");
    resDict = resObj.isDict() ? resObj.getDict() : nullptr;

    // check for a transparency group
    transpGroup = isolated = knockout = false;
    std::unique_ptr<GfxColorSpace> blendingColorSpace;
    obj1 = dict->lookup("Group");
    if (obj1.isDict()) {
        Object obj2 = obj1.dictLookup("S");
        if (obj2.isName("Transparency")) {
            Object obj3 = obj1.dictLookup("CS");
            if (!obj3.isNull()) {
                blendingColorSpace = GfxColorSpace::parse(res, &obj3, out, state);
            }
            obj3 = obj1.dictLookup("I");
            if (obj3.isBool()) {
                isolated = obj3.getBool();
            }
            obj3 = obj1.dictLookup("K");
            if (obj3.isBool()) {
                knockout = obj3.getBool();
            }
            transpGroup = isolated || out->checkTransparencyGroup(state, knockout) || checkTransparencyGroup(resDict);
        }
    }

    // draw it
    drawForm(str, resDict, m, bbox, transpGroup, false, blendingColorSpace.get(), isolated, knockout);

    ocState = ocSaved;
}