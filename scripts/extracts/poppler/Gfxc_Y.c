// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 25/41



    // preallocate a path (speed improvements)
    state->moveTo(0., 0.);
    state->lineTo(1., 0.);
    state->lineTo(0., 1.);
    state->closePath();

    const std::unique_ptr<GfxState::ReusablePathIterator> reusablePath = state->getReusablePath();

    if (shading->isParameterized()) {
        // work with parameterized values:
        double color0, color1, color2;
        // a relative threshold:
        const double refineColorThreshold = gouraudParameterizedColorDelta * (shading->getParameterDomainMax() - shading->getParameterDomainMin());
        for (i = 0; i < shading->getNTriangles(); ++i) {
            shading->getTriangle(i, &x0, &y0, &color0, &x1, &y1, &color1, &x2, &y2, &color2);
            gouraudFillTriangle(x0, y0, color0, x1, y1, color1, x2, y2, color2, refineColorThreshold, 0, shading, reusablePath.get());
        }

    } else {
        // this always produces output -- even for parameterized ranges.
        // But it ignores the parameterized color map (the function).
        //
        // Note that using this code in for parameterized shadings might be
        // correct in circumstances (namely if the function is linear in the actual
        // triangle), but in general, it will simply be wrong.
        GfxColor color0, color1, color2;
        for (i = 0; i < shading->getNTriangles(); ++i) {
            shading->getTriangle(i, &x0, &y0, &color0, &x1, &y1, &color1, &x2, &y2, &color2);
            gouraudFillTriangle(x0, y0, &color0, x1, y1, &color1, x2, y2, &color2, shading->getColorSpace()->getNComps(), 0, reusablePath.get());
        }
    }
}

#define checkTrue(b, message)                                                                                                                                                                                                                  \
    if (unlikely(!(b))) {                                                                                                                                                                                                                      \
        error(errSyntaxError, -1, message);                                                                                                                                                                                                    \
    }

void Gfx::gouraudFillTriangle(double x0, double y0, GfxColor *color0, double x1, double y1, GfxColor *color1, double x2, double y2, GfxColor *color2, int nComps, int depth, GfxState::ReusablePathIterator *path)
{
    double x01, y01, x12, y12, x20, y20;
    GfxColor color01, color12, color20;
    int i;

    for (i = 0; i < nComps; ++i) {
        int color01Delta, color12Delta;
        if (checkedSubtraction(color0->c[i], color1->c[i], &color01Delta)) {
            break;
        }
        if (checkedSubtraction(color1->c[i], color2->c[i], &color12Delta)) {
            break;
        }
        if (abs(color01Delta) > gouraudColorDelta || abs(color12Delta) > gouraudColorDelta) {
            break;
        }
    }
    if (i == nComps || depth == gouraudMaxDepth) {
        state->setFillColor(color0);
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