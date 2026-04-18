// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 31/41



                        if (refNum != -1) {
                            charProcDrawing.erase(charProcDrawingIt);
                        }
                    }
                    if (charProcResourcesObj.isDict()) {
                        popResources();
                    }
                } else {
                    error(errSyntaxError, getPos(), "Missing or bad Type3 CharProc entry");
                }
                out->endType3Char(state);
                if (resDict) {
                    popResources();
                }
            }
            restoreStateStack(savedState);
            // GfxState::restore() does *not* restore the current position,
            // so we deal with it here using (curX, curY) and (lineX, lineY)
            curX += tdx;
            curY += tdy;
            state->textShiftWithUserCoords(tdx, tdy);
            // Call updateCTM with the identity transformation.  That way, the CTM is unchanged,
            // but any side effect that the method may have is triggered.  This is the case,
            // in particular, for the Splash backend.
            out->updateCTM(state, 1, 0, 0, 1, 0, 0);
            p += n;
            len -= n;
        }
        parser = oldParser;

    } else if (out->useDrawChar()) {
        p = s.c_str();
        len = s.size();
        while (len > 0) {
            n = font->getNextChar(p, len, &code, &u, &uLen, &dx, &dy, &originX, &originY);
            if (wMode == GfxFont::WritingMode::Vertical) {
                dx *= state->getFontSize();
                dy = dy * state->getFontSize() + state->getCharSpace();
                if (n == 1 && *p == ' ') {
                    dy += state->getWordSpace();
                }
            } else {
                dx = dx * state->getFontSize() + state->getCharSpace();
                if (n == 1 && *p == ' ') {
                    dx += state->getWordSpace();
                }
                dx *= state->getHorizScaling();
                dy *= state->getFontSize();
            }
            state->textTransformDelta(dx, dy, &tdx, &tdy);
            originX *= state->getFontSize();
            originY *= state->getFontSize();
            state->textTransformDelta(originX, originY, &tOriginX, &tOriginY);
            if (ocState) {
                out->drawChar(state, state->getCurTextX() + riseX, state->getCurTextY() + riseY, tdx, tdy, tOriginX, tOriginY, code, n, u, uLen);
            }
            state->textShiftWithUserCoords(tdx, tdy);
            p += n;
            len -= n;
        }
    } else {
        dx = dy = 0;
        p = s.c_str();
        len = s.size();
        nChars = nSpaces = 0;
        while (len > 0) {
            n = font->getNextChar(p, len, &code, &u, &uLen, &dx2, &dy2, &originX, &originY);
            dx += dx2;
            dy += dy2;
            if (n == 1 && *p == ' ') {
                ++nSpaces;
            }
            ++nChars;
            p += n;
            len -= n;
        }
        if (wMode == GfxFont::WritingMode::Vertical) {
            dx *= state->getFontSize();
            dy = dy * state->getFontSize() + nChars * state->getCharSpace() + nSpaces * state->getWordSpace();
        } else {
            dx = dx * state->getFontSize() + nChars * state->getCharSpace() + nSpaces * state->getWordSpace();
            dx *= state->getHorizScaling();
            dy *= state->getFontSize();
        }
        state->textTransformDelta(dx, dy, &tdx, &tdy);
        if (ocState) {
            out->drawString(state, s);
        }
        state->textShiftWithUserCoords(tdx, tdy);
    }

    if (out->useDrawChar()) {
        out->endString(state);
    }

    if (patternFill && ocState) {
        out->saveTextPos(state);
        // tell the OutputDev to do the clipping
        out->endTextObject(state);
        // set up a clipping bbox so doPatternText will work -- assume
        // that the text bounding box does not extend past the baseline in
        // any direction by more than twice the font size
        x1 = state->getCurTextX() + riseX;
        y1 = state->getCurTextY() + riseY;
        if (x0 > x1) {
            x = x0;
            x0 = x1;
            x1 = x;
        }
        if (y0 > y1) {
            y = y0;
            y0 = y1;
            y1 = y;
        }
        state->textTransformDelta(0, state->getFontSize(), &dx, &dy);
        state->textTransformDelta(state->getFontSize(), 0, &dx2, &dy2);
        dx = fabs(dx);
        dx2 = fabs(dx2);
        if (dx2 > dx) {
            dx = dx2;
        }
        dy = fabs(dy);
        dy2 = fabs(dy2);
        if (dy2 > dy) {
            dy = dy2;
        }
        state->clipToRect(x0 - 2 * dx, y0 - 2 * dy, x1 + 2 * dx, y1 + 2 * dy);
        // set render mode to fill-only
        state->setRender(0);
        out->updateRender(state);
        doPatternText();
        restoreState();
        out->restoreTextPos(state);
    }

    updateLevel += 10 * s.size();
}