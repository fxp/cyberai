// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 30/41



    // if we're doing a pattern fill, set up clipping
    render = state->getRender();
    if (!(render & 1) && state->getFillColorSpace()->getMode() == csPattern) {
        patternFill = true;
        saveState();
        // disable fill, enable clipping, leave stroke unchanged
        if ((render ^ (render >> 1)) & 1) {
            render = 5;
        } else {
            render = 7;
        }
        state->setRender(render);
        out->updateRender(state);
    } else {
        patternFill = false;
    }

    state->textTransformDelta(0, state->getRise(), &riseX, &riseY);
    x0 = state->getCurTextX() + riseX;
    y0 = state->getCurTextY() + riseY;

    // handle a Type 3 char
    if (font->getType() == fontType3 && out->interpretType3Chars()) {
        const std::array<double, 6> &oldCTM = state->getCTM();
        const std::array<double, 6> &mat = state->getTextMat();
        tmp[0] = mat[0] * oldCTM[0] + mat[1] * oldCTM[2];
        tmp[1] = mat[0] * oldCTM[1] + mat[1] * oldCTM[3];
        tmp[2] = mat[2] * oldCTM[0] + mat[3] * oldCTM[2];
        tmp[3] = mat[2] * oldCTM[1] + mat[3] * oldCTM[3];
        const std::array<double, 6> &fontMat = font->getFontMatrix();
        newCTM[0] = fontMat[0] * tmp[0] + fontMat[1] * tmp[2];
        newCTM[1] = fontMat[0] * tmp[1] + fontMat[1] * tmp[3];
        newCTM[2] = fontMat[2] * tmp[0] + fontMat[3] * tmp[2];
        newCTM[3] = fontMat[2] * tmp[1] + fontMat[3] * tmp[3];
        newCTM[0] *= state->getFontSize();
        newCTM[1] *= state->getFontSize();
        newCTM[2] *= state->getFontSize();
        newCTM[3] *= state->getFontSize();
        newCTM[0] *= state->getHorizScaling();
        newCTM[1] *= state->getHorizScaling();
        curX = state->getCurTextX();
        curY = state->getCurTextY();
        oldParser = parser;
        p = s.c_str();
        len = s.size();
        while (len > 0) {
            n = font->getNextChar(p, len, &code, &u, &uLen, &dx, &dy, &originX, &originY);
            dx = dx * state->getFontSize() + state->getCharSpace();
            if (n == 1 && *p == ' ') {
                dx += state->getWordSpace();
            }
            dx *= state->getHorizScaling();
            dy *= state->getFontSize();
            state->textTransformDelta(dx, dy, &tdx, &tdy);
            state->transform(curX + riseX, curY + riseY, &x, &y);
            savedState = saveStateStack();
            state->setCTM(newCTM[0], newCTM[1], newCTM[2], newCTM[3], x, y);
            //~ the CTM concat values here are wrong (but never used)
            out->updateCTM(state, 1, 0, 0, 1, 0, 0);
            state->transformDelta(dx, dy, &ddx, &ddy);
            if (!out->beginType3Char(state, curX + riseX, curY + riseY, ddx, ddy, code, u, uLen)) {
                Object charProc = ((Gfx8BitFont *)font)->getCharProcNF(code);
                int refNum = -1;
                if (charProc.isRef()) {
                    refNum = charProc.getRef().num;
                    charProc = charProc.fetch(((Gfx8BitFont *)font)->getCharProcs()->getXRef());
                }
                if ((resDict = ((Gfx8BitFont *)font)->getResources())) {
                    pushResources(resDict);
                }
                if (charProc.isStream()) {
                    Object charProcResourcesObj = charProc.streamGetDict()->lookup("Resources");
                    if (charProcResourcesObj.isDict()) {
                        pushResources(charProcResourcesObj.getDict());
                    }
                    std::set<int>::iterator charProcDrawingIt;
                    bool displayCharProc = true;
                    if (refNum != -1) {
                        bool inserted;
                        std::tie(charProcDrawingIt, inserted) = charProcDrawing.insert(refNum);
                        if (!inserted) {
                            displayCharProc = false;
                            error(errSyntaxError, -1, "CharProc wants to draw a CharProc that is already being drawn");
                        }
                    }
                    if (displayCharProc) {
                        ++displayDepth;
                        display(&charProc, DisplayType::Type3Font);
                        --displayDepth;