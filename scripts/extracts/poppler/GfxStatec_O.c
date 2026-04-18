// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 47/49



void GfxState::getUserClipBBox(double *xMin, double *yMin, double *xMax, double *yMax) const
{
    double ictm[6];
    double xMin1, yMin1, xMax1, yMax1, tx, ty;

    // invert the CTM
    const double det_denominator = (ctm[0] * ctm[3] - ctm[1] * ctm[2]);
    if (unlikely(det_denominator == 0)) {
        *xMin = 0;
        *yMin = 0;
        *xMax = 0;
        *yMax = 0;
        return;
    }
    const double det = 1 / det_denominator;
    ictm[0] = ctm[3] * det;
    ictm[1] = -ctm[1] * det;
    ictm[2] = -ctm[2] * det;
    ictm[3] = ctm[0] * det;
    ictm[4] = (ctm[2] * ctm[5] - ctm[3] * ctm[4]) * det;
    ictm[5] = (ctm[1] * ctm[4] - ctm[0] * ctm[5]) * det;

    // transform all four corners of the clip bbox; find the min and max
    // x and y values
    xMin1 = xMax1 = clipXMin * ictm[0] + clipYMin * ictm[2] + ictm[4];
    yMin1 = yMax1 = clipXMin * ictm[1] + clipYMin * ictm[3] + ictm[5];
    tx = clipXMin * ictm[0] + clipYMax * ictm[2] + ictm[4];
    ty = clipXMin * ictm[1] + clipYMax * ictm[3] + ictm[5];
    if (tx < xMin1) {
        xMin1 = tx;
    } else if (tx > xMax1) {
        xMax1 = tx;
    }
    if (ty < yMin1) {
        yMin1 = ty;
    } else if (ty > yMax1) {
        yMax1 = ty;
    }
    tx = clipXMax * ictm[0] + clipYMin * ictm[2] + ictm[4];
    ty = clipXMax * ictm[1] + clipYMin * ictm[3] + ictm[5];
    if (tx < xMin1) {
        xMin1 = tx;
    } else if (tx > xMax1) {
        xMax1 = tx;
    }
    if (ty < yMin1) {
        yMin1 = ty;
    } else if (ty > yMax1) {
        yMax1 = ty;
    }
    tx = clipXMax * ictm[0] + clipYMax * ictm[2] + ictm[4];
    ty = clipXMax * ictm[1] + clipYMax * ictm[3] + ictm[5];
    if (tx < xMin1) {
        xMin1 = tx;
    } else if (tx > xMax1) {
        xMax1 = tx;
    }
    if (ty < yMin1) {
        yMin1 = ty;
    } else if (ty > yMax1) {
        yMax1 = ty;
    }

    *xMin = xMin1;
    *yMin = yMin1;
    *xMax = xMax1;
    *yMax = yMax1;
}

double GfxState::transformWidth(double w) const
{
    double x, y;

    x = ctm[0] + ctm[2];
    y = ctm[1] + ctm[3];
    return w * sqrt(0.5 * (x * x + y * y));
}

double GfxState::getTransformedFontSize() const
{
    double x1, y1, x2, y2;

    x1 = textMat[2] * fontSize;
    y1 = textMat[3] * fontSize;
    x2 = ctm[0] * x1 + ctm[2] * y1;
    y2 = ctm[1] * x1 + ctm[3] * y1;
    return sqrt(x2 * x2 + y2 * y2);
}

void GfxState::getFontTransMat(double *m11, double *m12, double *m21, double *m22) const
{
    *m11 = (textMat[0] * ctm[0] + textMat[1] * ctm[2]) * fontSize;
    *m12 = (textMat[0] * ctm[1] + textMat[1] * ctm[3]) * fontSize;
    *m21 = (textMat[2] * ctm[0] + textMat[3] * ctm[2]) * fontSize;
    *m22 = (textMat[2] * ctm[1] + textMat[3] * ctm[3]) * fontSize;
}

void GfxState::setCTM(double a, double b, double c, double d, double e, double f)
{
    ctm[0] = a;
    ctm[1] = b;
    ctm[2] = c;
    ctm[3] = d;
    ctm[4] = e;
    ctm[5] = f;
}

void GfxState::concatCTM(double a, double b, double c, double d, double e, double f)
{
    double a1 = ctm[0];
    double b1 = ctm[1];
    double c1 = ctm[2];
    double d1 = ctm[3];

    ctm[0] = a * a1 + b * c1;
    ctm[1] = a * b1 + b * d1;
    ctm[2] = c * a1 + d * c1;
    ctm[3] = c * b1 + d * d1;
    ctm[4] = e * a1 + f * c1 + ctm[4];
    ctm[5] = e * b1 + f * d1 + ctm[5];
}

void GfxState::shiftCTMAndClip(double tx, double ty)
{
    ctm[4] += tx;
    ctm[5] += ty;
    clipXMin += tx;
    clipYMin += ty;
    clipXMax += tx;
    clipYMax += ty;
}

void GfxState::setFillColorSpace(std::unique_ptr<GfxColorSpace> &&colorSpace)
{
    fillColorSpace = std::move(colorSpace);
}

void GfxState::setStrokeColorSpace(std::unique_ptr<GfxColorSpace> &&colorSpace)
{
    strokeColorSpace = std::move(colorSpace);
}

void GfxState::setFillPattern(std::unique_ptr<GfxPattern> &&pattern)
{
    fillPattern = std::move(pattern);
}

void GfxState::setStrokePattern(std::unique_ptr<GfxPattern> &&pattern)
{
    strokePattern = std::move(pattern);
}

void GfxState::setFont(std::shared_ptr<GfxFont> fontA, double fontSizeA)
{
    font = std::move(fontA);
    fontSize = fontSizeA;
}

void GfxState::setTransfer(std::vector<std::unique_ptr<Function>> funcs)
{
    transfer = std::move(funcs);
}

void GfxState::setLineDash(std::vector<double> &&dash, double start)
{
    lineDash = dash;
    lineDashStart = start;
}

void GfxState::clearPath()
{
    delete path;
    path = new GfxPath();
}

void GfxState::clip()
{
    double xMin, yMin, xMax, yMax, x, y;
    GfxSubpath *subpath;
    int i, j;