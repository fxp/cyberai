// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 20/41



    // Traverse the t axis and do the shading.
    //
    // For each point (tx, ty) on the t axis, consider a line through
    // that point perpendicular to the t axis:
    //
    //     x(s) = tx + s * -dy   -->   s = (x - tx) / -dy
    //     y(s) = ty + s * dx    -->   s = (y - ty) / dx
    //
    // Then look at the intersection of this line with the bounding box
    // (xMin, yMin, xMax, yMax).  In the general case, there are four
    // intersection points:
    //
    //     s0 = (xMin - tx) / -dy
    //     s1 = (xMax - tx) / -dy
    //     s2 = (yMin - ty) / dx
    //     s3 = (yMax - ty) / dx
    //
    // and we want the middle two s values.
    //
    // In the case where dx = 0, take s0 and s1; in the case where dy =
    // 0, take s2 and s3.
    //
    // Each filled polygon is bounded by two of these line segments
    // perpdendicular to the t axis.
    //
    // The t axis is bisected into smaller regions until the color
    // difference across a region is small enough, and then the region
    // is painted with a single color.

    // set up: require at least one split to avoid problems when the two
    // ends of the t axis have the same color
    nComps = shading->getColorSpace()->getNComps();
    ta[0] = tMin;
    next[0] = axialMaxSplits / 2;
    ta[axialMaxSplits / 2] = 0.5 * (tMin + tMax);
    next[axialMaxSplits / 2] = axialMaxSplits;
    ta[axialMaxSplits] = tMax;

    // compute the color at t = tMin
    if (tMin < 0) {
        tt = t0;
    } else if (tMin > 1) {
        tt = t1;
    } else {
        tt = t0 + (t1 - t0) * tMin;
    }
    shading->getColor(tt, &color0);

    if (out->useFillColorStop()) {
        // make sure we add stop color when t = tMin
        state->setFillColor(&color0);
        out->updateFillColorStop(state, 0);
    }

    // compute the coordinates of the point on the t axis at t = tMin;
    // then compute the intersection of the perpendicular line with the
    // bounding box
    tx = x0 + tMin * dx;
    ty = y0 + tMin * dy;
    if (dxZero && dyZero) {
        sMin = sMax = 0;
    } else if (dxZero) {
        sMin = (xMin - tx) / -dy;
        sMax = (xMax - tx) / -dy;
        if (sMin > sMax) {
            tmp = sMin;
            sMin = sMax;
            sMax = tmp;
        }
    } else if (dyZero) {
        sMin = (yMin - ty) / dx;
        sMax = (yMax - ty) / dx;
        if (sMin > sMax) {
            tmp = sMin;
            sMin = sMax;
            sMax = tmp;
        }
    } else {
        s[0] = (yMin - ty) / dx;
        s[1] = (yMax - ty) / dx;
        s[2] = (xMin - tx) / -dy;
        s[3] = (xMax - tx) / -dy;
        std::ranges::sort(s);
        sMin = s[1];
        sMax = s[2];
    }
    ux0 = tx - sMin * dy;
    uy0 = ty + sMin * dx;
    vx0 = tx - sMax * dy;
    vy0 = ty + sMax * dx;

    i = 0;
    bool doneBBox1, doneBBox2;
    if (dxZero && dyZero) {
        doneBBox1 = doneBBox2 = true;
    } else {
        doneBBox1 = bboxIntersections[1] < tMin;
        doneBBox2 = bboxIntersections[2] > tMax;
    }

    // If output device doesn't support the extended mode required
    // we have to do it here
    needExtend = !out->axialShadedSupportExtend(state, shading);

    while (i < axialMaxSplits) {

        // bisect until color difference is small enough or we hit the
        // bisection limit
        const double previousStop = tt;
        j = next[i];
        while (j > i + 1) {
            if (ta[j] < 0) {
                tt = t0;
            } else if (ta[j] > 1) {
                tt = t1;
            } else {
                tt = t0 + (t1 - t0) * ta[j];
            }

            // Try to determine whether the color map is constant between ta[i] and ta[j].
            // In the strict sense this question cannot be answered by sampling alone.
            // We try an educated guess in form of 2 samples.
            // See https://gitlab.freedesktop.org/poppler/poppler/issues/938 for a file where one sample was not enough.

            // The first test sample at 1.0 (i.e., ta[j]) is coded separately, because we may
            // want to reuse the color later
            shading->getColor(tt, &color1);
            bool isPatchOfConstantColor = isSameGfxColor(color1, color0, nComps, axialColorDelta);

            if (isPatchOfConstantColor) {

                // Add more sample locations here if required
                for (double l : { 0.5 }) {
                    GfxColor tmpColor;
                    double x = previousStop + l * (tt - previousStop);
                    shading->getColor(x, &tmpColor);
                    if (!isSameGfxColor(tmpColor, color0, nComps, axialColorDelta)) {
                        isPatchOfConstantColor = false;
                        break;
                    }
                }
            }