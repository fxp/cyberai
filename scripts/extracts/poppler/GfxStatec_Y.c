// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 25/49



    for (i = 0; i < gfxColorMaxComps; ++i) {
        background.c[i] = 0;
    }
    hasBackground = false;
    obj1 = dict->lookup("Background");
    if (obj1.isArray()) {
        if (obj1.arrayGetLength() == colorSpace->getNComps()) {
            hasBackground = true;
            for (i = 0; i < colorSpace->getNComps(); ++i) {
                Object obj2 = obj1.arrayGet(i);
                background.c[i] = dblToCol(obj2.getNum(&hasBackground));
            }
            if (!hasBackground) {
                error(errSyntaxWarning, -1, "Bad Background in shading dictionary");
            }
        } else {
            error(errSyntaxWarning, -1, "Bad Background in shading dictionary");
        }
    }

    bbox_xMin = bbox_yMin = bbox_xMax = bbox_yMax = 0;
    hasBBox = false;
    obj1 = dict->lookup("BBox");
    if (obj1.isArray()) {
        if (obj1.arrayGetLength() == 4) {
            hasBBox = true;
            bbox_xMin = obj1.arrayGet(0).getNum(&hasBBox);
            bbox_yMin = obj1.arrayGet(1).getNum(&hasBBox);
            bbox_xMax = obj1.arrayGet(2).getNum(&hasBBox);
            bbox_yMax = obj1.arrayGet(3).getNum(&hasBBox);
            if (!hasBBox) {
                error(errSyntaxWarning, -1, "Bad BBox in shading dictionary (Values not numbers)");
            }
        } else {
            error(errSyntaxWarning, -1, "Bad BBox in shading dictionary");
        }
    }

    return true;
}

//------------------------------------------------------------------------
// GfxFunctionShading
//------------------------------------------------------------------------

GfxFunctionShading::GfxFunctionShading(double x0A, double y0A, double x1A, double y1A, const std::array<double, 6> &matrixA, std::vector<std::unique_ptr<Function>> &&funcsA) : GfxShading(1), matrix(matrixA), funcs(std::move(funcsA))
{
    x0 = x0A;
    y0 = y0A;
    x1 = x1A;
    y1 = y1A;
}

GfxFunctionShading::GfxFunctionShading(const GfxFunctionShading *shading) : GfxShading(shading), matrix(shading->matrix)
{
    x0 = shading->x0;
    y0 = shading->y0;
    x1 = shading->x1;
    y1 = shading->y1;
    for (const auto &f : shading->funcs) {
        funcs.emplace_back(f->copy());
    }
}

GfxFunctionShading::~GfxFunctionShading() = default;

std::unique_ptr<GfxFunctionShading> GfxFunctionShading::parse(GfxResources *res, Dict *dict, OutputDev *out, GfxState *state)
{
    double x0A, y0A, x1A, y1A;
    std::vector<std::unique_ptr<Function>> funcsA;
    Object obj1;
    int i;

    x0A = y0A = 0;
    x1A = y1A = 1;
    obj1 = dict->lookup("Domain");
    if (obj1.isArrayOfLength(4)) {
        bool decodeOk = true;
        x0A = obj1.arrayGet(0).getNum(&decodeOk);
        x1A = obj1.arrayGet(1).getNum(&decodeOk);
        y0A = obj1.arrayGet(2).getNum(&decodeOk);
        y1A = obj1.arrayGet(3).getNum(&decodeOk);

        if (!decodeOk) {
            error(errSyntaxWarning, -1, "Invalid Domain array in function shading dictionary");
            return {};
        }
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
        bool decodeOk = true;
        matrixA[0] = obj1.arrayGet(0).getNum(&decodeOk);
        matrixA[1] = obj1.arrayGet(1).getNum(&decodeOk);
        matrixA[2] = obj1.arrayGet(2).getNum(&decodeOk);
        matrixA[3] = obj1.arrayGet(3).getNum(&decodeOk);
        matrixA[4] = obj1.arrayGet(4).getNum(&decodeOk);
        matrixA[5] = obj1.arrayGet(5).getNum(&decodeOk);

        if (!decodeOk) {
            error(errSyntaxWarning, -1, "Invalid Matrix array in function shading dictionary");
            return {};
        }
    }

    obj1 = dict->lookup("Function");
    if (obj1.isArray()) {
        const int nFuncsA = obj1.arrayGetLength();
        if (nFuncsA > gfxColorMaxComps || nFuncsA <= 0) {
            error(errSyntaxWarning, -1, "Invalid Function array in shading dictionary");
            return {};
        }
        for (i = 0; i < nFuncsA; ++i) {
            Object obj2 = obj1.arrayGet(i);
            std::unique_ptr<Function> f = Function::parse(&obj2);
            if (!f) {
                return {};
            }
            funcsA.emplace_back(std::move(f));
        }
    } else {
        std::unique_ptr<Function> f = Function::parse(&obj1);
        if (!f) {
            return {};
        }
        funcsA.emplace_back(std::move(f));
    }

    auto shading = std::make_unique<GfxFunctionShading>(x0A, y0A, x1A, y1A, matrixA, std::move(funcsA));
    if (!shading->init(res, dict, out, state)) {
        return {};
    }
    return shading;
}

bool GfxFunctionShading::init(GfxResources *res, Dict *dict, OutputDev *out, GfxState *state)
{
    const bool parentInit = GfxShading::init(res, dict, out, state);
    if (!parentInit) {
        return false;
    }