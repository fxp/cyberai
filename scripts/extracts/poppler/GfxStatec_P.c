// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 48/49



    xMin = xMax = yMin = yMax = 0; // make gcc happy
    for (i = 0; i < path->getNumSubpaths(); ++i) {
        subpath = path->getSubpath(i);
        for (j = 0; j < subpath->getNumPoints(); ++j) {
            transform(subpath->getX(j), subpath->getY(j), &x, &y);
            if (i == 0 && j == 0) {
                xMin = xMax = x;
                yMin = yMax = y;
            } else {
                if (x < xMin) {
                    xMin = x;
                } else if (x > xMax) {
                    xMax = x;
                }
                if (y < yMin) {
                    yMin = y;
                } else if (y > yMax) {
                    yMax = y;
                }
            }
        }
    }
    if (xMin > clipXMin) {
        clipXMin = xMin;
    }
    if (yMin > clipYMin) {
        clipYMin = yMin;
    }
    if (xMax < clipXMax) {
        clipXMax = xMax;
    }
    if (yMax < clipYMax) {
        clipYMax = yMax;
    }
}

void GfxState::clipToStrokePath()
{
    double xMin, yMin, xMax, yMax, x, y, t0, t1;
    GfxSubpath *subpath;
    int i, j;

    xMin = xMax = yMin = yMax = 0; // make gcc happy
    for (i = 0; i < path->getNumSubpaths(); ++i) {
        subpath = path->getSubpath(i);
        for (j = 0; j < subpath->getNumPoints(); ++j) {
            transform(subpath->getX(j), subpath->getY(j), &x, &y);
            if (i == 0 && j == 0) {
                xMin = xMax = x;
                yMin = yMax = y;
            } else {
                if (x < xMin) {
                    xMin = x;
                } else if (x > xMax) {
                    xMax = x;
                }
                if (y < yMin) {
                    yMin = y;
                } else if (y > yMax) {
                    yMax = y;
                }
            }
        }
    }

    // allow for the line width
    //~ miter joins can extend farther than this
    t0 = fabs(ctm[0]);
    t1 = fabs(ctm[2]);
    if (t0 > t1) {
        xMin -= 0.5 * lineWidth * t0;
        xMax += 0.5 * lineWidth * t0;
    } else {
        xMin -= 0.5 * lineWidth * t1;
        xMax += 0.5 * lineWidth * t1;
    }
    t0 = fabs(ctm[0]);
    t1 = fabs(ctm[3]);
    if (t0 > t1) {
        yMin -= 0.5 * lineWidth * t0;
        yMax += 0.5 * lineWidth * t0;
    } else {
        yMin -= 0.5 * lineWidth * t1;
        yMax += 0.5 * lineWidth * t1;
    }

    if (xMin > clipXMin) {
        clipXMin = xMin;
    }
    if (yMin > clipYMin) {
        clipYMin = yMin;
    }
    if (xMax < clipXMax) {
        clipXMax = xMax;
    }
    if (yMax < clipYMax) {
        clipYMax = yMax;
    }
}

void GfxState::clipToRect(double xMin, double yMin, double xMax, double yMax)
{
    double x, y, xMin1, yMin1, xMax1, yMax1;

    transform(xMin, yMin, &x, &y);
    xMin1 = xMax1 = x;
    yMin1 = yMax1 = y;
    transform(xMax, yMin, &x, &y);
    if (x < xMin1) {
        xMin1 = x;
    } else if (x > xMax1) {
        xMax1 = x;
    }
    if (y < yMin1) {
        yMin1 = y;
    } else if (y > yMax1) {
        yMax1 = y;
    }
    transform(xMax, yMax, &x, &y);
    if (x < xMin1) {
        xMin1 = x;
    } else if (x > xMax1) {
        xMax1 = x;
    }
    if (y < yMin1) {
        yMin1 = y;
    } else if (y > yMax1) {
        yMax1 = y;
    }
    transform(xMin, yMax, &x, &y);
    if (x < xMin1) {
        xMin1 = x;
    } else if (x > xMax1) {
        xMax1 = x;
    }
    if (y < yMin1) {
        yMin1 = y;
    } else if (y > yMax1) {
        yMax1 = y;
    }

    if (xMin1 > clipXMin) {
        clipXMin = xMin1;
    }
    if (yMin1 > clipYMin) {
        clipYMin = yMin1;
    }
    if (xMax1 < clipXMax) {
        clipXMax = xMax1;
    }
    if (yMax1 < clipYMax) {
        clipYMax = yMax1;
    }
}

void GfxState::textShift(double tx, double ty)
{
    double dx, dy;

    textTransformDelta(tx, ty, &dx, &dy);
    curTextX += dx;
    curTextY += dy;
}

void GfxState::textShiftWithUserCoords(double dx, double dy)
{
    curTextX += dx;
    curTextY += dy;
}

GfxState *GfxState::save()
{
    GfxState *newState;

    newState = copy();
    newState->saved = this;
    return newState;
}

GfxState *GfxState::restore()
{
    GfxState *oldState;

    if (saved) {
        oldState = saved;

        // these attributes aren't saved/restored by the q/Q operators
        oldState->path = path;
        oldState->curX = curX;
        oldState->curY = curY;
        oldState->curTextX = curTextX;
        oldState->curTextY = curTextY;
        oldState->lineX = lineX;
        oldState->lineY = lineY;

        path = nullptr;
        saved = nullptr;
        delete this;

    } else {
        oldState = this;
    }

    return oldState;
}

bool GfxState::parseBlendMode(Object *obj, GfxBlendMode *mode)
{
    int i, j;