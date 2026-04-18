// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 32/41



// NB: this is only called when ocState is false.
void Gfx::doIncCharCount(const std::string &s)
{
    if (out->needCharCount()) {
        out->incCharCount(s.size());
    }
}

//------------------------------------------------------------------------
// XObject operators
//------------------------------------------------------------------------

void Gfx::opXObject(Object args[], int /*numArgs*/)
{
    const char *name;

    if (!ocState && !out->needCharCount()) {
        return;
    }
    name = args[0].getName();
    Object obj1 = res->lookupXObject(name);
    if (obj1.isNull()) {
        return;
    }
    if (!obj1.isStream()) {
        error(errSyntaxError, getPos(), "XObject '{0:s}' is wrong type", name);
        return;
    }

    Object opiDict = obj1.streamGetDict()->lookup("OPI");
    if (opiDict.isDict()) {
        out->opiBegin(state, *opiDict.getDict());
    }
    Object obj2 = obj1.streamGetDict()->lookup("Subtype");
    if (obj2.isName("Image")) {
        if (out->needNonText()) {
            Object refObj = res->lookupXObjectNF(name);
            doImage(&refObj, obj1.getStream(), false);
        }
    } else if (obj2.isName("Form")) {
        Object refObj = res->lookupXObjectNF(name);
        bool shouldDoForm = true;
        std::set<int>::iterator drawingFormIt;
        if (refObj.isRef()) {
            const int num = refObj.getRef().num;
            bool inserted;
            std::tie(drawingFormIt, inserted) = formsDrawing.insert(num);
            if (!inserted) {
                shouldDoForm = false;
            }
        }
        if (shouldDoForm) {
            if (out->useDrawForm() && refObj.isRef()) {
                out->drawForm(refObj.getRef());
            } else {
                Ref ref = refObj.isRef() ? refObj.getRef() : Ref::INVALID();
                out->beginForm(&obj1, ref);
                doForm(&obj1);
                out->endForm(&obj1, ref);
            }
        }
        if (refObj.isRef() && shouldDoForm) {
            formsDrawing.erase(drawingFormIt);
        }
    } else if (obj2.isName("PS")) {
        Object obj3 = obj1.streamGetDict()->lookup("Level1");
        out->psXObject(obj1.getStream(), obj3.isStream() ? obj3.getStream() : nullptr);
    } else if (obj2.isName()) {
        error(errSyntaxError, getPos(), "Unknown XObject subtype '{0:s}'", obj2.getName());
    } else {
        error(errSyntaxError, getPos(), "XObject subtype is missing or wrong type");
    }
    if (opiDict.isDict()) {
        out->opiEnd(state, *opiDict.getDict());
    }
}

void Gfx::doImage(Object *ref, Stream *str, bool inlineImg)
{
    Dict *dict, *maskDict;
    int width, height;
    int bits, maskBits;
    bool interpolate;
    StreamColorSpaceMode csMode;
    bool mask;
    bool invert;
    bool haveColorKeyMask, haveExplicitMask, haveSoftMask;
    int maskColors[2 * gfxColorMaxComps] = {};
    int maskWidth, maskHeight;
    bool maskInvert;
    bool maskInterpolate;
    bool hasAlpha;
    Stream *maskStr;
    int i, n;

    // get info from the stream
    bits = 0;
    csMode = streamCSNone;
#if ENABLE_LIBOPENJPEG
    if (str->getKind() == strJPX && out->supportJPXtransparency()) {
        auto *jpxStream = dynamic_cast<JPXStream *>(str);
        jpxStream->setSupportJPXtransparency(true);
    }
#endif
    str->getImageParams(&bits, &csMode, &hasAlpha);

    // get stream dict
    dict = str->getDict();

    // check for optional content key
    if (ref) {
        const Object &objOC = dict->lookupNF("OC");
        if (catalog->getOptContentConfig() && !catalog->getOptContentConfig()->optContentIsVisible(&objOC)) {
            return;
        }
    }

    const std::array<double, 6> &ctm = state->getCTM();
    const double det = ctm[0] * ctm[3] - ctm[1] * ctm[2];
    // Detect singular matrix (non invertible) to avoid drawing Image in such case
    const bool singular_matrix = fabs(det) < 0.000001;

    // get size
    Object obj1 = dict->lookup("Width");
    if (obj1.isNull()) {
        obj1 = dict->lookup("W");
    }
    if (obj1.isInt()) {
        width = obj1.getInt();
    } else if (obj1.isReal()) {
        width = (int)obj1.getReal();
    } else {
        goto err1;
    }
    obj1 = dict->lookup("Height");
    if (obj1.isNull()) {
        obj1 = dict->lookup("H");
    }
    if (obj1.isInt()) {
        height = obj1.getInt();
    } else if (obj1.isReal()) {
        height = (int)obj1.getReal();
    } else {
        goto err1;
    }

    if (width < 1 || height < 1 || width > INT_MAX / height) {
        goto err1;
    }

    // image interpolation
    obj1 = dict->lookup("Interpolate");
    if (obj1.isNull()) {
        obj1 = dict->lookup("I");
    }
    if (obj1.isBool()) {
        interpolate = obj1.getBool();
    } else {
        interpolate = false;
    }
    maskInterpolate = false;