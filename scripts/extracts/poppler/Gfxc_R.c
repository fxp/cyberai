// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 18/41



    // set the color space
    state->setFillColorSpace(shading->getColorSpace()->copy());
    out->updateFillColorSpace(state);

    // background color fill
    if (shading->getHasBackground()) {
        state->setFillColor(shading->getBackground());
        out->updateFillColor(state);
        state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);
        state->moveTo(xMin, yMin);
        state->lineTo(xMax, yMin);
        state->lineTo(xMax, yMax);
        state->lineTo(xMin, yMax);
        state->closePath();
        out->fill(state);
        state->clearPath();
    }

#if 1 //~tmp: turn off anti-aliasing temporarily
    bool vaa = out->getVectorAntialias();
    if (vaa) {
        out->setVectorAntialias(false);
    }
#endif

    // do shading type-specific operations
    switch (shading->getType()) {
    case GfxShading::FunctionBasedShading:
        doFunctionShFill((GfxFunctionShading *)shading);
        break;
    case GfxShading::AxialShading:
        doAxialShFill((GfxAxialShading *)shading);
        break;
    case GfxShading::RadialShading:
        doRadialShFill((GfxRadialShading *)shading);
        break;
    case GfxShading::FreeFormGouraudShadedTriangleMesh:
    case GfxShading::LatticeFormGouraudShadedTriangleMesh:
        doGouraudTriangleShFill((GfxGouraudTriangleShading *)shading);
        break;
    case GfxShading::CoonsPatchMesh:
    case GfxShading::TensorProductPatchMesh:
        doPatchMeshShFill((GfxPatchMeshShading *)shading);
        break;
    }

#if 1 //~tmp: turn off anti-aliasing temporarily
    if (vaa) {
        out->setVectorAntialias(true);
    }
#endif

    // restore graphics state
    restoreStateStack(savedState);
}

void Gfx::opShFill(Object args[], int /*numArgs*/)
{
    std::unique_ptr<GfxShading> shading;
    GfxState *savedState;
    double xMin, yMin, xMax, yMax;

    if (!ocState) {
        return;
    }

    if (!(shading = res->lookupShading(args[0].getName(), out, state))) {
        return;
    }

    // save current graphics state
    savedState = saveStateStack();

    // clip to bbox
    if (shading->getHasBBox()) {
        shading->getBBox(&xMin, &yMin, &xMax, &yMax);
        state->moveTo(xMin, yMin);
        state->lineTo(xMax, yMin);
        state->lineTo(xMax, yMax);
        state->lineTo(xMin, yMax);
        state->closePath();
        state->clip();
        out->clip(state);
        state->clearPath();
    }

    // set the color space
    state->setFillColorSpace(shading->getColorSpace()->copy());
    out->updateFillColorSpace(state);

#if 1 //~tmp: turn off anti-aliasing temporarily
    bool vaa = out->getVectorAntialias();
    if (vaa) {
        out->setVectorAntialias(false);
    }
#endif

    // do shading type-specific operations
    switch (shading->getType()) {
    case GfxShading::FunctionBasedShading:
        doFunctionShFill((GfxFunctionShading *)shading.get());
        break;
    case GfxShading::AxialShading:
        doAxialShFill((GfxAxialShading *)shading.get());
        break;
    case GfxShading::RadialShading:
        doRadialShFill((GfxRadialShading *)shading.get());
        break;
    case GfxShading::FreeFormGouraudShadedTriangleMesh:
    case GfxShading::LatticeFormGouraudShadedTriangleMesh:
        doGouraudTriangleShFill((GfxGouraudTriangleShading *)shading.get());
        break;
    case GfxShading::CoonsPatchMesh:
    case GfxShading::TensorProductPatchMesh:
        doPatchMeshShFill((GfxPatchMeshShading *)shading.get());
        break;
    }

#if 1 //~tmp: turn off anti-aliasing temporarily
    if (vaa) {
        out->setVectorAntialias(true);
    }
#endif

    // restore graphics state
    restoreStateStack(savedState);
}

void Gfx::doFunctionShFill(GfxFunctionShading *shading)
{
    double x0, y0, x1, y1;
    GfxColor colors[4];

    if (out->useShadedFills(shading->getType()) && out->functionShadedFill(state, shading)) {
        return;
    }

    shading->getDomain(&x0, &y0, &x1, &y1);
    shading->getColor(x0, y0, &colors[0]);
    shading->getColor(x0, y1, &colors[1]);
    shading->getColor(x1, y0, &colors[2]);
    shading->getColor(x1, y1, &colors[3]);
    doFunctionShFill1(shading, x0, y0, x1, y1, colors, 0);
}

void Gfx::doFunctionShFill1(GfxFunctionShading *shading, double x0, double y0, double x1, double y1, GfxColor *colors, int depth)
{
    GfxColor fillColor;
    GfxColor color0M, color1M, colorM0, colorM1, colorMM;
    GfxColor colors2[4];
    double xM, yM;
    int nComps, i, j;

    nComps = shading->getColorSpace()->getNComps();
    const std::array<double, 6> &matrix = shading->getMatrix();

    // compare the four corner colors
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < nComps; ++j) {
            if (abs(colors[i].c[j] - colors[(i + 1) & 3].c[j]) > functionColorDelta) {
                break;
            }
        }
        if (j < nComps) {
            break;
        }
    }

    // center of the rectangle
    xM = 0.5 * (x0 + x1);
    yM = 0.5 * (y0 + y1);