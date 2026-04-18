// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 23/41



    // compute the number of steps into which circles must be divided to
    // achieve a curve flatness of 0.1 pixel in device space for the
    // largest circle (note that "device space" is 72 dpi when generating
    // PostScript, hence the relatively small 0.1 pixel accuracy)
    const std::array<double, 6> &ctm = state->getCTM();
    t = fabs(ctm[0]);
    if (fabs(ctm[1]) > t) {
        t = fabs(ctm[1]);
    }
    if (fabs(ctm[2]) > t) {
        t = fabs(ctm[2]);
    }
    if (fabs(ctm[3]) > t) {
        t = fabs(ctm[3]);
    }
    if (r0 > r1) {
        t *= r0;
    } else {
        t *= r1;
    }
    if (t < 1) {
        n = 3;
    } else {
        const double tmp = 1 - 0.1 / t;
        if (unlikely(tmp == 1)) {
            n = 200;
        } else {
            n = (int)(std::numbers::pi / acos(tmp));
        }
        if (n < 3) {
            n = 3;
        } else if (n > 200) {
            n = 200;
        }
    }

    // setup for the start circle
    ia = 0;
    sa = sMin;
    ta = t0 + sa * (t1 - t0);
    xa = x0 + sa * (x1 - x0);
    ya = y0 + sa * (y1 - y0);
    ra = r0 + sa * (r1 - r0);
    getShadingColorRadialHelper(t0, t1, ta, shading, &colorA);

    needExtend = !out->radialShadedSupportExtend(state, shading);

    // fill the circles
    while (ia < radialMaxSplits) {

        // go as far along the t axis (toward t1) as we can, such that the
        // color difference is within the tolerance (radialColorDelta) --
        // this uses bisection (between the current value, t, and t1),
        // limited to radialMaxSplits points along the t axis; require at
        // least one split to avoid problems when the innermost and
        // outermost colors are the same
        ib = radialMaxSplits;
        sb = sMax;
        tb = t0 + sb * (t1 - t0);
        getShadingColorRadialHelper(t0, t1, tb, shading, &colorB);
        while (ib - ia > 1) {
            if (isSameGfxColor(colorB, colorA, nComps, radialColorDelta)) {
                // The shading is not necessarily lineal so having two points with the
                // same color does not mean all the areas in between have the same color too
                int ic = ia + 1;
                for (; ic <= ib; ic++) {
                    const double sc = sMin + ((double)ic / (double)radialMaxSplits) * (sMax - sMin);
                    const double tc = t0 + sc * (t1 - t0);
                    getShadingColorRadialHelper(t0, t1, tc, shading, &colorC);
                    if (!isSameGfxColor(colorC, colorA, nComps, radialColorDelta)) {
                        break;
                    }
                }
                ib = (ic > ia + 1) ? ic - 1 : ia + 1;
                sb = sMin + ((double)ib / (double)radialMaxSplits) * (sMax - sMin);
                tb = t0 + sb * (t1 - t0);
                getShadingColorRadialHelper(t0, t1, tb, shading, &colorB);
                break;
            }
            ib = (ia + ib) / 2;
            sb = sMin + ((double)ib / (double)radialMaxSplits) * (sMax - sMin);
            tb = t0 + sb * (t1 - t0);
            getShadingColorRadialHelper(t0, t1, tb, shading, &colorB);
        }

        // compute center and radius of the circle
        xb = x0 + sb * (x1 - x0);
        yb = y0 + sb * (y1 - y0);
        rb = r0 + sb * (r1 - r0);

        // use the average of the colors at the two circles
        for (k = 0; k < nComps; ++k) {
            colorA.c[k] = safeAverage(colorA.c[k], colorB.c[k]);
        }
        state->setFillColor(&colorA);
        if (out->useFillColorStop()) {
            out->updateFillColorStop(state, (sa - sMin) / (sMax - sMin));
        } else {
            out->updateFillColor(state);
        }

        if (needExtend) {
            if (enclosed) {
                // construct path for first circle (counterclockwise)
                state->moveTo(xa + ra, ya);
                for (k = 1; k < n; ++k) {
                    angle = ((double)k / (double)n) * 2 * std::numbers::pi;
                    state->lineTo(xa + ra * cos(angle), ya + ra * sin(angle));
                }
                state->closePath();