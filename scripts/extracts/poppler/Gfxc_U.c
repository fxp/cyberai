// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 21/41



            if (isPatchOfConstantColor) {
                // in these two if what we guarantee is that if we are skipping lots of
                // positions because the colors are the same, we still create a region
                // with vertexs passing by bboxIntersections[1] and bboxIntersections[2]
                // otherwise we can have empty regions that should really be painted
                // like happened in bug 19896
                // What we do to ensure that we pass a line through this points
                // is making sure use the exact bboxIntersections[] value as one of the used ta[] values
                if (!doneBBox1 && ta[i] < bboxIntersections[1] && ta[j] > bboxIntersections[1]) {
                    int teoricalj = (int)((bboxIntersections[1] - tMin) * axialMaxSplits / (tMax - tMin));
                    if (teoricalj <= i) {
                        teoricalj = i + 1;
                    }
                    if (teoricalj < j) {
                        next[i] = teoricalj;
                        next[teoricalj] = j;
                    } else {
                        teoricalj = j;
                    }
                    ta[teoricalj] = bboxIntersections[1];
                    j = teoricalj;
                    doneBBox1 = true;
                }
                if (!doneBBox2 && ta[i] < bboxIntersections[2] && ta[j] > bboxIntersections[2]) {
                    int teoricalj = (int)((bboxIntersections[2] - tMin) * axialMaxSplits / (tMax - tMin));
                    if (teoricalj <= i) {
                        teoricalj = i + 1;
                    }
                    if (teoricalj < j) {
                        next[i] = teoricalj;
                        next[teoricalj] = j;
                    } else {
                        teoricalj = j;
                    }
                    ta[teoricalj] = bboxIntersections[2];
                    j = teoricalj;
                    doneBBox2 = true;
                }
                break;
            }
            k = (i + j) / 2;
            ta[k] = 0.5 * (ta[i] + ta[j]);
            next[i] = k;
            next[k] = j;
            j = k;
        }

        // use the average of the colors of the two sides of the region
        for (k = 0; k < nComps; ++k) {
            color0.c[k] = safeAverage(color0.c[k], color1.c[k]);
        }

        // compute the coordinates of the point on the t axis; then
        // compute the intersection of the perpendicular line with the
        // bounding box
        tx = x0 + ta[j] * dx;
        ty = y0 + ta[j] * dy;
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
        ux1 = tx - sMin * dy;
        uy1 = ty + sMin * dx;
        vx1 = tx - sMax * dy;
        vy1 = ty + sMax * dx;

        // set the color
        state->setFillColor(&color0);
        if (out->useFillColorStop()) {
            out->updateFillColorStop(state, (ta[j] - tMin) / (tMax - tMin));
        } else {
            out->updateFillColor(state);
        }

        if (needExtend) {
            // fill the region
            state->moveTo(ux0, uy0);
            state->lineTo(vx0, vy0);
            state->lineTo(vx1, vy1);
            state->lineTo(ux1, uy1);
            state->closePath();
        }

        if (!out->useFillColorStop()) {
            out->fill(state);
            state->clearPath();
        }

        // set up for next region
        ux0 = ux1;
        uy0 = uy1;
        vx0 = vx1;
        vy0 = vy1;
        color0 = color1;
        i = next[i];
    }

    if (out->useFillColorStop()) {
        if (!needExtend) {
            state->moveTo(xMin, yMin);
            state->lineTo(xMin, yMax);
            state->lineTo(xMax, yMax);
            state->lineTo(xMax, yMin);
            state->closePath();
        }
        out->fill(state);
        state->clearPath();
    }
}