// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 28/41

     patch01.color[0][0].c[i] = patch00.color[0][1].c[i];
            patch01.color[0][1].c[i] = patch->color[0][1].c[i];
            patch01.color[1][1].c[i] = (patch->color[0][1].c[i] + patch->color[1][1].c[i]) / 2.;
            patch11.color[0][1].c[i] = patch01.color[1][1].c[i];
            patch11.color[1][1].c[i] = patch->color[1][1].c[i];
            patch11.color[1][0].c[i] = (patch->color[1][1].c[i] + patch->color[1][0].c[i]) / 2.;
            patch10.color[1][1].c[i] = patch11.color[1][0].c[i];
            patch10.color[1][0].c[i] = patch->color[1][0].c[i];
            patch10.color[0][0].c[i] = (patch->color[1][0].c[i] + patch->color[0][0].c[i]) / 2.;
            patch00.color[1][0].c[i] = patch10.color[0][0].c[i];
            patch00.color[1][1].c[i] = (patch00.color[1][0].c[i] + patch01.color[1][1].c[i]) / 2.;
            patch01.color[1][0].c[i] = patch00.color[1][1].c[i];
            patch11.color[0][0].c[i] = patch00.color[1][1].c[i];
            patch10.color[0][1].c[i] = patch00.color[1][1].c[i];
        }
        fillPatch(&patch00, colorComps, patchColorComps, refineColorThreshold, depth + 1, shading);
        fillPatch(&patch10, colorComps, patchColorComps, refineColorThreshold, depth + 1, shading);
        fillPatch(&patch01, colorComps, patchColorComps, refineColorThreshold, depth + 1, shading);
        fillPatch(&patch11, colorComps, patchColorComps, refineColorThreshold, depth + 1, shading);
    }
}

void Gfx::doEndPath()
{
    if (state->isCurPt() && clip != clipNone) {
        state->clip();
        if (clip == clipNormal) {
            out->clip(state);
        } else {
            out->eoClip(state);
        }
    }
    clip = clipNone;
    state->clearPath();
}

//------------------------------------------------------------------------
// path clipping operators
//------------------------------------------------------------------------

void Gfx::opClip(Object /*args*/[], int /*numArgs*/)
{
    clip = clipNormal;
}

void Gfx::opEOClip(Object /*args*/[], int /*numArgs*/)
{
    clip = clipEO;
}

//------------------------------------------------------------------------
// text object operators
//------------------------------------------------------------------------

void Gfx::opBeginText(Object /*args*/[], int /*numArgs*/)
{
    out->beginTextObject(state);
    state->setTextMat(1, 0, 0, 1, 0, 0);
    state->textMoveTo(0, 0);
    out->updateTextMat(state);
    out->updateTextPos(state);
    fontChanged = true;
}

void Gfx::opEndText(Object /*args*/[], int /*numArgs*/)
{
    out->endTextObject(state);
}

//------------------------------------------------------------------------
// text state operators
//------------------------------------------------------------------------

void Gfx::opSetCharSpacing(Object args[], int /*numArgs*/)
{
    state->setCharSpace(args[0].getNum());
    out->updateCharSpace(state);
}

void Gfx::opSetFont(Object args[], int /*numArgs*/)
{
    std::shared_ptr<GfxFont> font;

    if (!(font = res->lookupFont(args[0].getName()))) {
        // unsetting the font (drawing no text) is better than using the
        // previous one and drawing random glyphs from it
        state->setFont(nullptr, args[1].getNum());
        fontChanged = true;
        return;
    }
    if (printCommands) {
        const std::optional<std::string> &fontName = font->getName();
        printf("  font: tag=%s name='%s' %g\n", font->getTag().c_str(), fontName ? fontName->c_str() : "???", args[1].getNum());
        fflush(stdout);
    }

    state->setFont(font, args[1].getNum());
    fontChanged = true;
}

void Gfx::opSetTextLeading(Object args[], int /*numArgs*/)
{
    state->setLeading(args[0].getNum());
}

void Gfx::opSetTextRender(Object args[], int /*numArgs*/)
{
    state->setRender(args[0].getInt());
    out->updateRender(state);
}

void Gfx::opSetTextRise(Object args[], int /*numArgs*/)
{
    state->setRise(args[0].getNum());
    out->updateRise(state);
}

void Gfx::opSetWordSpacing(Object args[], int /*numArgs*/)
{
    state->setWordSpace(args[0].getNum());
    out->updateWordSpace(state);
}

void Gfx::opSetHorizScaling(Object args[], int /*numArgs*/)
{
    state->setHorizScaling(args[0].getNum());
    out->updateHorizScaling(state);
    fontChanged = true;
}

//------------------------------------------------------------------------
// text positioning operators
//------------------------------------------------------------------------

void Gfx::opTextMove(Object args[], int /*numArgs*/)
{
    double tx, ty;

    tx = state->getLineX() + args[0].getNum();
    ty = state->getLineY() + args[1].getNum();
    state->textMoveTo(tx, ty);
    out->updateTextPos(state);
}

void Gfx::opTextMoveSet(Object args[], int /*numArgs*/)
{
    double tx, ty;

    tx = state->getLineX() + args[0].getNum();
    ty = args[1].getNum();
    state->setLeading(-ty);
    ty += state->getLineY();
    state->textMoveTo(tx, ty);
    out->updateTextPos(state);
}