// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 12/41



void Gfx::opSetFillRGBColor(Object args[], int /*numArgs*/)
{
    if (displayTypes.top() == DisplayType::Type3Font && type3FontIsD1.top()) {
        return;
    }

    std::unique_ptr<GfxColorSpace> colorSpace;
    GfxColor color;
    int i;

    state->setFillPattern(nullptr);
    Object obj = res->lookupColorSpace("DefaultRGB");
    if (!obj.isNull()) {
        colorSpace = GfxColorSpace::parse(res, &obj, out, state);
    }
    if (!colorSpace || colorSpace->getNComps() > 3) {
        colorSpace = state->copyDefaultRGBColorSpace();
    }
    state->setFillColorSpace(std::move(colorSpace));
    out->updateFillColorSpace(state);
    for (i = 0; i < 3; ++i) {
        color.c[i] = dblToCol(args[i].getNum());
    }
    state->setFillColor(&color);
    out->updateFillColor(state);
}

void Gfx::opSetStrokeRGBColor(Object args[], int /*numArgs*/)
{
    if (displayTypes.top() == DisplayType::Type3Font && type3FontIsD1.top()) {
        return;
    }

    std::unique_ptr<GfxColorSpace> colorSpace;
    GfxColor color;
    int i;

    state->setStrokePattern(nullptr);
    Object obj = res->lookupColorSpace("DefaultRGB");
    if (!obj.isNull()) {
        colorSpace = GfxColorSpace::parse(res, &obj, out, state);
    }
    if (!colorSpace) {
        colorSpace = state->copyDefaultRGBColorSpace();
    }
    state->setStrokeColorSpace(std::move(colorSpace));
    out->updateStrokeColorSpace(state);
    for (i = 0; i < 3; ++i) {
        color.c[i] = dblToCol(args[i].getNum());
    }
    state->setStrokeColor(&color);
    out->updateStrokeColor(state);
}

void Gfx::opSetFillColorSpace(Object args[], int /*numArgs*/)
{
    if (displayTypes.top() == DisplayType::Type3Font && type3FontIsD1.top()) {
        return;
    }

    std::unique_ptr<GfxColorSpace> colorSpace;
    GfxColor color;

    Object obj = res->lookupColorSpace(args[0].getName());
    if (obj.isNull()) {
        colorSpace = GfxColorSpace::parse(res, &args[0], out, state);
    } else {
        colorSpace = GfxColorSpace::parse(res, &obj, out, state);
    }
    if (colorSpace) {
        state->setFillPattern(nullptr);
        state->setFillColorSpace(std::move(colorSpace));
        out->updateFillColorSpace(state);
        state->getFillColorSpace()->getDefaultColor(&color);
        state->setFillColor(&color);
        out->updateFillColor(state);
    } else {
        error(errSyntaxError, getPos(), "Bad color space (fill)");
    }
}

void Gfx::opSetStrokeColorSpace(Object args[], int /*numArgs*/)
{
    if (displayTypes.top() == DisplayType::Type3Font && type3FontIsD1.top()) {
        return;
    }

    std::unique_ptr<GfxColorSpace> colorSpace;
    GfxColor color;

    state->setStrokePattern(nullptr);
    Object obj = res->lookupColorSpace(args[0].getName());
    if (obj.isNull()) {
        colorSpace = GfxColorSpace::parse(res, &args[0], out, state);
    } else {
        colorSpace = GfxColorSpace::parse(res, &obj, out, state);
    }
    if (colorSpace) {
        state->setStrokeColorSpace(std::move(colorSpace));
        out->updateStrokeColorSpace(state);
        state->getStrokeColorSpace()->getDefaultColor(&color);
        state->setStrokeColor(&color);
        out->updateStrokeColor(state);
    } else {
        error(errSyntaxError, getPos(), "Bad color space (stroke)");
    }
}

void Gfx::opSetFillColor(Object args[], int numArgs)
{
    if (displayTypes.top() == DisplayType::Type3Font && type3FontIsD1.top()) {
        return;
    }

    GfxColor color;
    int i;

    if (numArgs != state->getFillColorSpace()->getNComps()) {
        error(errSyntaxError, getPos(), "Incorrect number of arguments in 'sc' command");
        return;
    }
    state->setFillPattern(nullptr);
    for (i = 0; i < numArgs; ++i) {
        color.c[i] = dblToCol(args[i].getNum());
    }
    state->setFillColor(&color);
    out->updateFillColor(state);
}

void Gfx::opSetStrokeColor(Object args[], int numArgs)
{
    if (displayTypes.top() == DisplayType::Type3Font && type3FontIsD1.top()) {
        return;
    }

    GfxColor color;
    int i;

    if (numArgs != state->getStrokeColorSpace()->getNComps()) {
        error(errSyntaxError, getPos(), "Incorrect number of arguments in 'SC' command");
        return;
    }
    state->setStrokePattern(nullptr);
    for (i = 0; i < numArgs; ++i) {
        color.c[i] = dblToCol(args[i].getNum());
    }
    state->setStrokeColor(&color);
    out->updateStrokeColor(state);
}

void Gfx::opSetFillColorN(Object args[], int numArgs)
{
    if (displayTypes.top() == DisplayType::Type3Font && type3FontIsD1.top()) {
        return;
    }

    GfxColor color;
    int i;