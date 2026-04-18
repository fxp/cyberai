// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 23/49



void GfxPatternColorSpace::getRGB(const GfxColor * /*color*/, GfxRGB *rgb) const
{
    rgb->r = rgb->g = rgb->b = 0;
}

void GfxPatternColorSpace::getCMYK(const GfxColor * /*color*/, GfxCMYK *cmyk) const
{
    cmyk->c = cmyk->m = cmyk->y = 0;
    cmyk->k = 1;
}

void GfxPatternColorSpace::getDeviceN(const GfxColor * /*color*/, GfxColor *deviceN) const
{
    clearGfxColor(deviceN);
    deviceN->c[3] = 1;
}

void GfxPatternColorSpace::getDefaultColor(GfxColor *color) const
{
    color->c[0] = 0;
}

//------------------------------------------------------------------------
// Pattern
//------------------------------------------------------------------------

GfxPattern::GfxPattern(int typeA, int patternRefNumA) : type(typeA), patternRefNum(patternRefNumA) { }

GfxPattern::~GfxPattern() = default;

std::unique_ptr<GfxPattern> GfxPattern::parse(GfxResources *res, Object *obj, OutputDev *out, GfxState *state, int patternRefNum)
{
    Object obj1;

    if (obj->isDict()) {
        obj1 = obj->dictLookup("PatternType");
    } else if (obj->isStream()) {
        obj1 = obj->streamGetDict()->lookup("PatternType");
    } else {
        return {};
    }
    if (obj1.isInt() && obj1.getInt() == 1) {
        return GfxTilingPattern::parse(obj, patternRefNum);
    }
    if (obj1.isInt() && obj1.getInt() == 2) {
        return GfxShadingPattern::parse(res, obj, out, state, patternRefNum);
    }
    return {};
}

//------------------------------------------------------------------------
// GfxTilingPattern
//------------------------------------------------------------------------

std::unique_ptr<GfxTilingPattern> GfxTilingPattern::parse(Object *patObj, int patternRefNum)
{
    Dict *dict;
    int paintTypeA, tilingTypeA;
    double xStepA, yStepA;
    Object resDictA;
    Object obj1;
    int i;

    if (!patObj->isStream()) {
        return nullptr;
    }
    dict = patObj->streamGetDict();

    obj1 = dict->lookup("PaintType");
    if (obj1.isInt()) {
        paintTypeA = obj1.getInt();
    } else {
        paintTypeA = 1;
        error(errSyntaxWarning, -1, "Invalid or missing PaintType in pattern");
    }
    obj1 = dict->lookup("TilingType");
    if (obj1.isInt()) {
        tilingTypeA = obj1.getInt();
    } else {
        tilingTypeA = 1;
        error(errSyntaxWarning, -1, "Invalid or missing TilingType in pattern");
    }
    std::array<double, 4> bboxA;
    bboxA[0] = bboxA[1] = 0;
    bboxA[2] = bboxA[3] = 1;
    obj1 = dict->lookup("BBox");
    if (obj1.isArrayOfLength(4)) {
        for (i = 0; i < 4; ++i) {
            Object obj2 = obj1.arrayGet(i);
            if (obj2.isNum()) {
                bboxA[i] = obj2.getNum();
            }
        }
    } else {
        error(errSyntaxWarning, -1, "Invalid or missing BBox in pattern");
    }
    obj1 = dict->lookup("XStep");
    if (obj1.isNum()) {
        xStepA = obj1.getNum();
    } else {
        xStepA = 1;
        error(errSyntaxWarning, -1, "Invalid or missing XStep in pattern");
    }
    obj1 = dict->lookup("YStep");
    if (obj1.isNum()) {
        yStepA = obj1.getNum();
    } else {
        yStepA = 1;
        error(errSyntaxWarning, -1, "Invalid or missing YStep in pattern");
    }
    resDictA = dict->lookup("Resources");
    if (!resDictA.isDict()) {
        error(errSyntaxWarning, -1, "Invalid or missing Resources in pattern");
    }
    std::array<double, 6> matrixA;
    matrixA[0] = 1;
    matrixA[1] = 0;
    matrixA[2] = 0;
    matrixA[3] = 1;
    matrixA[4] = 0;
    matrixA[5] = 0;
    obj1 = dict->lookup("Matrix");
    if (obj1.isArrayOfLength(6)) {
        for (i = 0; i < 6; ++i) {
            Object obj2 = obj1.arrayGet(i);
            if (obj2.isNum()) {
                matrixA[i] = obj2.getNum();
            }
        }
    }

    auto *pattern = new GfxTilingPattern(paintTypeA, tilingTypeA, bboxA, xStepA, yStepA, &resDictA, matrixA, patObj, patternRefNum);
    return std::unique_ptr<GfxTilingPattern>(pattern);
}

GfxTilingPattern::GfxTilingPattern(int paintTypeA, int tilingTypeA, const std::array<double, 4> &bboxA, double xStepA, double yStepA, const Object *resDictA, const std::array<double, 6> &matrixA, const Object *contentStreamA,
                                   int patternRefNumA)
    : GfxPattern(1, patternRefNumA), bbox(bboxA), matrix(matrixA)
{
    paintType = paintTypeA;
    tilingType = tilingTypeA;
    xStep = xStepA;
    yStep = yStepA;
    resDict = resDictA->copy();
    contentStream = contentStreamA->copy();
}

GfxTilingPattern::~GfxTilingPattern() = default;

std::unique_ptr<GfxPattern> GfxTilingPattern::copy() const
{
    auto *pattern = new GfxTilingPattern(paintType, tilingType, bbox, xStep, yStep, &resDict, matrix, &contentStream, getPatternRefNum());
    return std::unique_ptr<GfxTilingPattern>(pattern);
}

//------------------------------------------------------------------------
// GfxShadingPattern
//------------------------------------------------------------------------