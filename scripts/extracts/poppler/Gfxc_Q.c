// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 17/41



    // draw the pattern
    //~ this should treat negative steps differently -- start at right/top
    //~ edge instead of left/bottom (?)
    xstep = fabs(tPat->getXStep());
    ystep = fabs(tPat->getYStep());
    if (unlikely(xstep == 0 || ystep == 0)) {
        goto restore;
    }
    if (tPat->getBBox()[0] < tPat->getBBox()[2]) {
        xi0 = (int)ceil((xMin - tPat->getBBox()[2]) / xstep);
        xi1 = (int)floor((xMax - tPat->getBBox()[0]) / xstep) + 1;
    } else {
        xi0 = (int)ceil((xMin - tPat->getBBox()[0]) / xstep);
        xi1 = (int)floor((xMax - tPat->getBBox()[2]) / xstep) + 1;
    }
    if (tPat->getBBox()[1] < tPat->getBBox()[3]) {
        yi0 = (int)ceil((yMin - tPat->getBBox()[3]) / ystep);
        yi1 = (int)floor((yMax - tPat->getBBox()[1]) / ystep) + 1;
    } else {
        yi0 = (int)ceil((yMin - tPat->getBBox()[1]) / ystep);
        yi1 = (int)floor((yMax - tPat->getBBox()[3]) / ystep) + 1;
    }
    for (i = 0; i < 4; ++i) {
        m1[i] = m[i];
    }
    m1[4] = m[4];
    m1[5] = m[5];
    {
        bool shouldDrawPattern = true;
        std::set<int>::iterator patternRefIt;
        const int patternRefNum = tPat->getPatternRefNum();
        if (patternRefNum != -1) {
            bool inserted;
            std::tie(patternRefIt, inserted) = formsDrawing.insert(patternRefNum);
            if (!inserted) {
                shouldDrawPattern = false;
            }
        }
        if (shouldDrawPattern) {
            if (out->useTilingPatternFill() && out->tilingPatternFill(state, this, catalog, tPat, m1, xi0, yi0, xi1, yi1, xstep, ystep)) {
                // do nothing
            } else {
                out->updatePatternOpacity(state);
                for (yi = yi0; yi < yi1; ++yi) {
                    for (xi = xi0; xi < xi1; ++xi) {
                        x = xi * xstep;
                        y = yi * ystep;
                        m1[4] = x * m[0] + y * m[2] + m[4];
                        m1[5] = x * m[1] + y * m[3] + m[5];
                        drawForm(tPat->getContentStream(), tPat->getResDict(), m1, tPat->getBBox());
                    }
                }
                out->clearPatternOpacity(state);
            }
            if (patternRefNum != -1) {
                formsDrawing.erase(patternRefIt);
            }
        }
    }

    // restore graphics state
restore:
    restoreStateStack(savedState);
}

void Gfx::doShadingPatternFill(GfxShadingPattern *sPat, bool stroke, bool eoFill, bool text)
{
    GfxShading *shading;
    GfxState *savedState;
    double m[6], ictm[6], m1[6];
    double xMin, yMin, xMax, yMax;
    double det;

    shading = sPat->getShading();

    // save current graphics state
    savedState = saveStateStack();

    // clip to current path
    if (stroke) {
        state->clipToStrokePath();
        out->clipToStrokePath(state);
    } else if (!text) {
        state->clip();
        if (eoFill) {
            out->eoClip(state);
        } else {
            out->clip(state);
        }
    }
    state->clearPath();

    // construct a (pattern space) -> (current space) transform matrix
    const std::array<double, 6> &ctm = state->getCTM();
    const std::array<double, 6> &btm = baseMatrix;
    const std::array<double, 6> &ptm = sPat->getMatrix();
    // iCTM = invert CTM
    det = ctm[0] * ctm[3] - ctm[1] * ctm[2];
    if (fabs(det) < 0.000001) {
        error(errSyntaxError, getPos(), "Singular matrix in shading pattern fill");
        restoreStateStack(savedState);
        return;
    }
    det = 1 / det;
    ictm[0] = ctm[3] * det;
    ictm[1] = -ctm[1] * det;
    ictm[2] = -ctm[2] * det;
    ictm[3] = ctm[0] * det;
    ictm[4] = (ctm[2] * ctm[5] - ctm[3] * ctm[4]) * det;
    ictm[5] = (ctm[1] * ctm[4] - ctm[0] * ctm[5]) * det;
    // m1 = PTM * BTM = PTM * base transform matrix
    m1[0] = ptm[0] * btm[0] + ptm[1] * btm[2];
    m1[1] = ptm[0] * btm[1] + ptm[1] * btm[3];
    m1[2] = ptm[2] * btm[0] + ptm[3] * btm[2];
    m1[3] = ptm[2] * btm[1] + ptm[3] * btm[3];
    m1[4] = ptm[4] * btm[0] + ptm[5] * btm[2] + btm[4];
    m1[5] = ptm[4] * btm[1] + ptm[5] * btm[3] + btm[5];
    // m = m1 * iCTM = (PTM * BTM) * (iCTM)
    m[0] = m1[0] * ictm[0] + m1[1] * ictm[2];
    m[1] = m1[0] * ictm[1] + m1[1] * ictm[3];
    m[2] = m1[2] * ictm[0] + m1[3] * ictm[2];
    m[3] = m1[2] * ictm[1] + m1[3] * ictm[3];
    m[4] = m1[4] * ictm[0] + m1[5] * ictm[2] + ictm[4];
    m[5] = m1[4] * ictm[1] + m1[5] * ictm[3] + ictm[5];

    // set the new matrix
    state->concatCTM(m[0], m[1], m[2], m[3], m[4], m[5]);
    out->updateCTM(state, m[0], m[1], m[2], m[3], m[4], m[5]);

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