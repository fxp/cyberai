// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 34/49



bool GfxGouraudTriangleShading::init(GfxResources *res, Dict *dict, OutputDev *out, GfxState *state)
{
    const bool parentInit = GfxShading::init(res, dict, out, state);
    if (!parentInit) {
        return false;
    }

    // funcs needs to be one of the three:
    //  * One function 1-in -> nComps-out
    //  * nComps functions 1-in -> 1-out
    //  * empty
    const int nComps = colorSpace->getNComps();
    const int nFuncs = funcs.size();
    if (nFuncs == 1) {
        if (funcs[0]->getInputSize() != 1) {
            error(errSyntaxWarning, -1, "GfxGouraudTriangleShading: function with input size != 2");
            return false;
        }
        if (funcs[0]->getOutputSize() != nComps) {
            error(errSyntaxWarning, -1, "GfxGouraudTriangleShading: function with wrong output size");
            return false;
        }
    } else if (nFuncs == nComps) {
        for (const std::unique_ptr<Function> &f : funcs) {
            if (f->getInputSize() != 1) {
                error(errSyntaxWarning, -1, "GfxGouraudTriangleShading: function with input size != 2");
                return false;
            }
            if (f->getOutputSize() != 1) {
                error(errSyntaxWarning, -1, "GfxGouraudTriangleShading: function with wrong output size");
                return false;
            }
        }
    } else if (nFuncs != 0) {
        return false;
    }

    return true;
}

std::unique_ptr<GfxShading> GfxGouraudTriangleShading::copy() const
{
    return std::make_unique<GfxGouraudTriangleShading>(this);
}

void GfxGouraudTriangleShading::getTriangle(int i, double *x0, double *y0, GfxColor *color0, double *x1, double *y1, GfxColor *color1, double *x2, double *y2, GfxColor *color2) const
{
    int v;

    assert(!isParameterized());

    v = triangles[i][0];
    *x0 = vertices[v].x;
    *y0 = vertices[v].y;
    *color0 = vertices[v].color;
    v = triangles[i][1];
    *x1 = vertices[v].x;
    *y1 = vertices[v].y;
    *color1 = vertices[v].color;
    v = triangles[i][2];
    *x2 = vertices[v].x;
    *y2 = vertices[v].y;
    *color2 = vertices[v].color;
}

void GfxGouraudTriangleShading::getParameterizedColor(double t, GfxColor *color) const
{
    double out[gfxColorMaxComps];

    for (unsigned int j = 0; j < funcs.size(); ++j) {
        funcs[j]->transform(&t, &out[j]);
    }
    for (int j = 0; j < gfxColorMaxComps; ++j) {
        color->c[j] = dblToCol(out[j]);
    }
}

void GfxGouraudTriangleShading::getTriangle(int i, double *x0, double *y0, double *color0, double *x1, double *y1, double *color1, double *x2, double *y2, double *color2) const
{
    int v;

    assert(isParameterized());

    v = triangles[i][0];
    if (likely(v >= 0 && v < nVertices)) {
        *x0 = vertices[v].x;
        *y0 = vertices[v].y;
        *color0 = colToDbl(vertices[v].color.c[0]);
    }
    v = triangles[i][1];
    if (likely(v >= 0 && v < nVertices)) {
        *x1 = vertices[v].x;
        *y1 = vertices[v].y;
        *color1 = colToDbl(vertices[v].color.c[0]);
    }
    v = triangles[i][2];
    if (likely(v >= 0 && v < nVertices)) {
        *x2 = vertices[v].x;
        *y2 = vertices[v].y;
        *color2 = colToDbl(vertices[v].color.c[0]);
    }
}

//------------------------------------------------------------------------
// GfxPatchMeshShading
//------------------------------------------------------------------------

GfxPatchMeshShading::GfxPatchMeshShading(int typeA, GfxPatch *patchesA, int nPatchesA, std::vector<std::unique_ptr<Function>> &&funcsA) : GfxShading(typeA), funcs(std::move(funcsA))
{
    patches = patchesA;
    nPatches = nPatchesA;
}

GfxPatchMeshShading::GfxPatchMeshShading(const GfxPatchMeshShading *shading) : GfxShading(shading)
{
    nPatches = shading->nPatches;
    patches = (GfxPatch *)gmallocn(nPatches, sizeof(GfxPatch));
    memcpy(patches, shading->patches, nPatches * sizeof(GfxPatch));
    for (const auto &f : shading->funcs) {
        funcs.emplace_back(f->copy());
    }
}

GfxPatchMeshShading::~GfxPatchMeshShading()
{
    gfree(patches);
}

std::unique_ptr<GfxPatchMeshShading> GfxPatchMeshShading::parse(GfxResources *res, int typeA, Dict *dict, Stream *str, OutputDev *out, GfxState *state)
{
    std::vector<std::unique_ptr<Function>> funcsA;
    int coordBits, compBits, flagBits;
    double xMin, xMax, yMin, yMax;
    double cMin[gfxColorMaxComps], cMax[gfxColorMaxComps];
    double xMul, yMul;
    double cMul[gfxColorMaxComps];
    GfxPatch *patchesA, *p;
    int nComps, nPatchesA, patchesSize, nPts, nColors;
    unsigned int flag;
    double x[16], y[16];
    unsigned int xi, yi;
    double c[4][gfxColorMaxComps];
    unsigned int ci;
    Object obj1;
    int i, j;