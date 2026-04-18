// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 30/49



    // A radial pattern is considered degenerate if it can be
    // represented as a solid or clear pattern.  This corresponds to one
    // of the two cases:
    //
    // 1) The radii are both very small:
    //      |dr| < FLT_EPSILON && min (r0, r1) < FLT_EPSILON
    //
    // 2) The two circles have about the same radius and are very
    //    close to each other (approximately a cylinder gradient that
    //    doesn't move with the parameter):
    //      |dr| < FLT_EPSILON && max (|dx|, |dy|) < 2 * FLT_EPSILON

    if (xMin >= xMax || yMin >= yMax || (fabs(r0 - r1) < RADIAL_EPSILON && (std::min<double>(r0, r1) < RADIAL_EPSILON || std::max<double>(fabs(x0 - x1), fabs(y0 - y1)) < 2 * RADIAL_EPSILON))) {
        *lower = *upper = 0;
        return;
    }

    range[0] = range[1] = 0;
    valid = false;

    x_focus = y_focus = 0; // silence gcc

    cx = x0;
    cy = y0;
    cr = r0;
    dx = x1 - cx;
    dy = y1 - cy;
    dr = r1 - cr;

    // translate by -(cx, cy) to simplify computations
    xMin -= cx;
    yMin -= cy;
    xMax -= cx;
    yMax -= cy;

    // enlarge boundaries slightly to avoid rounding problems in the
    // parameter range computation
    xMin -= RADIAL_EPSILON;
    yMin -= RADIAL_EPSILON;
    xMax += RADIAL_EPSILON;
    yMax += RADIAL_EPSILON;

    // enlarge boundaries even more to avoid rounding problems when
    // testing if a point belongs to the box
    minx = xMin - RADIAL_EPSILON;
    miny = yMin - RADIAL_EPSILON;
    maxx = xMax + RADIAL_EPSILON;
    maxy = yMax + RADIAL_EPSILON;

    // we dont' allow negative radiuses, so we will be checking that
    // t*dr >= mindr to consider t valid
    mindr = -(cr + RADIAL_EPSILON);

    // After the previous transformations, the start circle is centered
    // in the origin and has radius cr. A 1-unit change in the t
    // parameter corresponds to dx,dy,dr changes in the x,y,r of the
    // circle (center coordinates, radius).
    //
    // To compute the minimum range needed to correctly draw the
    // pattern, we start with an empty range and extend it to include
    // the circles touching the bounding box or within it.

    // Focus, the point where the circle has radius == 0.
    //
    // r = cr + t * dr = 0
    // t = -cr / dr
    //
    // If the radius is constant (dr == 0) there is no focus (the
    // gradient represents a cylinder instead of a cone).
    if (fabs(dr) >= RADIAL_EPSILON) {
        double t_focus;

        t_focus = -cr / dr;
        x_focus = t_focus * dx;
        y_focus = t_focus * dy;
        if (minx <= x_focus && x_focus <= maxx && miny <= y_focus && y_focus <= maxy) {
            valid = radialExtendRange(range, t_focus, valid);
        }
    }

    // Circles externally tangent to box edges.
    //
    // All circles have center in (dx, dy) * t
    //
    // If the circle is tangent to the line defined by the edge of the
    // box, then at least one of the following holds true:
    //
    //   (dx*t) + (cr + dr*t) == x0 (left   edge)
    //   (dx*t) - (cr + dr*t) == x1 (right  edge)
    //   (dy*t) + (cr + dr*t) == y0 (top    edge)
    //   (dy*t) - (cr + dr*t) == y1 (bottom edge)
    //
    // The solution is only valid if the tangent point is actually on
    // the edge, i.e. if its y coordinate is in [y0,y1] for left/right
    // edges and if its x coordinate is in [x0,x1] for top/bottom edges.
    //
    // For the first equation:
    //
    //   (dx + dr) * t = x0 - cr
    //   t = (x0 - cr) / (dx + dr)
    //   y = dy * t
    //
    // in the code this becomes:
    //
    //   t_edge = (num) / (den)
    //   v = (delta) * t_edge
    //
    // If the denominator in t is 0, the pattern is tangent to a line
    // parallel to the edge under examination. The corner-case where the
    // boundary line is the same as the edge is handled by the focus
    // point case and/or by the a==0 case.

    // circles tangent (externally) to left/right/top/bottom edge
    radialEdge(xMin - cr, dx + dr, dy, miny, maxy, dr, mindr, valid, range);
    radialEdge(xMax + cr, dx - dr, dy, miny, maxy, dr, mindr, valid, range);
    radialEdge(yMin - cr, dy + dr, dx, minx, maxx, dr, mindr, valid, range);
    radialEdge(yMax + cr, dy - dr, dx, minx, maxx, dr, mindr, valid, range);

    // Circles passing through a corner.
    //
    // A circle passing through the point (x,y) satisfies:
    //
    // (x-t*dx)^2 + (y-t*dy)^2 == (cr + t*dr)^2
    //
    // If we set:
    //   a = dx^2 + dy^2 - dr^2
    //   b = x*dx + y*dy + cr*dr
    //   c = x^2 + y^2 - cr^2
    // we have:
    //   a*t^2 - 2*b*t + c == 0

    a = dx * dx + dy * dy - dr * dr;
    if (fabs(a) < RADIAL_EPSILON * RADIAL_EPSILON) {
        double b;