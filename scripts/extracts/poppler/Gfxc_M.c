// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 13/41



    if (state->getFillColorSpace()->getMode() == csPattern) {
        if (numArgs > 1) {
            if (!((GfxPatternColorSpace *)state->getFillColorSpace())->getUnder() || numArgs - 1 != ((GfxPatternColorSpace *)state->getFillColorSpace())->getUnder()->getNComps()) {
                error(errSyntaxError, getPos(), "Incorrect number of arguments in 'scn' command");
                return;
            }
            for (i = 0; i < numArgs - 1 && i < gfxColorMaxComps; ++i) {
                if (args[i].isNum()) {
                    color.c[i] = dblToCol(args[i].getNum());
                } else {
                    color.c[i] = 0; // TODO Investigate if this is what Adobe does
                }
            }
            state->setFillColor(&color);
            out->updateFillColor(state);
        }
        if (numArgs > 0) {
            std::unique_ptr<GfxPattern> pattern;
            if (args[numArgs - 1].isName() && (pattern = res->lookupPattern(args[numArgs - 1].getName(), out, state))) {
                state->setFillPattern(std::move(pattern));
            }
        }

    } else {
        if (numArgs != state->getFillColorSpace()->getNComps()) {
            error(errSyntaxError, getPos(), "Incorrect number of arguments in 'scn' command");
            return;
        }
        state->setFillPattern(nullptr);
        for (i = 0; i < numArgs && i < gfxColorMaxComps; ++i) {
            if (args[i].isNum()) {
                color.c[i] = dblToCol(args[i].getNum());
            } else {
                color.c[i] = 0; // TODO Investigate if this is what Adobe does
            }
        }
        state->setFillColor(&color);
        out->updateFillColor(state);
    }
}

void Gfx::opSetStrokeColorN(Object args[], int numArgs)
{
    if (displayTypes.top() == DisplayType::Type3Font && type3FontIsD1.top()) {
        return;
    }

    GfxColor color;
    int i;

    if (state->getStrokeColorSpace()->getMode() == csPattern) {
        if (numArgs > 1) {
            if (!((GfxPatternColorSpace *)state->getStrokeColorSpace())->getUnder() || numArgs - 1 != ((GfxPatternColorSpace *)state->getStrokeColorSpace())->getUnder()->getNComps()) {
                error(errSyntaxError, getPos(), "Incorrect number of arguments in 'SCN' command");
                return;
            }
            for (i = 0; i < numArgs - 1 && i < gfxColorMaxComps; ++i) {
                if (args[i].isNum()) {
                    color.c[i] = dblToCol(args[i].getNum());
                } else {
                    color.c[i] = 0; // TODO Investigate if this is what Adobe does
                }
            }
            state->setStrokeColor(&color);
            out->updateStrokeColor(state);
        }
        if (unlikely(numArgs <= 0)) {
            error(errSyntaxError, getPos(), "Incorrect number of arguments in 'SCN' command");
            return;
        }
        std::unique_ptr<GfxPattern> pattern;
        if (args[numArgs - 1].isName() && (pattern = res->lookupPattern(args[numArgs - 1].getName(), out, state))) {
            state->setStrokePattern(std::move(pattern));
        }

    } else {
        if (numArgs != state->getStrokeColorSpace()->getNComps()) {
            error(errSyntaxError, getPos(), "Incorrect number of arguments in 'SCN' command");
            return;
        }
        state->setStrokePattern(nullptr);
        for (i = 0; i < numArgs && i < gfxColorMaxComps; ++i) {
            if (args[i].isNum()) {
                color.c[i] = dblToCol(args[i].getNum());
            } else {
                color.c[i] = 0; // TODO Investigate if this is what Adobe does
            }
        }
        state->setStrokeColor(&color);
        out->updateStrokeColor(state);
    }
}

//------------------------------------------------------------------------
// path segment operators
//------------------------------------------------------------------------

void Gfx::opMoveTo(Object args[], int /*numArgs*/)
{
    state->moveTo(args[0].getNum(), args[1].getNum());
}

void Gfx::opLineTo(Object args[], int /*numArgs*/)
{
    if (!state->isCurPt()) {
        error(errSyntaxError, getPos(), "No current point in lineto");
        return;
    }
    state->lineTo(args[0].getNum(), args[1].getNum());
}

void Gfx::opCurveTo(Object args[], int /*numArgs*/)
{
    double x1, y1, x2, y2, x3, y3;

    if (!state->isCurPt()) {
        error(errSyntaxError, getPos(), "No current point in curveto");
        return;
    }
    x1 = args[0].getNum();
    y1 = args[1].getNum();
    x2 = args[2].getNum();
    y2 = args[3].getNum();
    x3 = args[4].getNum();
    y3 = args[5].getNum();
    state->curveTo(x1, y1, x2, y2, x3, y3);
}

void Gfx::opCurveTo1(Object args[], int /*numArgs*/)
{
    double x1, y1, x2, y2, x3, y3;