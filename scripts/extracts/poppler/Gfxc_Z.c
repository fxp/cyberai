// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 26/41



    } else {
        x01 = 0.5 * (x0 + x1);
        y01 = 0.5 * (y0 + y1);
        x12 = 0.5 * (x1 + x2);
        y12 = 0.5 * (y1 + y2);
        x20 = 0.5 * (x2 + x0);
        y20 = 0.5 * (y2 + y0);
        for (i = 0; i < nComps; ++i) {
            color01.c[i] = safeAverage(color0->c[i], color1->c[i]);
            color12.c[i] = safeAverage(color1->c[i], color2->c[i]);
            color20.c[i] = safeAverage(color2->c[i], color0->c[i]);
        }
        gouraudFillTriangle(x0, y0, color0, x01, y01, &color01, x20, y20, &color20, nComps, depth + 1, path);
        gouraudFillTriangle(x01, y01, &color01, x1, y1, color1, x12, y12, &color12, nComps, depth + 1, path);
        gouraudFillTriangle(x01, y01, &color01, x12, y12, &color12, x20, y20, &color20, nComps, depth + 1, path);
        gouraudFillTriangle(x20, y20, &color20, x12, y12, &color12, x2, y2, color2, nComps, depth + 1, path);
    }
}
void Gfx::gouraudFillTriangle(double x0, double y0, double color0, double x1, double y1, double color1, double x2, double y2, double color2, double refineColorThreshold, int depth, GfxGouraudTriangleShading *shading,
                              GfxState::ReusablePathIterator *path)
{
    const double meanColor = (color0 + color1 + color2) / 3;

    const bool isFineEnough = fabs(color0 - meanColor) < refineColorThreshold && fabs(color1 - meanColor) < refineColorThreshold && fabs(color2 - meanColor) < refineColorThreshold;

    if (isFineEnough || depth == gouraudMaxDepth) {
        GfxColor color;

        shading->getParameterizedColor(meanColor, &color);
        state->setFillColor(&color);
        out->updateFillColor(state);

        path->reset();
        checkTrue(!path->isEnd(), "Path should not be at end");
        path->setCoord(x0, y0);
        path->next();
        checkTrue(!path->isEnd(), "Path should not be at end");
        path->setCoord(x1, y1);
        path->next();
        checkTrue(!path->isEnd(), "Path should not be at end");
        path->setCoord(x2, y2);
        path->next();
        checkTrue(!path->isEnd(), "Path should not be at end");
        path->setCoord(x0, y0);
        path->next();
        checkTrue(path->isEnd(), "Path should be at end");
        out->fill(state);

    } else {
        const double x01 = 0.5 * (x0 + x1);
        const double y01 = 0.5 * (y0 + y1);
        const double x12 = 0.5 * (x1 + x2);
        const double y12 = 0.5 * (y1 + y2);
        const double x20 = 0.5 * (x2 + x0);
        const double y20 = 0.5 * (y2 + y0);
        const double color01 = (color0 + color1) / 2.;
        const double color12 = (color1 + color2) / 2.;
        const double color20 = (color2 + color0) / 2.;
        ++depth;
        gouraudFillTriangle(x0, y0, color0, x01, y01, color01, x20, y20, color20, refineColorThreshold, depth, shading, path);
        gouraudFillTriangle(x01, y01, color01, x1, y1, color1, x12, y12, color12, refineColorThreshold, depth, shading, path);
        gouraudFillTriangle(x01, y01, color01, x12, y12, color12, x20, y20, color20, refineColorThreshold, depth, shading, path);
        gouraudFillTriangle(x20, y20, color20, x12, y12, color12, x2, y2, color2, refineColorThreshold, depth, shading, path);
    }
}

void Gfx::doPatchMeshShFill(GfxPatchMeshShading *shading)
{
    int start, i;

    if (out->useShadedFills(shading->getType())) {
        if (out->patchMeshShadedFill(state, shading)) {
            return;
        }
    }

    if (shading->getNPatches() > 128) {
        start = 3;
    } else if (shading->getNPatches() > 64) {
        start = 2;
    } else if (shading->getNPatches() > 16) {
        start = 1;
    } else {
        start = 0;
    }
    /*
     * Parameterized shadings take one parameter [t_0,t_e]
     * and map it into the color space.
     *
     * Consequently, all color values are stored as doubles.
     *
     * These color values are interpreted as parameters for parameterized
     * shadings and as colorspace entities otherwise.
     *
     * The only difference is that color space entities are stored into
     * DOUBLE arrays, not into arrays of type GfxColorComp.
     */
    const int colorComps = shading->getColorSpace()->getNComps();
    double refineColorThreshold;
    if (shading->isParameterized()) {
        refineColorThreshold = gouraudParameterizedColorDelta * (shading->getParameterDomainMax() - shading->getParameterDomainMin());

    } else {
        refineColorThreshold = patchColorDelta;
    }

    for (i = 0; i < shading->getNPatches(); ++i) {
        fillPatch(shading->getPatch(i), colorComps, shading->isParameterized() ? 1 : colorComps, refineColorThreshold, start, shading);
    }
}

void Gfx::fillPatch(const GfxPatch *patch, int colorComps, int patchColorComps, double refineColorThreshold, int depth, const GfxPatchMeshShading *shading)
{
    GfxPatch patch00, patch01, patch10, patch11;
    double xx[4][8], yy[4][8];
    double xxm, yym;
    int i;