// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 11/41



void Gfx::doSoftMask(Object *str, bool alpha, GfxColorSpace *blendingColorSpace, bool isolated, bool knockout, Function *transferFunc, GfxColor *backdropColor)
{
    Dict *dict, *resDict;
    std::array<double, 6> m;
    std::array<double, 4> bbox;
    Object obj1;
    int i;

    // get stream dict
    dict = str->streamGetDict();

    // check form type
    obj1 = dict->lookup("FormType");
    if (!obj1.isNull() && (!obj1.isInt() || obj1.getInt() != 1)) {
        error(errSyntaxError, getPos(), "Unknown form type");
    }

    // get bounding box
    obj1 = dict->lookup("BBox");
    if (!obj1.isArray()) {
        error(errSyntaxError, getPos(), "Bad form bounding box");
        return;
    }
    for (i = 0; i < 4; ++i) {
        Object obj2 = obj1.arrayGet(i);
        if (likely(obj2.isNum())) {
            bbox[i] = obj2.getNum();
        } else {
            error(errSyntaxError, getPos(), "Bad form bounding box (non number)");
            return;
        }
    }

    // get matrix
    obj1 = dict->lookup("Matrix");
    if (obj1.isArray()) {
        for (i = 0; i < 6; ++i) {
            Object obj2 = obj1.arrayGet(i);
            if (likely(obj2.isNum())) {
                m[i] = obj2.getNum();
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
    obj1 = dict->lookup("Resources");
    resDict = obj1.isDict() ? obj1.getDict() : nullptr;

    // draw it
    drawForm(str, resDict, m, bbox, true, true, blendingColorSpace, isolated, knockout, alpha, transferFunc, backdropColor);
}

void Gfx::opSetRenderingIntent(Object args[], int /*numArgs*/)
{
    state->setRenderingIntent(args[0].getName());
}

//------------------------------------------------------------------------
// color operators
//------------------------------------------------------------------------

void Gfx::opSetFillGray(Object args[], int /*numArgs*/)
{
    if (displayTypes.top() == DisplayType::Type3Font && type3FontIsD1.top()) {
        return;
    }

    GfxColor color;
    std::unique_ptr<GfxColorSpace> colorSpace;

    state->setFillPattern(nullptr);
    Object obj = res->lookupColorSpace("DefaultGray");
    if (!obj.isNull()) {
        colorSpace = GfxColorSpace::parse(res, &obj, out, state);
    }
    if (!colorSpace || colorSpace->getNComps() > 1) {
        colorSpace = state->copyDefaultGrayColorSpace();
    }
    state->setFillColorSpace(std::move(colorSpace));
    out->updateFillColorSpace(state);
    color.c[0] = dblToCol(args[0].getNum());
    state->setFillColor(&color);
    out->updateFillColor(state);
}

void Gfx::opSetStrokeGray(Object args[], int /*numArgs*/)
{
    if (displayTypes.top() == DisplayType::Type3Font && type3FontIsD1.top()) {
        return;
    }

    GfxColor color;
    std::unique_ptr<GfxColorSpace> colorSpace;

    state->setStrokePattern(nullptr);
    Object obj = res->lookupColorSpace("DefaultGray");
    if (!obj.isNull()) {
        colorSpace = GfxColorSpace::parse(res, &obj, out, state);
    }
    if (!colorSpace) {
        colorSpace = state->copyDefaultGrayColorSpace();
    }
    state->setStrokeColorSpace(std::move(colorSpace));
    out->updateStrokeColorSpace(state);
    color.c[0] = dblToCol(args[0].getNum());
    state->setStrokeColor(&color);
    out->updateStrokeColor(state);
}

void Gfx::opSetFillCMYKColor(Object args[], int /*numArgs*/)
{
    if (displayTypes.top() == DisplayType::Type3Font && type3FontIsD1.top()) {
        return;
    }

    GfxColor color;
    std::unique_ptr<GfxColorSpace> colorSpace;
    int i;

    Object obj = res->lookupColorSpace("DefaultCMYK");
    if (!obj.isNull()) {
        colorSpace = GfxColorSpace::parse(res, &obj, out, state);
    }
    if (!colorSpace) {
        colorSpace = state->copyDefaultCMYKColorSpace();
    }
    state->setFillPattern(nullptr);
    state->setFillColorSpace(std::move(colorSpace));
    out->updateFillColorSpace(state);
    for (i = 0; i < 4; ++i) {
        color.c[i] = dblToCol(args[i].getNum());
    }
    state->setFillColor(&color);
    out->updateFillColor(state);
}

void Gfx::opSetStrokeCMYKColor(Object args[], int /*numArgs*/)
{
    if (displayTypes.top() == DisplayType::Type3Font && type3FontIsD1.top()) {
        return;
    }

    GfxColor color;
    std::unique_ptr<GfxColorSpace> colorSpace;
    int i;

    state->setStrokePattern(nullptr);
    Object obj = res->lookupColorSpace("DefaultCMYK");
    if (!obj.isNull()) {
        colorSpace = GfxColorSpace::parse(res, &obj, out, state);
    }
    if (!colorSpace) {
        colorSpace = state->copyDefaultCMYKColorSpace();
    }
    state->setStrokeColorSpace(std::move(colorSpace));
    out->updateStrokeColorSpace(state);
    for (i = 0; i < 4; ++i) {
        color.c[i] = dblToCol(args[i].getNum());
    }
    state->setStrokeColor(&color);
    out->updateStrokeColor(state);
}