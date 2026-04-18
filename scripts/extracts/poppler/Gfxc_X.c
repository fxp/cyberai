// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 24/41



                // construct and append path for second circle (clockwise)
                state->moveTo(xb + rb, yb);
                for (k = 1; k < n; ++k) {
                    angle = -((double)k / (double)n) * 2 * std::numbers::pi;
                    state->lineTo(xb + rb * cos(angle), yb + rb * sin(angle));
                }
                state->closePath();
            } else {
                // construct the first subpath (clockwise)
                state->moveTo(xa + ra * cos(alpha + theta + 0.5 * std::numbers::pi), ya + ra * sin(alpha + theta + 0.5 * std::numbers::pi));
                for (k = 0; k < n; ++k) {
                    angle = alpha + theta + 0.5 * std::numbers::pi - ((double)k / (double)n) * (2 * theta + std::numbers::pi);
                    state->lineTo(xb + rb * cos(angle), yb + rb * sin(angle));
                }
                for (k = 0; k < n; ++k) {
                    angle = alpha - theta - 0.5 * std::numbers::pi + ((double)k / (double)n) * (2 * theta - std::numbers::pi);
                    state->lineTo(xa + ra * cos(angle), ya + ra * sin(angle));
                }
                state->closePath();

                // construct the second subpath (counterclockwise)
                state->moveTo(xa + ra * cos(alpha + theta + 0.5 * std::numbers::pi), ya + ra * sin(alpha + theta + 0.5 * std::numbers::pi));
                for (k = 0; k < n; ++k) {
                    angle = alpha + theta + 0.5 * std::numbers::pi + ((double)k / (double)n) * (-2 * theta + std::numbers::pi);
                    state->lineTo(xb + rb * cos(angle), yb + rb * sin(angle));
                }
                for (k = 0; k < n; ++k) {
                    angle = alpha - theta - 0.5 * std::numbers::pi + ((double)k / (double)n) * (2 * theta + std::numbers::pi);
                    state->lineTo(xa + ra * cos(angle), ya + ra * sin(angle));
                }
                state->closePath();
            }
        }

        if (!out->useFillColorStop()) {
            // fill the path
            out->fill(state);
            state->clearPath();
        }

        // step to the next value of t
        ia = ib;
        sa = sb;
        ta = tb;
        xa = xb;
        ya = yb;
        ra = rb;
        colorA = colorB;
    }

    if (out->useFillColorStop()) {
        // make sure we add stop color when sb = sMax
        state->setFillColor(&colorA);
        out->updateFillColorStop(state, (sb - sMin) / (sMax - sMin));

        // fill the path
        state->moveTo(xMin, yMin);
        state->lineTo(xMin, yMax);
        state->lineTo(xMax, yMax);
        state->lineTo(xMax, yMin);
        state->closePath();

        out->fill(state);
        state->clearPath();
    }

    if (!needExtend) {
        return;
    }

    if (enclosed) {
        // extend the smaller circle
        if ((shading->getExtend0() && r0 <= r1) || (shading->getExtend1() && r1 < r0)) {
            if (r0 <= r1) {
                ta = t0;
                ra = r0;
                xa = x0;
                ya = y0;
            } else {
                ta = t1;
                ra = r1;
                xa = x1;
                ya = y1;
            }
            shading->getColor(ta, &colorA);
            state->setFillColor(&colorA);
            out->updateFillColor(state);
            state->moveTo(xa + ra, ya);
            for (k = 1; k < n; ++k) {
                angle = ((double)k / (double)n) * 2 * std::numbers::pi;
                state->lineTo(xa + ra * cos(angle), ya + ra * sin(angle));
            }
            state->closePath();
            out->fill(state);
            state->clearPath();
        }

        // extend the larger circle
        if ((shading->getExtend0() && r0 > r1) || (shading->getExtend1() && r1 >= r0)) {
            if (r0 > r1) {
                ta = t0;
                ra = r0;
                xa = x0;
                ya = y0;
            } else {
                ta = t1;
                ra = r1;
                xa = x1;
                ya = y1;
            }
            shading->getColor(ta, &colorA);
            state->setFillColor(&colorA);
            out->updateFillColor(state);
            state->moveTo(xMin, yMin);
            state->lineTo(xMin, yMax);
            state->lineTo(xMax, yMax);
            state->lineTo(xMax, yMin);
            state->closePath();
            state->moveTo(xa + ra, ya);
            for (k = 1; k < n; ++k) {
                angle = ((double)k / (double)n) * 2 * std::numbers::pi;
                state->lineTo(xa + ra * cos(angle), ya + ra * sin(angle));
            }
            state->closePath();
            out->fill(state);
            state->clearPath();
        }
    }
}

void Gfx::doGouraudTriangleShFill(GfxGouraudTriangleShading *shading)
{
    double x0, y0, x1, y1, x2, y2;
    int i;

    if (out->useShadedFills(shading->getType())) {
        if (out->gouraudTriangleShadedFill(state, shading)) {
            return;
        }
    }