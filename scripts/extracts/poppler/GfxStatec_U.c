// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 21/49



    if (likely(nCompsA >= funcA->getInputSize() && altA->getNComps() <= funcA->getOutputSize())) {
        return std::make_unique<GfxDeviceNColorSpace>(nCompsA, std::move(namesA), std::move(altA), std::move(funcA), std::move(separationList));
    }
    return nullptr;
}

void GfxDeviceNColorSpace::getGray(const GfxColor *color, GfxGray *gray) const
{
    double x[gfxColorMaxComps], c[gfxColorMaxComps];
    GfxColor color2;
    int i;

    for (i = 0; i < nComps; ++i) {
        x[i] = colToDbl(color->c[i]);
    }
    func->transform(x, c);
    for (i = 0; i < alt->getNComps(); ++i) {
        color2.c[i] = dblToCol(c[i]);
    }
    alt->getGray(&color2, gray);
}

void GfxDeviceNColorSpace::getRGB(const GfxColor *color, GfxRGB *rgb) const
{
    double x[gfxColorMaxComps], c[gfxColorMaxComps];
    GfxColor color2;
    int i;

    for (i = 0; i < nComps; ++i) {
        x[i] = colToDbl(color->c[i]);
    }
    func->transform(x, c);
    for (i = 0; i < alt->getNComps(); ++i) {
        color2.c[i] = dblToCol(c[i]);
    }
    alt->getRGB(&color2, rgb);
}

void GfxDeviceNColorSpace::getCMYK(const GfxColor *color, GfxCMYK *cmyk) const
{
    double x[gfxColorMaxComps], c[gfxColorMaxComps];
    GfxColor color2;
    int i;

    for (i = 0; i < nComps; ++i) {
        x[i] = colToDbl(color->c[i]);
    }
    func->transform(x, c);
    for (i = 0; i < alt->getNComps(); ++i) {
        color2.c[i] = dblToCol(c[i]);
    }
    alt->getCMYK(&color2, cmyk);
}

void GfxDeviceNColorSpace::getDeviceN(const GfxColor *color, GfxColor *deviceN) const
{
    clearGfxColor(deviceN);
    if (mapping.empty()) {
        GfxCMYK cmyk;

        getCMYK(color, &cmyk);
        deviceN->c[0] = cmyk.c;
        deviceN->c[1] = cmyk.m;
        deviceN->c[2] = cmyk.y;
        deviceN->c[3] = cmyk.k;
    } else {
        for (int j = 0; j < nComps; j++) {
            if (mapping[j] != -1) {
                deviceN->c[mapping[j]] = color->c[j];
            }
        }
    }
}

void GfxDeviceNColorSpace::getDefaultColor(GfxColor *color) const
{
    int i;

    for (i = 0; i < nComps; ++i) {
        color->c[i] = gfxColorComp1;
    }
}