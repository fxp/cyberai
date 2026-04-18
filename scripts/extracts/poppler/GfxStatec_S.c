// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 19/49



void GfxSeparationColorSpace::getGray(const GfxColor *color, GfxGray *gray) const
{
    double x;
    double c[gfxColorMaxComps];
    GfxColor color2;
    int i;

    if (alt->getMode() == csDeviceGray && name->compare("Black") == 0) {
        *gray = clip01(gfxColorComp1 - color->c[0]);
    } else {
        x = colToDbl(color->c[0]);
        func->transform(&x, c);
        for (i = 0; i < alt->getNComps(); ++i) {
            color2.c[i] = dblToCol(c[i]);
        }
        alt->getGray(&color2, gray);
    }
}

void GfxSeparationColorSpace::getRGB(const GfxColor *color, GfxRGB *rgb) const
{
    double x;
    double c[gfxColorMaxComps];
    GfxColor color2;
    int i;

    if (alt->getMode() == csDeviceGray && name->compare("Black") == 0) {
        rgb->r = clip01(gfxColorComp1 - color->c[0]);
        rgb->g = clip01(gfxColorComp1 - color->c[0]);
        rgb->b = clip01(gfxColorComp1 - color->c[0]);
    } else {
        x = colToDbl(color->c[0]);
        func->transform(&x, c);
        const int altNComps = alt->getNComps();
        for (i = 0; i < altNComps; ++i) {
            color2.c[i] = dblToCol(c[i]);
        }
        alt->getRGB(&color2, rgb);
    }
}

void GfxSeparationColorSpace::getCMYK(const GfxColor *color, GfxCMYK *cmyk) const
{
    double x;
    double c[gfxColorMaxComps];
    GfxColor color2;
    int i;

    if (name->compare("Black") == 0) {
        cmyk->c = 0;
        cmyk->m = 0;
        cmyk->y = 0;
        cmyk->k = color->c[0];
    } else if (name->compare("Cyan") == 0) {
        cmyk->c = color->c[0];
        cmyk->m = 0;
        cmyk->y = 0;
        cmyk->k = 0;
    } else if (name->compare("Magenta") == 0) {
        cmyk->c = 0;
        cmyk->m = color->c[0];
        cmyk->y = 0;
        cmyk->k = 0;
    } else if (name->compare("Yellow") == 0) {
        cmyk->c = 0;
        cmyk->m = 0;
        cmyk->y = color->c[0];
        cmyk->k = 0;
    } else {
        x = colToDbl(color->c[0]);
        func->transform(&x, c);
        for (i = 0; i < alt->getNComps(); ++i) {
            color2.c[i] = dblToCol(c[i]);
        }
        alt->getCMYK(&color2, cmyk);
    }
}

void GfxSeparationColorSpace::getDeviceN(const GfxColor *color, GfxColor *deviceN) const
{
    clearGfxColor(deviceN);
    if (mapping.empty() || mapping[0] == -1) {
        GfxCMYK cmyk;

        getCMYK(color, &cmyk);
        deviceN->c[0] = cmyk.c;
        deviceN->c[1] = cmyk.m;
        deviceN->c[2] = cmyk.y;
        deviceN->c[3] = cmyk.k;
    } else {
        deviceN->c[mapping[0]] = color->c[0];
    }
}

void GfxSeparationColorSpace::getDefaultColor(GfxColor *color) const
{
    color->c[0] = gfxColorComp1;
}

void GfxSeparationColorSpace::createMapping(std::vector<std::unique_ptr<GfxSeparationColorSpace>> *separationList, size_t maxSepComps)
{
    if (nonMarking) {
        return;
    }
    mapping.resize(1);
    switch (overprintMask) {
    case 0x01:
        mapping[0] = 0;
        break;
    case 0x02:
        mapping[0] = 1;
        break;
    case 0x04:
        mapping[0] = 2;
        break;
    case 0x08:
        mapping[0] = 3;
        break;
    default:
        unsigned int newOverprintMask = 0x10;
        for (std::size_t i = 0; i < separationList->size(); i++) {
            const std::unique_ptr<GfxSeparationColorSpace> &sepCS = (*separationList)[i];
            if (!sepCS->getName()->compare(name->toStr())) {
                if (sepCS->getFunc()->hasDifferentResultSet(func.get())) {
                    error(errSyntaxWarning, -1, "Different functions found for '{0:t}', convert immediately", name.get());
                    mapping.clear();
                    return;
                }
                mapping[0] = i + 4;
                overprintMask = newOverprintMask;
                return;
            }
            newOverprintMask <<= 1;
        }
        if (separationList->size() == maxSepComps) {
            error(errSyntaxWarning, -1, "Too many ({0:ulld}) spots, convert '{1:t}' immediately", static_cast<unsigned long long>(maxSepComps), name.get());
            mapping.clear();
            return;
        }
        mapping[0] = separationList->size() + 4;
        separationList->push_back(copyAsOwnType());
        overprintMask = newOverprintMask;
        break;
    }
}

//------------------------------------------------------------------------
// GfxDeviceNColorSpace
//------------------------------------------------------------------------