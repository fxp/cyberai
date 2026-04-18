// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 18/49



    base->getRGBLine(line, out, length);

    gfree(line);
}

void GfxIndexedColorSpace::getRGBXLine(unsigned char *in, unsigned char *out, int length)
{
    unsigned char *line;
    int i, j, n;

    n = base->getNComps();
    line = (unsigned char *)gmallocn(length, n);
    for (i = 0; i < length; i++) {
        for (j = 0; j < n; j++) {
            line[i * n + j] = lookup[in[i] * n + j];
        }
    }

    base->getRGBXLine(line, out, length);

    gfree(line);
}

void GfxIndexedColorSpace::getCMYKLine(unsigned char *in, unsigned char *out, int length)
{
    unsigned char *line;
    int i, j, n;

    n = base->getNComps();
    line = (unsigned char *)gmallocn(length, n);
    for (i = 0; i < length; i++) {
        for (j = 0; j < n; j++) {
            line[i * n + j] = lookup[in[i] * n + j];
        }
    }

    base->getCMYKLine(line, out, length);

    gfree(line);
}

void GfxIndexedColorSpace::getDeviceNLine(unsigned char *in, unsigned char *out, int length)
{
    unsigned char *line;
    int i, j, n;

    n = base->getNComps();
    line = (unsigned char *)gmallocn(length, n);
    for (i = 0; i < length; i++) {
        for (j = 0; j < n; j++) {
            line[i * n + j] = lookup[in[i] * n + j];
        }
    }

    base->getDeviceNLine(line, out, length);

    gfree(line);
}

void GfxIndexedColorSpace::getCMYK(const GfxColor *color, GfxCMYK *cmyk) const
{
    GfxColor color2;

    base->getCMYK(mapColorToBase(color, &color2), cmyk);
}

void GfxIndexedColorSpace::getDeviceN(const GfxColor *color, GfxColor *deviceN) const
{
    GfxColor color2;

    base->getDeviceN(mapColorToBase(color, &color2), deviceN);
}

void GfxIndexedColorSpace::getDefaultColor(GfxColor *color) const
{
    color->c[0] = 0;
}

void GfxIndexedColorSpace::getDefaultRanges(double *decodeLow, double *decodeRange, int maxImgPixel) const
{
    decodeLow[0] = 0;
    decodeRange[0] = maxImgPixel;
}

//------------------------------------------------------------------------
// GfxSeparationColorSpace
//------------------------------------------------------------------------

GfxSeparationColorSpace::GfxSeparationColorSpace(std::unique_ptr<GooString> &&nameA, std::unique_ptr<GfxColorSpace> &&altA, std::unique_ptr<Function> funcA) : name(std::move(nameA)), alt(std::move(altA))
{
    func = std::move(funcA);
    nonMarking = !name->compare("None");
    if (!name->compare("Cyan")) {
        overprintMask = 0x01;
    } else if (!name->compare("Magenta")) {
        overprintMask = 0x02;
    } else if (!name->compare("Yellow")) {
        overprintMask = 0x04;
    } else if (!name->compare("Black")) {
        overprintMask = 0x08;
    } else if (!name->compare("All")) {
        overprintMask = 0xffffffff;
    }
}

GfxSeparationColorSpace::GfxSeparationColorSpace(std::unique_ptr<GooString> &&nameA, std::unique_ptr<GfxColorSpace> &&altA, std::unique_ptr<Function> funcA, bool nonMarkingA, unsigned int overprintMaskA, const std::vector<int> &mappingA,
                                                 PrivateTag /*unused*/)
    : name(std::move(nameA)), alt(std::move(altA))
{
    func = std::move(funcA);
    nonMarking = nonMarkingA;
    overprintMask = overprintMaskA;
    mapping = mappingA;
}

GfxSeparationColorSpace::~GfxSeparationColorSpace() = default;

std::unique_ptr<GfxColorSpace> GfxSeparationColorSpace::copy() const
{
    return copyAsOwnType();
}

std::unique_ptr<GfxSeparationColorSpace> GfxSeparationColorSpace::copyAsOwnType() const
{
    return std::make_unique<GfxSeparationColorSpace>(name->copy(), alt->copy(), func->copy(), nonMarking, overprintMask, mapping);
}

//~ handle the 'All' and 'None' colorants
std::unique_ptr<GfxColorSpace> GfxSeparationColorSpace::parse(GfxResources *res, const Array &arr, OutputDev *out, GfxState *state, int recursion)
{
    std::unique_ptr<GfxColorSpace> altA;
    std::unique_ptr<Function> funcA;
    Object obj1;

    if (arr.getLength() != 4) {
        error(errSyntaxWarning, -1, "Bad Separation color space");
        return {};
    }
    obj1 = arr.get(1);
    if (!obj1.isName()) {
        error(errSyntaxWarning, -1, "Bad Separation color space (name)");
        return {};
    }
    std::unique_ptr<GooString> nameA = std::make_unique<GooString>(obj1.getNameString());
    obj1 = arr.get(2);
    if (!(altA = GfxColorSpace::parse(res, &obj1, out, state, recursion + 1))) {
        error(errSyntaxWarning, -1, "Bad Separation color space (alternate color space)");
        return {};
    }
    obj1 = arr.get(3);
    if (!(funcA = Function::parse(&obj1))) {
        return {};
    }
    if (funcA->getInputSize() != 1) {
        error(errSyntaxWarning, -1, "Bad SeparationColorSpace function");
        goto err5;
    }
    if (altA->getNComps() <= funcA->getOutputSize()) {
        return std::make_unique<GfxSeparationColorSpace>(std::move(nameA), std::move(altA), std::move(funcA));
    }

err5:
    return {};
}