// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 19/41



    // the four corner colors are close (or we hit the recursive limit)
    // -- fill the rectangle; but require at least one subdivision
    // (depth==0) to avoid problems when the four outer corners of the
    // shaded region are the same color
    if ((i == 4 && depth > 0) || depth == functionMaxDepth) {

        // use the center color
        shading->getColor(xM, yM, &fillColor);
        state->setFillColor(&fillColor);
        out->updateFillColor(state);

        // fill the rectangle
        state->moveTo(x0 * matrix[0] + y0 * matrix[2] + matrix[4], x0 * matrix[1] + y0 * matrix[3] + matrix[5]);
        state->lineTo(x1 * matrix[0] + y0 * matrix[2] + matrix[4], x1 * matrix[1] + y0 * matrix[3] + matrix[5]);
        state->lineTo(x1 * matrix[0] + y1 * matrix[2] + matrix[4], x1 * matrix[1] + y1 * matrix[3] + matrix[5]);
        state->lineTo(x0 * matrix[0] + y1 * matrix[2] + matrix[4], x0 * matrix[1] + y1 * matrix[3] + matrix[5]);
        state->closePath();
        out->fill(state);
        state->clearPath();

        // the four corner colors are not close enough -- subdivide the
        // rectangle
    } else {

        // colors[0]       colorM0       colors[2]
        //   (x0,y0)       (xM,y0)       (x1,y0)
        //         +----------+----------+
        //         |          |          |
        //         |    UL    |    UR    |
        // color0M |       colorMM       | color1M
        // (x0,yM) +----------+----------+ (x1,yM)
        //         |       (xM,yM)       |
        //         |    LL    |    LR    |
        //         |          |          |
        //         +----------+----------+
        // colors[1]       colorM1       colors[3]
        //   (x0,y1)       (xM,y1)       (x1,y1)

        shading->getColor(x0, yM, &color0M);
        shading->getColor(x1, yM, &color1M);
        shading->getColor(xM, y0, &colorM0);
        shading->getColor(xM, y1, &colorM1);
        shading->getColor(xM, yM, &colorMM);

        // upper-left sub-rectangle
        colors2[0] = colors[0];
        colors2[1] = color0M;
        colors2[2] = colorM0;
        colors2[3] = colorMM;
        doFunctionShFill1(shading, x0, y0, xM, yM, colors2, depth + 1);

        // lower-left sub-rectangle
        colors2[0] = color0M;
        colors2[1] = colors[1];
        colors2[2] = colorMM;
        colors2[3] = colorM1;
        doFunctionShFill1(shading, x0, yM, xM, y1, colors2, depth + 1);

        // upper-right sub-rectangle
        colors2[0] = colorM0;
        colors2[1] = colorMM;
        colors2[2] = colors[2];
        colors2[3] = color1M;
        doFunctionShFill1(shading, xM, y0, x1, yM, colors2, depth + 1);

        // lower-right sub-rectangle
        colors2[0] = colorMM;
        colors2[1] = colorM1;
        colors2[2] = color1M;
        colors2[3] = colors[3];
        doFunctionShFill1(shading, xM, yM, x1, y1, colors2, depth + 1);
    }
}

void Gfx::doAxialShFill(GfxAxialShading *shading)
{
    double xMin, yMin, xMax, yMax;
    double x0, y0, x1, y1;
    double dx, dy, mul;
    bool dxZero, dyZero;
    double bboxIntersections[4];
    double tMin, tMax, tx, ty;
    double s[4], sMin, sMax, tmp;
    double ux0, uy0, ux1, uy1, vx0, vy0, vx1, vy1;
    double t0, t1, tt;
    double ta[axialMaxSplits + 1];
    int next[axialMaxSplits + 1];
    GfxColor color0 = {}, color1 = {};
    int nComps;
    int i, j, k;
    bool needExtend = true;

    // get the clip region bbox
    state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);

    // compute min and max t values, based on the four corners of the
    // clip region bbox
    shading->getCoords(&x0, &y0, &x1, &y1);
    dx = x1 - x0;
    dy = y1 - y0;
    dxZero = fabs(dx) < 0.01;
    dyZero = fabs(dy) < 0.01;
    if (dxZero && dyZero) {
        tMin = tMax = 0;
    } else {
        mul = 1 / (dx * dx + dy * dy);
        bboxIntersections[0] = ((xMin - x0) * dx + (yMin - y0) * dy) * mul;
        bboxIntersections[1] = ((xMin - x0) * dx + (yMax - y0) * dy) * mul;
        bboxIntersections[2] = ((xMax - x0) * dx + (yMin - y0) * dy) * mul;
        bboxIntersections[3] = ((xMax - x0) * dx + (yMax - y0) * dy) * mul;
        std::ranges::sort(bboxIntersections);
        tMin = bboxIntersections[0];
        tMax = bboxIntersections[3];
        if (tMin < 0 && !shading->getExtend0()) {
            tMin = 0;
        }
        if (tMax > 1 && !shading->getExtend1()) {
            tMax = 1;
        }
    }

    if (out->useShadedFills(shading->getType()) && out->axialShadedFill(state, shading, tMin, tMax)) {
        return;
    }

    // get the function domain
    t0 = shading->getDomain0();
    t1 = shading->getDomain1();