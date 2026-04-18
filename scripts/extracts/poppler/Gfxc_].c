// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 29/41



void Gfx::opSetTextMatrix(Object args[], int /*numArgs*/)
{
    state->setTextMat(args[0].getNum(), args[1].getNum(), args[2].getNum(), args[3].getNum(), args[4].getNum(), args[5].getNum());
    state->textMoveTo(0, 0);
    out->updateTextMat(state);
    out->updateTextPos(state);
    fontChanged = true;
}

void Gfx::opTextNextLine(Object /*args*/[], int /*numArgs*/)
{
    double tx, ty;

    tx = state->getLineX();
    ty = state->getLineY() - state->getLeading();
    state->textMoveTo(tx, ty);
    out->updateTextPos(state);
}

//------------------------------------------------------------------------
// text string operators
//------------------------------------------------------------------------

void Gfx::opShowText(Object args[], int /*numArgs*/)
{
    if (!state->getFont()) {
        error(errSyntaxError, getPos(), "No font in show");
        return;
    }
    if (fontChanged) {
        out->updateFont(state);
        fontChanged = false;
    }
    out->beginStringOp(state);
    doShowText(args[0].getString());
    out->endStringOp(state);
    if (!ocState) {
        doIncCharCount(args[0].getString());
    }
}

void Gfx::opMoveShowText(Object args[], int /*numArgs*/)
{
    double tx, ty;

    if (!state->getFont()) {
        error(errSyntaxError, getPos(), "No font in move/show");
        return;
    }
    if (fontChanged) {
        out->updateFont(state);
        fontChanged = false;
    }
    tx = state->getLineX();
    ty = state->getLineY() - state->getLeading();
    state->textMoveTo(tx, ty);
    out->updateTextPos(state);
    out->beginStringOp(state);
    doShowText(args[0].getString());
    out->endStringOp(state);
    if (!ocState) {
        doIncCharCount(args[0].getString());
    }
}

void Gfx::opMoveSetShowText(Object args[], int /*numArgs*/)
{
    double tx, ty;

    if (!state->getFont()) {
        error(errSyntaxError, getPos(), "No font in move/set/show");
        return;
    }
    if (fontChanged) {
        out->updateFont(state);
        fontChanged = false;
    }
    state->setWordSpace(args[0].getNum());
    state->setCharSpace(args[1].getNum());
    tx = state->getLineX();
    ty = state->getLineY() - state->getLeading();
    state->textMoveTo(tx, ty);
    out->updateWordSpace(state);
    out->updateCharSpace(state);
    out->updateTextPos(state);
    out->beginStringOp(state);
    doShowText(args[2].getString());
    out->endStringOp(state);
    if (ocState) {
        doIncCharCount(args[2].getString());
    }
}

void Gfx::opShowSpaceText(Object args[], int /*numArgs*/)
{
    Array *a;
    int i;

    if (!state->getFont()) {
        error(errSyntaxError, getPos(), "No font in show/space");
        return;
    }
    if (fontChanged) {
        out->updateFont(state);
        fontChanged = false;
    }
    out->beginStringOp(state);
    const GfxFont::WritingMode wMode = state->getFont()->getWMode();
    a = args[0].getArray();
    for (i = 0; i < a->getLength(); ++i) {
        Object obj = a->get(i);
        if (obj.isNum()) {
            // this uses the absolute value of the font size to match
            // Acrobat's behavior
            if (wMode == GfxFont::WritingMode::Vertical) {
                state->textShift(0, -obj.getNum() * 0.001 * state->getFontSize());
            } else {
                state->textShift(-obj.getNum() * 0.001 * state->getFontSize() * state->getHorizScaling(), 0);
            }
            out->updateTextShift(state, obj.getNum());
        } else if (obj.isString()) {
            doShowText(obj.getString());
        } else {
            error(errSyntaxError, getPos(), "Element of show/space array must be number or string");
        }
    }
    out->endStringOp(state);
    if (!ocState) {
        a = args[0].getArray();
        for (i = 0; i < a->getLength(); ++i) {
            Object obj = a->get(i);
            if (obj.isString()) {
                doIncCharCount(obj.getString());
            }
        }
    }
}

void Gfx::doShowText(const std::string &s)
{
    double riseX, riseY;
    CharCode code;
    const Unicode *u = nullptr;
    double x, y, dx, dy, dx2, dy2, curX, curY, tdx, tdy, ddx, ddy;
    double originX, originY, tOriginX, tOriginY;
    double x0, y0, x1, y1;
    double tmp[4], newCTM[6];
    Dict *resDict;
    Parser *oldParser;
    GfxState *savedState;
    const char *p;
    int render;
    bool patternFill;
    int len, n, uLen, nChars, nSpaces;

    GfxFont *const font = state->getFont().get();
    const GfxFont::WritingMode wMode = font->getWMode();

    if (out->useDrawChar()) {
        out->beginString(state, s);
    }