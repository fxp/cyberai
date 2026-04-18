// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 32/49



void GfxShadingBitBuf::flushBits()
{
    bitBuf = 0;
    nBits = 0;
}

//------------------------------------------------------------------------
// GfxGouraudTriangleShading
//------------------------------------------------------------------------

GfxGouraudTriangleShading::GfxGouraudTriangleShading(int typeA, GfxGouraudVertex *verticesA, int nVerticesA, int (*trianglesA)[3], int nTrianglesA, std::vector<std::unique_ptr<Function>> &&funcsA)
    : GfxShading(typeA), funcs(std::move(funcsA))
{
    vertices = verticesA;
    nVertices = nVerticesA;
    triangles = trianglesA;
    nTriangles = nTrianglesA;
}

GfxGouraudTriangleShading::GfxGouraudTriangleShading(const GfxGouraudTriangleShading *shading) : GfxShading(shading)
{
    nVertices = shading->nVertices;
    vertices = (GfxGouraudVertex *)gmallocn(nVertices, sizeof(GfxGouraudVertex));
    memcpy(vertices, shading->vertices, nVertices * sizeof(GfxGouraudVertex));
    nTriangles = shading->nTriangles;
    triangles = (int (*)[3])gmallocn(nTriangles * 3, sizeof(int));
    memcpy(triangles, shading->triangles, nTriangles * 3 * sizeof(int));
    for (const auto &f : shading->funcs) {
        funcs.emplace_back(f->copy());
    }
}

GfxGouraudTriangleShading::~GfxGouraudTriangleShading()
{
    gfree(vertices);
    gfree(triangles);
}

std::unique_ptr<GfxGouraudTriangleShading> GfxGouraudTriangleShading::parse(GfxResources *res, int typeA, Dict *dict, Stream *str, OutputDev *out, GfxState *gfxState)
{
    std::vector<std::unique_ptr<Function>> funcsA;
    int coordBits, compBits, flagBits, vertsPerRow, nRows;
    double xMin, xMax, yMin, yMax;
    double cMin[gfxColorMaxComps], cMax[gfxColorMaxComps];
    double xMul, yMul;
    double cMul[gfxColorMaxComps];
    GfxGouraudVertex *verticesA;
    int (*trianglesA)[3];
    int nComps, nVerticesA, nTrianglesA, vertSize, triSize;
    unsigned int x, y, flag;
    unsigned int c[gfxColorMaxComps];
    GfxShadingBitBuf *bitBuf;
    Object obj1;
    int i, j, k, state;

    obj1 = dict->lookup("BitsPerCoordinate");
    if (obj1.isInt()) {
        coordBits = obj1.getInt();
    } else {
        error(errSyntaxWarning, -1, "Missing or invalid BitsPerCoordinate in shading dictionary");
        return {};
    }
    if (unlikely(coordBits <= 0)) {
        error(errSyntaxWarning, -1, "Invalid BitsPerCoordinate in shading dictionary");
        return {};
    }
    obj1 = dict->lookup("BitsPerComponent");
    if (obj1.isInt()) {
        compBits = obj1.getInt();
    } else {
        error(errSyntaxWarning, -1, "Missing or invalid BitsPerComponent in shading dictionary");
        return {};
    }
    if (unlikely(compBits <= 0 || compBits > 31)) {
        error(errSyntaxWarning, -1, "Invalid BitsPerComponent in shading dictionary");
        return {};
    }
    flagBits = vertsPerRow = 0; // make gcc happy
    if (typeA == 4) {
        obj1 = dict->lookup("BitsPerFlag");
        if (obj1.isInt()) {
            flagBits = obj1.getInt();
        } else {
            error(errSyntaxWarning, -1, "Missing or invalid BitsPerFlag in shading dictionary");
            return {};
        }
    } else {
        obj1 = dict->lookup("VerticesPerRow");
        if (obj1.isInt()) {
            vertsPerRow = obj1.getInt();
        } else {
            error(errSyntaxWarning, -1, "Missing or invalid VerticesPerRow in shading dictionary");
            return {};
        }
    }
    obj1 = dict->lookup("Decode");
    if (obj1.isArrayOfLengthAtLeast(6)) {
        bool decodeOk = true;
        xMin = obj1.arrayGet(0).getNum(&decodeOk);
        xMax = obj1.arrayGet(1).getNum(&decodeOk);
        xMul = (xMax - xMin) / (pow(2.0, coordBits) - 1);
        yMin = obj1.arrayGet(2).getNum(&decodeOk);
        yMax = obj1.arrayGet(3).getNum(&decodeOk);
        yMul = (yMax - yMin) / (pow(2.0, coordBits) - 1);
        for (i = 0; 5 + 2 * i < obj1.arrayGetLength() && i < gfxColorMaxComps; ++i) {
            cMin[i] = obj1.arrayGet(4 + 2 * i).getNum(&decodeOk);
            cMax[i] = obj1.arrayGet(5 + 2 * i).getNum(&decodeOk);
            cMul[i] = (cMax[i] - cMin[i]) / (double)((1U << compBits) - 1);
        }
        nComps = i;

        if (!decodeOk) {
            error(errSyntaxWarning, -1, "Missing or invalid Decode array in shading dictionary");
            return {};
        }
    } else {
        error(errSyntaxWarning, -1, "Missing or invalid Decode array in shading dictionary");
        return {};
    }