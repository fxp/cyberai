// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 14/41



    if (!state->isCurPt()) {
        error(errSyntaxError, getPos(), "No current point in curveto1");
        return;
    }
    x1 = state->getCurX();
    y1 = state->getCurY();
    x2 = args[0].getNum();
    y2 = args[1].getNum();
    x3 = args[2].getNum();
    y3 = args[3].getNum();
    state->curveTo(x1, y1, x2, y2, x3, y3);
}

void Gfx::opCurveTo2(Object args[], int /*numArgs*/)
{
    double x1, y1, x2, y2, x3, y3;

    if (!state->isCurPt()) {
        error(errSyntaxError, getPos(), "No current point in curveto2");
        return;
    }
    x1 = args[0].getNum();
    y1 = args[1].getNum();
    x2 = args[2].getNum();
    y2 = args[3].getNum();
    x3 = x2;
    y3 = y2;
    state->curveTo(x1, y1, x2, y2, x3, y3);
}

void Gfx::opRectangle(Object args[], int /*numArgs*/)
{
    double x, y, w, h;

    x = args[0].getNum();
    y = args[1].getNum();
    w = args[2].getNum();
    h = args[3].getNum();
    state->moveTo(x, y);
    state->lineTo(x + w, y);
    state->lineTo(x + w, y + h);
    state->lineTo(x, y + h);
    state->closePath();
}

void Gfx::opClosePath(Object /*args*/[], int /*numArgs*/)
{
    if (!state->isCurPt()) {
        error(errSyntaxError, getPos(), "No current point in closepath");
        return;
    }
    state->closePath();
}

//------------------------------------------------------------------------
// path painting operators
//------------------------------------------------------------------------

void Gfx::opEndPath(Object /*args*/[], int /*numArgs*/)
{
    doEndPath();
}

void Gfx::opStroke(Object /*args*/[], int /*numArgs*/)
{
    if (!state->isCurPt()) {
        // error(errSyntaxError, getPos(), "No path in stroke");
        return;
    }
    if (state->isPath()) {
        if (ocState) {
            if (state->getStrokeColorSpace()->getMode() == csPattern) {
                doPatternStroke();
            } else {
                out->stroke(state);
            }
        }
    }
    doEndPath();
}

void Gfx::opCloseStroke(Object * /*args[]*/, int /*numArgs*/)
{
    if (!state->isCurPt()) {
        // error(errSyntaxError, getPos(), "No path in closepath/stroke");
        return;
    }
    if (state->isPath()) {
        state->closePath();
        if (ocState) {
            if (state->getStrokeColorSpace()->getMode() == csPattern) {
                doPatternStroke();
            } else {
                out->stroke(state);
            }
        }
    }
    doEndPath();
}

void Gfx::opFill(Object /*args*/[], int /*numArgs*/)
{
    if (!state->isCurPt()) {
        // error(errSyntaxError, getPos(), "No path in fill");
        return;
    }
    if (state->isPath()) {
        if (ocState) {
            if (state->getFillColorSpace()->getMode() == csPattern) {
                doPatternFill(false);
            } else {
                out->fill(state);
            }
        }
    }
    doEndPath();
}

void Gfx::opEOFill(Object /*args*/[], int /*numArgs*/)
{
    if (!state->isCurPt()) {
        // error(errSyntaxError, getPos(), "No path in eofill");
        return;
    }
    if (state->isPath()) {
        if (ocState) {
            if (state->getFillColorSpace()->getMode() == csPattern) {
                doPatternFill(true);
            } else {
                out->eoFill(state);
            }
        }
    }
    doEndPath();
}

void Gfx::opFillStroke(Object /*args*/[], int /*numArgs*/)
{
    if (!state->isCurPt()) {
        // error(errSyntaxError, getPos(), "No path in fill/stroke");
        return;
    }
    if (state->isPath()) {
        if (ocState) {
            if (state->getFillColorSpace()->getMode() == csPattern) {
                doPatternFill(false);
            } else {
                out->fill(state);
            }
            if (state->getStrokeColorSpace()->getMode() == csPattern) {
                doPatternStroke();
            } else {
                out->stroke(state);
            }
        }
    }
    doEndPath();
}

void Gfx::opCloseFillStroke(Object /*args*/[], int /*numArgs*/)
{
    if (!state->isCurPt()) {
        // error(errSyntaxError, getPos(), "No path in closepath/fill/stroke");
        return;
    }
    if (state->isPath()) {
        state->closePath();
        if (ocState) {
            if (state->getFillColorSpace()->getMode() == csPattern) {
                doPatternFill(false);
            } else {
                out->fill(state);
            }
            if (state->getStrokeColorSpace()->getMode() == csPattern) {
                doPatternStroke();
            } else {
                out->stroke(state);
            }
        }
    }
    doEndPath();
}