// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 22/41



static inline void getShadingColorRadialHelper(double t0, double t1, double t, GfxRadialShading *shading, GfxColor *color)
{
    if (t0 < t1) {
        if (t < t0) {
            shading->getColor(t0, color);
        } else if (t > t1) {
            shading->getColor(t1, color);
        } else {
            shading->getColor(t, color);
        }
    } else {
        if (t > t0) {
            shading->getColor(t0, color);
        } else if (t < t1) {
            shading->getColor(t1, color);
        } else {
            shading->getColor(t, color);
        }
    }
}

void Gfx::doRadialShFill(GfxRadialShading *shading)
{
    double xMin, yMin, xMax, yMax;
    double x0, y0, r0, x1, y1, r1, t0, t1;
    int nComps;
    GfxColor colorA = {}, colorB = {}, colorC = {};
    double xa, ya, xb, yb, ra, rb;
    double ta, tb, sa, sb;
    double sz, xz, yz, sMin, sMax;
    bool enclosed;
    int ia, ib, k, n;
    double theta, alpha, angle, t;
    bool needExtend = true;

    // get the shading info
    shading->getCoords(&x0, &y0, &r0, &x1, &y1, &r1);
    t0 = shading->getDomain0();
    t1 = shading->getDomain1();
    nComps = shading->getColorSpace()->getNComps();

    // Compute the point at which r(s) = 0; check for the enclosed
    // circles case; and compute the angles for the tangent lines.
    if (x0 == x1 && y0 == y1) {
        enclosed = true;
        theta = 0; // make gcc happy
        sz = 0; // make gcc happy
    } else if (r0 == r1) {
        enclosed = false;
        theta = 0;
        sz = 0; // make gcc happy
    } else {
        sz = (r1 > r0) ? -r0 / (r1 - r0) : -r1 / (r0 - r1);
        xz = x0 + sz * (x1 - x0);
        yz = y0 + sz * (y1 - y0);
        enclosed = (xz - x0) * (xz - x0) + (yz - y0) * (yz - y0) <= r0 * r0;
        const double theta_aux = sqrt((x0 - xz) * (x0 - xz) + (y0 - yz) * (y0 - yz));
        if (likely(theta_aux != 0)) {
            theta = asin(r0 / theta_aux);
        } else {
            theta = 0;
        }
        if (r0 > r1) {
            theta = -theta;
        }
    }
    if (enclosed) {
        alpha = 0;
    } else {
        alpha = atan2(y1 - y0, x1 - x0);
    }

    // compute the (possibly extended) s range
    state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);
    if (enclosed) {
        sMin = 0;
        sMax = 1;
    } else {
        sMin = 1;
        sMax = 0;
        // solve for x(s) + r(s) = xMin
        if ((x1 + r1) - (x0 + r0) != 0) {
            sa = (xMin - (x0 + r0)) / ((x1 + r1) - (x0 + r0));
            if (sa < sMin) {
                sMin = sa;
            } else if (sa > sMax) {
                sMax = sa;
            }
        }
        // solve for x(s) - r(s) = xMax
        if ((x1 - r1) - (x0 - r0) != 0) {
            sa = (xMax - (x0 - r0)) / ((x1 - r1) - (x0 - r0));
            if (sa < sMin) {
                sMin = sa;
            } else if (sa > sMax) {
                sMax = sa;
            }
        }
        // solve for y(s) + r(s) = yMin
        if ((y1 + r1) - (y0 + r0) != 0) {
            sa = (yMin - (y0 + r0)) / ((y1 + r1) - (y0 + r0));
            if (sa < sMin) {
                sMin = sa;
            } else if (sa > sMax) {
                sMax = sa;
            }
        }
        // solve for y(s) - r(s) = yMax
        if ((y1 - r1) - (y0 - r0) != 0) {
            sa = (yMax - (y0 - r0)) / ((y1 - r1) - (y0 - r0));
            if (sa < sMin) {
                sMin = sa;
            } else if (sa > sMax) {
                sMax = sa;
            }
        }
        // check against sz
        if (r0 < r1) {
            if (sMin < sz) {
                sMin = sz;
            }
        } else if (r0 > r1) {
            if (sMax > sz) {
                sMax = sz;
            }
        }
        // check the 'extend' flags
        if (!shading->getExtend0() && sMin < 0) {
            sMin = 0;
        }
        if (!shading->getExtend1() && sMax > 1) {
            sMax = 1;
        }
    }

    if (out->useShadedFills(shading->getType()) && out->radialShadedFill(state, shading, sMin, sMax)) {
        return;
    }