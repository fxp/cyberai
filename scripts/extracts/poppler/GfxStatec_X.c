// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 24/49



std::unique_ptr<GfxShadingPattern> GfxShadingPattern::parse(GfxResources *res, Object *patObj, OutputDev *out, GfxState *state, int patternRefNum)
{
    Dict *dict;
    Object obj1;
    int i;

    if (!patObj->isDict()) {
        return {};
    }
    dict = patObj->getDict();

    obj1 = dict->lookup("Shading");
    std::unique_ptr<GfxShading> shadingA = GfxShading::parse(res, &obj1, out, state);
    if (!shadingA) {
        return {};
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

    auto *pattern = new GfxShadingPattern(std::move(shadingA), matrixA, patternRefNum);
    return std::unique_ptr<GfxShadingPattern>(pattern);
}

GfxShadingPattern::GfxShadingPattern(std::unique_ptr<GfxShading> &&shadingA, const std::array<double, 6> &matrixA, int patternRefNumA) : GfxPattern(2, patternRefNumA), shading(std::move(shadingA)), matrix(matrixA) { }

GfxShadingPattern::~GfxShadingPattern() = default;

std::unique_ptr<GfxPattern> GfxShadingPattern::copy() const
{
    auto *pattern = new GfxShadingPattern(shading->copy(), matrix, getPatternRefNum());
    return std::unique_ptr<GfxShadingPattern>(pattern);
}

//------------------------------------------------------------------------
// GfxShading
//------------------------------------------------------------------------

GfxShading::GfxShading(int typeA)
{
    type = static_cast<ShadingType>(typeA);
}

GfxShading::GfxShading(const GfxShading *shading)
{
    int i;

    type = shading->type;
    colorSpace = shading->colorSpace->copy();
    for (i = 0; i < gfxColorMaxComps; ++i) {
        background.c[i] = shading->background.c[i];
    }
    hasBackground = shading->hasBackground;
    bbox_xMin = shading->bbox_xMin;
    bbox_yMin = shading->bbox_yMin;
    bbox_xMax = shading->bbox_xMax;
    bbox_yMax = shading->bbox_yMax;
    hasBBox = shading->hasBBox;
}

GfxShading::~GfxShading() = default;

std::unique_ptr<GfxShading> GfxShading::parse(GfxResources *res, Object *obj, OutputDev *out, GfxState *state)
{
    Dict *dict;
    int typeA;
    Object obj1;

    if (obj->isDict()) {
        dict = obj->getDict();
    } else if (obj->isStream()) {
        dict = obj->streamGetDict();
    } else {
        return {};
    }

    obj1 = dict->lookup("ShadingType");
    if (!obj1.isInt()) {
        error(errSyntaxWarning, -1, "Invalid ShadingType in shading dictionary");
        return {};
    }
    typeA = obj1.getInt();

    switch (typeA) {
    case 1:
        return GfxFunctionShading::parse(res, dict, out, state);
        break;
    case 2:
        return GfxAxialShading::parse(res, dict, out, state);
        break;
    case 3:
        return GfxRadialShading::parse(res, dict, out, state);
        break;
    case 4:
        if (obj->isStream()) {
            return GfxGouraudTriangleShading::parse(res, 4, dict, obj->getStream(), out, state);
        } else {
            error(errSyntaxWarning, -1, "Invalid Type 4 shading object");
        }
        break;
    case 5:
        if (obj->isStream()) {
            return GfxGouraudTriangleShading::parse(res, 5, dict, obj->getStream(), out, state);
        } else {
            error(errSyntaxWarning, -1, "Invalid Type 5 shading object");
        }
        break;
    case 6:
        if (obj->isStream()) {
            return GfxPatchMeshShading::parse(res, 6, dict, obj->getStream(), out, state);
        } else {
            error(errSyntaxWarning, -1, "Invalid Type 6 shading object");
        }
        break;
    case 7:
        if (obj->isStream()) {
            return GfxPatchMeshShading::parse(res, 7, dict, obj->getStream(), out, state);
        } else {
            error(errSyntaxWarning, -1, "Invalid Type 7 shading object");
        }
        break;
    default:
        error(errSyntaxWarning, -1, "Unimplemented shading type {0:d}", typeA);
    }
    return {};
}

bool GfxShading::init(GfxResources *res, Dict *dict, OutputDev *out, GfxState *state)
{
    Object obj1;
    int i;

    obj1 = dict->lookup("ColorSpace");
    if (!(colorSpace = GfxColorSpace::parse(res, &obj1, out, state))) {
        error(errSyntaxWarning, -1, "Bad color space in shading dictionary");
        return false;
    }