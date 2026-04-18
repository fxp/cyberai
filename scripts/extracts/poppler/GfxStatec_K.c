// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 43/49



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
        colorSpace->getCMYKLine(in, out, length);
        break;
    }
}

void GfxImageColorMap::getDeviceNLine(unsigned char *in, unsigned char *out, int length)
{
    unsigned char *inp, *tmp_line;

    if (!useDeviceNLine()) {
        GfxColor deviceN;

        inp = in;
        for (int i = 0; i < length; i++) {
            getDeviceN(inp, &deviceN);
            for (int j = 0; j < SPOT_NCOMPS + 4; j++) {
                *out++ = deviceN.c[j];
            }
            inp += nComps;
        }
        return;
    }

    switch (colorSpace->getMode()) {
    case csIndexed:
    case csSeparation:
        tmp_line = (unsigned char *)gmallocn(length, nComps2);
        for (int i = 0; i < length; i++) {
            for (int j = 0; j < nComps2; j++) {
                unsigned char c = in[i];
                if (byte_lookup) {
                    c = byte_lookup[c * nComps2 + j];
                }
                tmp_line[i * nComps2 + j] = c;
            }
        }
        colorSpace2->getDeviceNLine(tmp_line, out, length);
        gfree(tmp_line);
        break;

    default:
        if (byte_lookup) {
            inp = in;
            for (int j = 0; j < length; j++) {
                for (int i = 0; i < nComps; i++) {
                    *inp = byte_lookup[*inp * nComps + i];
                    inp++;
                }
            }
        }
        colorSpace->getDeviceNLine(in, out, length);
        break;
    }
}

void GfxImageColorMap::getCMYK(const unsigned char *x, GfxCMYK *cmyk)
{
    GfxColor color;
    int i;

    if (colorSpace2) {
        for (i = 0; i < nComps2; ++i) {
            color.c[i] = lookup2[i][x[0]];
        }
        colorSpace2->getCMYK(&color, cmyk);
    } else {
        for (i = 0; i < nComps; ++i) {
            color.c[i] = lookup[i][x[i]];
        }
        colorSpace->getCMYK(&color, cmyk);
    }
}

void GfxImageColorMap::getDeviceN(const unsigned char *x, GfxColor *deviceN)
{
    GfxColor color;
    int i;

    if (colorSpace2 && (colorSpace->getMapping().empty() || colorSpace->getMapping()[0] == -1)) {
        for (i = 0; i < nComps2; ++i) {
            color.c[i] = lookup2[i][x[0]];
        }
        colorSpace2->getDeviceN(&color, deviceN);
    } else {
        for (i = 0; i < nComps; ++i) {
            color.c[i] = lookup[i][x[i]];
        }
        colorSpace->getDeviceN(&color, deviceN);
    }
}

void GfxImageColorMap::getColor(const unsigned char *x, GfxColor *color)
{
    int maxPixel, i;

    maxPixel = (1 << bits) - 1;
    for (i = 0; i < nComps; ++i) {
        color->c[i] = dblToCol(decodeLow[i] + (x[i] * decodeRange[i]) / maxPixel);
    }
}

//------------------------------------------------------------------------
// GfxSubpath and GfxPath
//------------------------------------------------------------------------

GfxSubpath::GfxSubpath(double x1, double y1)
{
    size = 16;
    x = (double *)gmallocn(size, sizeof(double));
    y = (double *)gmallocn(size, sizeof(double));
    curve = (bool *)gmallocn(size, sizeof(bool));
    n = 1;
    x[0] = x1;
    y[0] = y1;
    curve[0] = false;
    closed = false;
}

GfxSubpath::~GfxSubpath()
{
    gfree(x);
    gfree(y);
    gfree(curve);
}

// Used for copy().
GfxSubpath::GfxSubpath(const GfxSubpath *subpath)
{
    size = subpath->size;
    n = subpath->n;
    x = (double *)gmallocn(size, sizeof(double));
    y = (double *)gmallocn(size, sizeof(double));
    curve = (bool *)gmallocn(size, sizeof(bool));
    memcpy(x, subpath->x, n * sizeof(double));
    memcpy(y, subpath->y, n * sizeof(double));
    memcpy(curve, subpath->curve, n * sizeof(bool));
    closed = subpath->closed;
}

void GfxSubpath::lineTo(double x1, double y1)
{
    if (n >= size) {
        size *= 2;
        x = (double *)greallocn(x, size, sizeof(double));
        y = (double *)greallocn(y, size, sizeof(double));
        curve = (bool *)greallocn(curve, size, sizeof(bool));
    }
    x[n] = x1;
    y[n] = y1;
    curve[n] = false;
    ++n;
}

void GfxSubpath::curveTo(double x1, double y1, double x2, double y2, double x3, double y3)
{
    if (n + 3 > size) {
        size *= 2;
        x = (double *)greallocn(x, size, sizeof(double));
        y = (double *)greallocn(y, size, sizeof(double));
        curve = (bool *)greallocn(curve, size, sizeof(bool));
    }
    x[n] = x1;
    y[n] = y1;
    x[n + 1] = x2;
    y[n + 1] = y2;
    x[n + 2] = x3;
    y[n + 2] = y3;
    curve[n] = curve[n + 1] = true;
    curve[n + 2] = false;
    n += 3;
}

void GfxSubpath::close()
{
    if (x[n - 1] != x[0] || y[n - 1] != y[0]) {
        lineTo(x[0], y[0]);
    }
    closed = true;
}

void GfxSubpath::offset(double dx, double dy)
{
    int i;