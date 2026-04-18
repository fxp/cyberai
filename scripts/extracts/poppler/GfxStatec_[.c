// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 27/49



    getParameterRange(&sMin, &sMax, xMin, yMin, xMax, yMax);
    upperBound = ctm->norm() * getDistance(sMin, sMax);
    maxSize = static_cast<int>(ceil(upperBound));
    maxSize = std::max<int>(maxSize, 2);

    {
        double x[4], y[4];

        ctm->transform(xMin, yMin, &x[0], &y[0]);
        ctm->transform(xMax, yMin, &x[1], &y[1]);
        ctm->transform(xMin, yMax, &x[2], &y[2]);
        ctm->transform(xMax, yMax, &x[3], &y[3]);

        xMin = xMax = x[0];
        yMin = yMax = y[0];
        for (i = 1; i < 4; i++) {
            xMin = std::min<double>(xMin, x[i]);
            yMin = std::min<double>(yMin, y[i]);
            xMax = std::max<double>(xMax, x[i]);
            yMax = std::max<double>(yMax, y[i]);
        }
    }

    if (maxSize > (xMax - xMin) * (yMax - yMin)) {
        return;
    }

    if (t0 < t1) {
        tMin = t0 + sMin * (t1 - t0);
        tMax = t0 + sMax * (t1 - t0);
    } else {
        tMin = t0 + sMax * (t1 - t0);
        tMax = t0 + sMin * (t1 - t0);
    }

    cacheBounds = (double *)gmallocn_checkoverflow(maxSize, sizeof(double) * (nComps + 2));
    if (unlikely(!cacheBounds)) {
        return;
    }
    cacheCoeff = cacheBounds + maxSize;
    cacheValues = cacheCoeff + maxSize;

    if (cacheSize != 0) {
        for (j = 0; j < cacheSize; ++j) {
            cacheCoeff[j] = 1 / (cacheBounds[j + 1] - cacheBounds[j]);
        }
    } else if (tMax != tMin) {
        double step = (tMax - tMin) / (maxSize - 1);
        double coeff = (maxSize - 1) / (tMax - tMin);

        cacheSize = maxSize;

        for (j = 0; j < cacheSize; ++j) {
            cacheBounds[j] = tMin + j * step;
            cacheCoeff[j] = coeff;

            for (i = 0; i < nComps; ++i) {
                cacheValues[j * nComps + i] = 0;
            }
            for (i = 0; i < getNFuncs(); ++i) {
                funcs[i]->transform(&cacheBounds[j], &cacheValues[j * nComps + i]);
            }
        }
    }

    lastMatch = 1;
}

bool GfxUnivariateShading::init(GfxResources *res, Dict *dict, OutputDev *out, GfxState *state)
{
    const bool parentInit = GfxShading::init(res, dict, out, state);
    if (!parentInit) {
        return false;
    }

    // funcs needs to be one of the two:
    //  * One function 1-in -> nComps-out
    //  * nComps functions 1-in -> 1-out
    const int nComps = colorSpace->getNComps();
    const int nFuncs = funcs.size();
    if (nFuncs == 1) {
        if (funcs[0]->getInputSize() != 1) {
            error(errSyntaxWarning, -1, "GfxUnivariateShading: function with input size != 2");
            return false;
        }
        if (funcs[0]->getOutputSize() != nComps) {
            error(errSyntaxWarning, -1, "GfxUnivariateShading: function with wrong output size");
            return false;
        }
    } else if (nFuncs == nComps) {
        for (const std::unique_ptr<Function> &f : funcs) {
            if (f->getInputSize() != 1) {
                error(errSyntaxWarning, -1, "GfxUnivariateShading: function with input size != 2");
                return false;
            }
            if (f->getOutputSize() != 1) {
                error(errSyntaxWarning, -1, "GfxUnivariateShading: function with wrong output size");
                return false;
            }
        }
    } else {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------
// GfxAxialShading
//------------------------------------------------------------------------

GfxAxialShading::GfxAxialShading(double x0A, double y0A, double x1A, double y1A, double t0A, double t1A, std::vector<std::unique_ptr<Function>> &&funcsA, bool extend0A, bool extend1A)
    : GfxUnivariateShading(2, t0A, t1A, std::move(funcsA), extend0A, extend1A)
{
    x0 = x0A;
    y0 = y0A;
    x1 = x1A;
    y1 = y1A;
}

GfxAxialShading::GfxAxialShading(const GfxAxialShading *shading) : GfxUnivariateShading(shading)
{
    x0 = shading->x0;
    y0 = shading->y0;
    x1 = shading->x1;
    y1 = shading->y1;
}

GfxAxialShading::~GfxAxialShading() = default;

std::unique_ptr<GfxAxialShading> GfxAxialShading::parse(GfxResources *res, Dict *dict, OutputDev *out, GfxState *state)
{
    double x0A, y0A, x1A, y1A;
    double t0A, t1A;
    std::vector<std::unique_ptr<Function>> funcsA;
    bool extend0A, extend1A;
    Object obj1;

    x0A = y0A = x1A = y1A = 0;
    obj1 = dict->lookup("Coords");
    if (obj1.isArrayOfLength(4)) {
        x0A = obj1.arrayGet(0).getNumWithDefaultValue(0);
        y0A = obj1.arrayGet(1).getNumWithDefaultValue(0);
        x1A = obj1.arrayGet(2).getNumWithDefaultValue(0);
        y1A = obj1.arrayGet(3).getNumWithDefaultValue(0);
    } else {
        error(errSyntaxWarning, -1, "Missing or invalid Coords in shading dictionary");
        return {};
    }