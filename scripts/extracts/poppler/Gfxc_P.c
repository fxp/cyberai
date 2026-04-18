// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 16/41



    // construct a (pattern space) -> (current space) transform matrix
    const std::array<double, 6> &ctm = state->getCTM();
    const std::array<double, 6> &btm = baseMatrix;
    const std::array<double, 6> &ptm = tPat->getMatrix();
    // iCTM = invert CTM
    det = ctm[0] * ctm[3] - ctm[1] * ctm[2];
    if (fabs(det) < 0.000001) {
        error(errSyntaxError, getPos(), "Singular matrix in tiling pattern fill");
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

    // construct a (device space) -> (pattern space) transform matrix
    det = m1[0] * m1[3] - m1[1] * m1[2];
    det = 1 / det;
    if (!std::isfinite(det)) {
        error(errSyntaxError, getPos(), "Singular matrix in tiling pattern fill");
        return;
    }
    imb[0] = m1[3] * det;
    imb[1] = -m1[1] * det;
    imb[2] = -m1[2] * det;
    imb[3] = m1[0] * det;
    imb[4] = (m1[2] * m1[5] - m1[3] * m1[4]) * det;
    imb[5] = (m1[1] * m1[4] - m1[0] * m1[5]) * det;

    // save current graphics state
    savedState = saveStateStack();

    // set underlying color space (for uncolored tiling patterns); set
    // various other parameters (stroke color, line width) to match
    // Adobe's behavior
    state->setFillPattern(nullptr);
    state->setStrokePattern(nullptr);
    if (tPat->getPaintType() == 2 && patCS->getUnder()) {
        GfxColorSpace *cs = patCS->getUnder();
        state->setFillColorSpace(cs->copy());
        out->updateFillColorSpace(state);
        state->setStrokeColorSpace(cs->copy());
        out->updateStrokeColorSpace(state);
        if (stroke) {
            state->setFillColor(state->getStrokeColor());
        } else {
            state->setStrokeColor(state->getFillColor());
        }
        out->updateFillColor(state);
        out->updateStrokeColor(state);
    } else {
        state->setFillColorSpace(std::make_unique<GfxDeviceGrayColorSpace>());
        state->getFillColorSpace()->getDefaultColor(&color);
        state->setFillColor(&color);
        out->updateFillColorSpace(state);
        state->setStrokeColorSpace(std::make_unique<GfxDeviceGrayColorSpace>());
        state->setStrokeColor(&color);
        out->updateStrokeColorSpace(state);
    }
    if (!stroke) {
        state->setLineWidth(0);
        out->updateLineWidth(state);
    }

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

    // get the clip region, check for empty
    state->getClipBBox(&cxMin, &cyMin, &cxMax, &cyMax);
    if (cxMin > cxMax || cyMin > cyMax) {
        goto restore;
    }

    // transform clip region bbox to pattern space
    xMin = xMax = cxMin * imb[0] + cyMin * imb[2] + imb[4];
    yMin = yMax = cxMin * imb[1] + cyMin * imb[3] + imb[5];
    x1 = cxMin * imb[0] + cyMax * imb[2] + imb[4];
    y1 = cxMin * imb[1] + cyMax * imb[3] + imb[5];
    if (x1 < xMin) {
        xMin = x1;
    } else if (x1 > xMax) {
        xMax = x1;
    }
    if (y1 < yMin) {
        yMin = y1;
    } else if (y1 > yMax) {
        yMax = y1;
    }
    x1 = cxMax * imb[0] + cyMin * imb[2] + imb[4];
    y1 = cxMax * imb[1] + cyMin * imb[3] + imb[5];
    if (x1 < xMin) {
        xMin = x1;
    } else if (x1 > xMax) {
        xMax = x1;
    }
    if (y1 < yMin) {
        yMin = y1;
    } else if (y1 > yMax) {
        yMax = y1;
    }
    x1 = cxMax * imb[0] + cyMax * imb[2] + imb[4];
    y1 = cxMax * imb[1] + cyMax * imb[3] + imb[5];
    if (x1 < xMin) {
        xMin = x1;
    } else if (x1 > xMax) {
        xMax = x1;
    }
    if (y1 < yMin) {
        yMin = y1;
    } else if (y1 > yMax) {
        yMax = y1;
    }