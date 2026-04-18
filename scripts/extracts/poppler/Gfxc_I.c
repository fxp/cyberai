// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 41/41



        // construct a mapping matrix, [sx 0  0], which maps the transformed
        //                             [0  sy 0]
        //                             [tx ty 1]
        // bbox to the annotation rectangle
        if (formXMin == formXMax) {
            // this shouldn't happen
            sx = 1;
        } else {
            sx = (xMax - xMin) / (formXMax - formXMin);
        }
        if (formYMin == formYMax) {
            // this shouldn't happen
            sy = 1;
        } else {
            sy = (yMax - yMin) / (formYMax - formYMin);
        }
        tx = -formXMin * sx + xMin;
        ty = -formYMin * sy + yMin;

        // the final transform matrix is (form matrix) * (mapping matrix)
        m[0] *= sx;
        m[1] *= sy;
        m[2] *= sx;
        m[3] *= sy;
        m[4] = m[4] * sx + tx;
        m[5] = m[5] * sy + ty;

        // get the resources
        Object resObj = dict->lookup("Resources");
        resDict = resObj.isDict() ? resObj.getDict() : nullptr;

        // draw it
        drawForm(str, resDict, m, bbox);
    }

    // draw the border
    if (border && border->getWidth() > 0 && (!aColor || aColor->getSpace() != AnnotColor::colorTransparent)) {
        if (state->getStrokeColorSpace()->getMode() != csDeviceRGB) {
            state->setStrokePattern(nullptr);
            state->setStrokeColorSpace(std::make_unique<GfxDeviceRGBColorSpace>());
            out->updateStrokeColorSpace(state);
        }
        double r, g, b;
        if (!aColor) {
            r = g = b = 0;
        } else if ((aColor->getSpace() == AnnotColor::colorRGB)) {
            const std::array<double, 4> &values = aColor->getValues();
            r = values[0];
            g = values[1];
            b = values[2];
        } else {
            error(errUnimplemented, -1, "AnnotColor different than RGB and Transparent not supported");
            r = g = b = 0;
        };
        color.c[0] = dblToCol(r);
        color.c[1] = dblToCol(g);
        color.c[2] = dblToCol(b);
        state->setStrokeColor(&color);
        out->updateStrokeColor(state);
        state->setLineWidth(border->getWidth());
        out->updateLineWidth(state);
        const std::vector<double> &dash = border->getDash();
        if (border->getStyle() == AnnotBorder::borderDashed && !dash.empty()) {
            std::vector<double> dash2 = dash;
            state->setLineDash(std::move(dash2), 0);
            out->updateLineDash(state);
        }
        //~ this doesn't currently handle the beveled and engraved styles
        state->clearPath();
        state->moveTo(xMin, yMin);
        state->lineTo(xMax, yMin);
        if (border->getStyle() != AnnotBorder::borderUnderlined) {
            state->lineTo(xMax, yMax);
            state->lineTo(xMin, yMax);
            state->closePath();
        }
        out->stroke(state);
    }
}

int Gfx::bottomGuard()
{
    return stateGuards[stateGuards.size() - 1];
}

void Gfx::pushStateGuard()
{
    stateGuards.push_back(stackHeight);
}

void Gfx::popStateGuard()
{
    while (stackHeight > bottomGuard() && state->hasSaves()) {
        restoreState();
    }
    stateGuards.pop_back();
}

void Gfx::saveState()
{
    out->saveState(state);
    state = state->save();
    stackHeight++;
}

void Gfx::restoreState()
{
    if (stackHeight <= bottomGuard() || !state->hasSaves()) {
        error(errSyntaxError, -1, "Restoring state when no valid states to pop");
        return;
    }
    state = state->restore();
    out->restoreState(state);
    stackHeight--;
    clip = clipNone;
}

// Create a new state stack, and initialize it with a copy of the
// current state.
GfxState *Gfx::saveStateStack()
{
    GfxState *oldState;

    out->saveState(state);
    oldState = state;
    state = state->copy(true);
    return oldState;
}

// Switch back to the previous state stack.
void Gfx::restoreStateStack(GfxState *oldState)
{
    while (state->hasSaves()) {
        restoreState();
    }
    delete state;
    state = oldState;
    out->restoreState(state);
}

void Gfx::pushResources(Dict *resDict)
{
    res = new GfxResources(xref, resDict, res);
}

void Gfx::popResources()
{
    GfxResources *resPtr;

    resPtr = res->getNext();
    delete res;
    res = resPtr;
}
