// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 15/41



void Gfx::opEOFillStroke(Object /*args*/[], int /*numArgs*/)
{
    if (!state->isCurPt()) {
        // error(errSyntaxError, getPos(), "No path in eofill/stroke");
        return;
    }
    if (state->isPath()) {
        if (ocState) {
            if (state->getFillColorSpace()->getMode() == csPattern) {
                doPatternFill(true);
            } else {
                out->eoFill(state);
            }
            if (state->getStrokeColorSpace()->getMode() == csPattern) {
                doPatternStroke();
            } else {
                out->stroke(state);
            }
        }
    }
    doEndPath();
}

void Gfx::opCloseEOFillStroke(Object /*args*/[], int /*numArgs*/)
{
    if (!state->isCurPt()) {
        // error(errSyntaxError, getPos(), "No path in closepath/eofill/stroke");
        return;
    }
    if (state->isPath()) {
        state->closePath();
        if (ocState) {
            if (state->getFillColorSpace()->getMode() == csPattern) {
                doPatternFill(true);
            } else {
                out->eoFill(state);
            }
            if (state->getStrokeColorSpace()->getMode() == csPattern) {
                doPatternStroke();
            } else {
                out->stroke(state);
            }
        }
    }
    doEndPath();
}

void Gfx::doPatternFill(bool eoFill)
{
    GfxPattern *pattern;

    // this is a bit of a kludge -- patterns can be really slow, so we
    // skip them if we're only doing text extraction, since they almost
    // certainly don't contain any text
    if (!out->needNonText()) {
        return;
    }

    if (!(pattern = state->getFillPattern())) {
        return;
    }
    switch (pattern->getType()) {
    case 1:
        doTilingPatternFill((GfxTilingPattern *)pattern, false, eoFill, false);
        break;
    case 2:
        doShadingPatternFill((GfxShadingPattern *)pattern, false, eoFill, false);
        break;
    default:
        error(errSyntaxError, getPos(), "Unknown pattern type ({0:d}) in fill", pattern->getType());
        break;
    }
}

void Gfx::doPatternStroke()
{
    GfxPattern *pattern;

    // this is a bit of a kludge -- patterns can be really slow, so we
    // skip them if we're only doing text extraction, since they almost
    // certainly don't contain any text
    if (!out->needNonText()) {
        return;
    }

    if (!(pattern = state->getStrokePattern())) {
        return;
    }
    switch (pattern->getType()) {
    case 1:
        doTilingPatternFill((GfxTilingPattern *)pattern, true, false, false);
        break;
    case 2:
        doShadingPatternFill((GfxShadingPattern *)pattern, true, false, false);
        break;
    default:
        error(errSyntaxError, getPos(), "Unknown pattern type ({0:d}) in stroke", pattern->getType());
        break;
    }
}

void Gfx::doPatternText()
{
    GfxPattern *pattern;

    // this is a bit of a kludge -- patterns can be really slow, so we
    // skip them if we're only doing text extraction, since they almost
    // certainly don't contain any text
    if (!out->needNonText()) {
        return;
    }

    if (!(pattern = state->getFillPattern())) {
        return;
    }
    switch (pattern->getType()) {
    case 1:
        doTilingPatternFill((GfxTilingPattern *)pattern, false, false, true);
        break;
    case 2:
        doShadingPatternFill((GfxShadingPattern *)pattern, false, false, true);
        break;
    default:
        error(errSyntaxError, getPos(), "Unknown pattern type ({0:d}) in fill", pattern->getType());
        break;
    }
}

void Gfx::doPatternImageMask(Object *ref, Stream *str, int width, int height, bool invert, bool inlineImg)
{
    saveState();

    out->setSoftMaskFromImageMask(state, ref, str, width, height, invert, inlineImg, baseMatrix);

    state->clearPath();
    state->moveTo(0, 0);
    state->lineTo(1, 0);
    state->lineTo(1, 1);
    state->lineTo(0, 1);
    state->closePath();
    doPatternText();

    out->unsetSoftMaskFromImageMask(state, baseMatrix);
    restoreState();
}

void Gfx::doTilingPatternFill(GfxTilingPattern *tPat, bool stroke, bool eoFill, bool text)
{
    GfxPatternColorSpace *patCS;
    GfxColor color;
    GfxState *savedState;
    double xMin, yMin, xMax, yMax, x, y, x1, y1;
    double cxMin, cyMin, cxMax, cyMax;
    int xi0, yi0, xi1, yi1, xi, yi;
    double m[6], ictm[6], imb[6];
    std::array<double, 6> m1;
    double det;
    double xstep, ystep;
    int i;

    // get color space
    patCS = (GfxPatternColorSpace *)(stroke ? state->getStrokeColorSpace() : state->getFillColorSpace());