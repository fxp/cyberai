// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 46/49



    defaultGrayColorSpace = nullptr;
    defaultRGBColorSpace = nullptr;
    defaultCMYKColorSpace = nullptr;
#if USE_CMS
    localDisplayProfile = nullptr;
    XYZ2DisplayTransforms = std::make_shared<GfxXYZ2DisplayTransforms>(nullptr);

    if (!sRGBProfile) {
        // This is probably the one of the first invocations of lcms2, so we set the error handler
        setCMSErrorHandler();

        sRGBProfile = make_GfxLCMSProfilePtr(cmsCreate_sRGBProfile());
    }
#endif
}

GfxState::~GfxState()
{
    delete path;
}

// Used for copy();
GfxState::GfxState(const GfxState *state, bool copyPath)
{
    hDPI = state->hDPI;
    vDPI = state->vDPI;
    ctm = state->ctm;
    px1 = state->px1;
    py1 = state->py1;
    px2 = state->px2;
    py2 = state->py2;
    pageWidth = state->pageWidth;
    pageHeight = state->pageHeight;
    rotate = state->rotate;

    if (state->fillColorSpace) {
        fillColorSpace = state->fillColorSpace->copy();
    }
    if (state->strokeColorSpace) {
        strokeColorSpace = state->strokeColorSpace->copy();
    }
    fillColor = state->fillColor;
    strokeColor = state->strokeColor;

    if (state->fillPattern) {
        fillPattern = state->fillPattern->copy();
    }
    if (state->strokePattern) {
        strokePattern = state->strokePattern->copy();
    }
    blendMode = state->blendMode;
    fillOpacity = state->fillOpacity;
    strokeOpacity = state->strokeOpacity;
    fillOverprint = state->fillOverprint;
    strokeOverprint = state->strokeOverprint;
    overprintMode = state->overprintMode;
    transfer.reserve(state->transfer.size());
    for (const auto &element : state->transfer) {
        transfer.push_back(element->copy());
    }
    lineWidth = state->lineWidth;
    lineDash = state->lineDash;
    lineDashStart = state->lineDashStart;
    flatness = state->flatness;
    lineJoin = state->lineJoin;
    lineCap = state->lineCap;
    miterLimit = state->miterLimit;
    strokeAdjust = state->strokeAdjust;
    alphaIsShape = state->alphaIsShape;
    textKnockout = state->textKnockout;

    font = state->font;
    fontSize = state->fontSize;
    textMat = state->textMat;
    charSpace = state->charSpace;
    wordSpace = state->wordSpace;
    horizScaling = state->horizScaling;
    leading = state->leading;
    rise = state->rise;
    render = state->render;

    path = state->path;
    if (copyPath) {
        path = state->path->copy();
    }
    curX = state->curX;
    curY = state->curY;
    curTextX = state->curTextX;
    curTextY = state->curTextY;
    lineX = state->lineX;
    lineY = state->lineY;

    clipXMin = state->clipXMin;
    clipYMin = state->clipYMin;
    clipXMax = state->clipXMax;
    clipYMax = state->clipYMax;
    memcpy(renderingIntent, state->renderingIntent, sizeof(renderingIntent));

    saved = nullptr;
#if USE_CMS
    localDisplayProfile = state->localDisplayProfile;
    XYZ2DisplayTransforms = state->XYZ2DisplayTransforms;
#endif

    if (state->defaultGrayColorSpace) {
        defaultGrayColorSpace = state->defaultGrayColorSpace->copy();
    } else {
        defaultGrayColorSpace = nullptr;
    }
    if (state->defaultRGBColorSpace) {
        defaultRGBColorSpace = state->defaultRGBColorSpace->copy();
    } else {
        defaultRGBColorSpace = nullptr;
    }
    if (state->defaultCMYKColorSpace) {
        defaultCMYKColorSpace = state->defaultCMYKColorSpace->copy();
    } else {
        defaultCMYKColorSpace = nullptr;
    }
}

#if USE_CMS

GfxLCMSProfilePtr GfxState::sRGBProfile = nullptr;

void GfxState::setDisplayProfile(const GfxLCMSProfilePtr &localDisplayProfileA)
{
    localDisplayProfile = localDisplayProfileA;
    XYZ2DisplayTransforms = std::make_shared<GfxXYZ2DisplayTransforms>(localDisplayProfile);
}

void GfxState::setXYZ2DisplayTransforms(std::shared_ptr<GfxXYZ2DisplayTransforms> transforms)
{
    XYZ2DisplayTransforms = std::move(transforms);
    localDisplayProfile = XYZ2DisplayTransforms->getDisplayProfile();
}

std::shared_ptr<GfxColorTransform> GfxState::getXYZ2DisplayTransform()
{
    auto transform = XYZ2DisplayTransforms->getRelCol();
    if (strcmp(renderingIntent, "AbsoluteColorimetric") == 0) {
        transform = XYZ2DisplayTransforms->getAbsCol();
    } else if (strcmp(renderingIntent, "Saturation") == 0) {
        transform = XYZ2DisplayTransforms->getSat();
    } else if (strcmp(renderingIntent, "Perceptual") == 0) {
        transform = XYZ2DisplayTransforms->getPerc();
    }
    return transform;
}

int GfxState::getCmsRenderingIntent() const
{
    const char *intent = getRenderingIntent();
    int cmsIntent = INTENT_RELATIVE_COLORIMETRIC;
    if (intent) {
        if (strcmp(intent, "AbsoluteColorimetric") == 0) {
            cmsIntent = INTENT_ABSOLUTE_COLORIMETRIC;
        } else if (strcmp(intent, "Saturation") == 0) {
            cmsIntent = INTENT_SATURATION;
        } else if (strcmp(intent, "Perceptual") == 0) {
            cmsIntent = INTENT_PERCEPTUAL;
        }
    }
    return cmsIntent;
}

#endif