// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 41/49



                    byte = (int)(mapped * 255.0 + 0.5);
                    if (byte < 0) {
                        byte = 0;
                    } else if (byte > 255) {
                        byte = 255;
                    }
                    byte_lookup[i * nComps + k] = byte;
                }
            }
        }
    }

    return;

err1:
    ok = false;
}

GfxImageColorMap::GfxImageColorMap(const GfxImageColorMap *colorMap)
{
    int n, i, k;

    colorSpace = colorMap->colorSpace->copy();
    bits = colorMap->bits;
    nComps = colorMap->nComps;
    nComps2 = colorMap->nComps2;
    useMatte = colorMap->useMatte;
    matteColor = colorMap->matteColor;
    colorSpace2 = nullptr;
    for (k = 0; k < gfxColorMaxComps; ++k) {
        lookup[k] = nullptr;
        lookup2[k] = nullptr;
    }
    byte_lookup = nullptr;
    n = 1 << bits;
    for (k = 0; k < nComps; ++k) {
        lookup[k] = (GfxColorComp *)gmallocn(n, sizeof(GfxColorComp));
        memcpy(lookup[k], colorMap->lookup[k], n * sizeof(GfxColorComp));
    }
    if (colorSpace->getMode() == csIndexed) {
        colorSpace2 = ((GfxIndexedColorSpace *)colorSpace.get())->getBase();
        for (k = 0; k < nComps2; ++k) {
            lookup2[k] = (GfxColorComp *)gmallocn(n, sizeof(GfxColorComp));
            memcpy(lookup2[k], colorMap->lookup2[k], n * sizeof(GfxColorComp));
        }
    } else if (colorSpace->getMode() == csSeparation) {
        colorSpace2 = ((GfxSeparationColorSpace *)colorSpace.get())->getAlt();
        for (k = 0; k < nComps2; ++k) {
            lookup2[k] = (GfxColorComp *)gmallocn(n, sizeof(GfxColorComp));
            memcpy(lookup2[k], colorMap->lookup2[k], n * sizeof(GfxColorComp));
        }
    } else {
        for (k = 0; k < nComps; ++k) {
            lookup2[k] = (GfxColorComp *)gmallocn(n, sizeof(GfxColorComp));
            memcpy(lookup2[k], colorMap->lookup2[k], n * sizeof(GfxColorComp));
        }
    }
    if (colorMap->byte_lookup) {
        int nc = colorSpace2 ? nComps2 : nComps;

        byte_lookup = (unsigned char *)gmallocn(n, nc);
        memcpy(byte_lookup, colorMap->byte_lookup, n * nc);
    }
    for (i = 0; i < nComps; ++i) {
        decodeLow[i] = colorMap->decodeLow[i];
        decodeRange[i] = colorMap->decodeRange[i];
    }
    ok = true;
}

GfxImageColorMap::~GfxImageColorMap()
{
    int i;

    for (i = 0; i < gfxColorMaxComps; ++i) {
        gfree(lookup[i]);
        gfree(lookup2[i]);
    }
    gfree(byte_lookup);
}

void GfxImageColorMap::getGray(const unsigned char *x, GfxGray *gray)
{
    GfxColor color;
    int i;

    if (colorSpace2) {
        for (i = 0; i < nComps2; ++i) {
            color.c[i] = lookup2[i][x[0]];
        }
        colorSpace2->getGray(&color, gray);
    } else {
        for (i = 0; i < nComps; ++i) {
            color.c[i] = lookup2[i][x[i]];
        }
        colorSpace->getGray(&color, gray);
    }
}

void GfxImageColorMap::getRGB(const unsigned char *x, GfxRGB *rgb)
{
    GfxColor color;
    int i;

    if (colorSpace2) {
        for (i = 0; i < nComps2; ++i) {
            color.c[i] = lookup2[i][x[0]];
        }
        colorSpace2->getRGB(&color, rgb);
    } else {
        for (i = 0; i < nComps; ++i) {
            color.c[i] = lookup2[i][x[i]];
        }
        colorSpace->getRGB(&color, rgb);
    }
}

void GfxImageColorMap::getGrayLine(unsigned char *in, unsigned char *out, int length)
{
    int i, j;
    unsigned char *inp, *tmp_line;

    if ((colorSpace2 && !colorSpace2->useGetGrayLine()) || (!colorSpace2 && !colorSpace->useGetGrayLine())) {
        GfxGray gray;

        inp = in;
        for (i = 0; i < length; i++) {
            getGray(inp, &gray);
            out[i] = colToByte(gray);
            inp += nComps;
        }
        return;
    }

    switch (colorSpace->getMode()) {
    case csIndexed:
    case csSeparation:
        tmp_line = (unsigned char *)gmallocn(length, nComps2);
        for (i = 0; i < length; i++) {
            for (j = 0; j < nComps2; j++) {
                unsigned char c = in[i];
                if (byte_lookup) {
                    c = byte_lookup[c * nComps2 + j];
                }
                tmp_line[i * nComps2 + j] = c;
            }
        }
        colorSpace2->getGrayLine(tmp_line, out, length);
        gfree(tmp_line);
        break;

    default:
        if (byte_lookup) {
            inp = in;
            for (j = 0; j < length; j++) {
                for (i = 0; i < nComps; i++) {
                    *inp = byte_lookup[*inp * nComps + i];
                    inp++;
                }
            }
        }
        colorSpace->getGrayLine(in, out, length);
        break;
    }
}

void GfxImageColorMap::getRGBLine(unsigned char *in, unsigned int *out, int length)
{
    int i, j;
    unsigned char *inp, *tmp_line;

    if (!useRGBLine()) {
        GfxRGB rgb;