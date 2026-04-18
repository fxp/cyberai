// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 38/41



void Gfx::drawForm(Object *str, Dict *resDict, const std::array<double, 6> &matrix, const std::array<double, 4> &bbox, bool transpGroup, bool softMask, GfxColorSpace *blendingColorSpace, bool isolated, bool knockout, bool alpha,
                   Function *transferFunc, GfxColor *backdropColor)
{
    Parser *oldParser;
    GfxState *savedState;

    // push new resources on stack
    pushResources(resDict);

    // save current graphics state
    savedState = saveStateStack();

    // kill any pre-existing path
    state->clearPath();

    // save current parser
    oldParser = parser;

    // set form transformation matrix
    state->concatCTM(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]);
    out->updateCTM(state, matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]);

    // set form bounding box
    state->moveTo(bbox[0], bbox[1]);
    state->lineTo(bbox[2], bbox[1]);
    state->lineTo(bbox[2], bbox[3]);
    state->lineTo(bbox[0], bbox[3]);
    state->closePath();
    state->clip();
    out->clip(state);
    state->clearPath();

    if (softMask || transpGroup) {
        if (state->getBlendMode() != gfxBlendNormal) {
            state->setBlendMode(gfxBlendNormal);
            out->updateBlendMode(state);
        }
        if (state->getFillOpacity() != 1) {
            state->setFillOpacity(1);
            out->updateFillOpacity(state);
        }
        if (state->getStrokeOpacity() != 1) {
            state->setStrokeOpacity(1);
            out->updateStrokeOpacity(state);
        }
        out->clearSoftMask(state);
        out->beginTransparencyGroup(state, bbox, blendingColorSpace, isolated, knockout, softMask);
    }

    // set new base matrix
    const std::array<double, 6> oldBaseMatrix = baseMatrix;
    baseMatrix = state->getCTM();

    GfxState *stateBefore = state;

    // draw the form
    ++displayDepth;
    display(str, DisplayType::Form);
    --displayDepth;

    if (stateBefore != state) {
        if (state->isParentState(stateBefore)) {
            error(errSyntaxError, -1, "There's a form with more q than Q, trying to fix");
            while (stateBefore != state) {
                restoreState();
            }
        } else {
            error(errSyntaxError, -1, "There's a form with more Q than q");
        }
    }

    if (softMask || transpGroup) {
        out->endTransparencyGroup(state);
    }

    // restore base matrix
    baseMatrix = oldBaseMatrix;

    // restore parser
    parser = oldParser;

    // restore graphics state
    restoreStateStack(savedState);

    // pop resource stack
    popResources();

    if (softMask) {
        out->setSoftMask(state, bbox, alpha, transferFunc, backdropColor);
    } else if (transpGroup) {
        out->paintTransparencyGroup(state, bbox);
    }
}

//------------------------------------------------------------------------
// in-line image operators
//------------------------------------------------------------------------

void Gfx::opBeginImage(Object /*args*/[], int /*numArgs*/)
{
    int c1, c2;

    // NB: this function is run even if ocState is false -- doImage() is
    // responsible for skipping over the inline image data

    // build dict/stream
    auto str = buildImageStream();

    // display the image
    if (str) {
        doImage(nullptr, str.get(), true);

        // skip 'EI' tag
        c1 = str->getUndecodedStream()->getChar();
        c2 = str->getUndecodedStream()->getChar();
        while ((c1 != 'E' || c2 != 'I') && c2 != EOF) {
            c1 = c2;
            c2 = str->getUndecodedStream()->getChar();
        }
    }
}

std::unique_ptr<Stream> Gfx::buildImageStream()
{
    // build dictionary
    Object dict(std::make_unique<Dict>(xref));
    Object obj = parser->getObj();
    while (!obj.isCmd("ID") && !obj.isEOF()) {
        if (!obj.isName()) {
            error(errSyntaxError, getPos(), "Inline image dictionary key must be a name object");
        } else {
            auto val = parser->getObj();
            if (val.isEOF() || val.isError()) {
                break;
            }
            dict.dictAdd(obj.getName(), std::move(val));
        }
        obj = parser->getObj();
    }
    if (obj.isEOF()) {
        error(errSyntaxError, getPos(), "End of file in inline image");
        return nullptr;
    }

    // make stream
    if (parser->getStream()) {
        auto str = std::make_unique<EmbedStream>(parser->getStream(), std::move(dict), false, 0, true);
        auto *filterDict = str->getDict();
        return Stream::addFilters(std::move(str), filterDict);
    }
    return nullptr;
}

void Gfx::opImageData(Object /*args*/[], int /*numArgs*/)
{
    error(errInternal, getPos(), "Got 'ID' operator");
}

void Gfx::opEndImage(Object /*args*/[], int /*numArgs*/)
{
    error(errInternal, getPos(), "Got 'EI' operator");
}