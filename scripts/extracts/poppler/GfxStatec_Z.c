// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 26/49



    // funcs needs to be one of the two:
    //  * One function 2-in -> nComps-out
    //  * nComps functions 2-in -> 1-out
    const int nComps = colorSpace->getNComps();
    const int nFuncs = funcs.size();
    if (nFuncs == 1) {
        if (funcs[0]->getInputSize() != 2) {
            error(errSyntaxWarning, -1, "GfxFunctionShading: function with input size != 2");
            return false;
        }
        if (funcs[0]->getOutputSize() != nComps) {
            error(errSyntaxWarning, -1, "GfxFunctionShading: function with wrong output size");
            return false;
        }
    } else if (nFuncs == nComps) {
        for (const std::unique_ptr<Function> &f : funcs) {
            if (f->getInputSize() != 2) {
                error(errSyntaxWarning, -1, "GfxFunctionShading: function with input size != 2");
                return false;
            }
            if (f->getOutputSize() != 1) {
                error(errSyntaxWarning, -1, "GfxFunctionShading: function with wrong output size");
                return false;
            }
        }
    } else {
        return false;
    }

    return true;
}

std::unique_ptr<GfxShading> GfxFunctionShading::copy() const
{
    return std::make_unique<GfxFunctionShading>(this);
}

void GfxFunctionShading::getColor(double x, double y, GfxColor *color) const
{
    double in[2], out[gfxColorMaxComps];

    // NB: there can be one function with n outputs or n functions with
    // one output each (where n = number of color components)
    for (double &i : out) {
        i = 0;
    }
    in[0] = x;
    in[1] = y;
    for (int i = 0; i < getNFuncs(); ++i) {
        funcs[i]->transform(in, &out[i]);
    }
    for (int i = 0; i < gfxColorMaxComps; ++i) {
        color->c[i] = dblToCol(out[i]);
    }
}

//------------------------------------------------------------------------
// GfxUnivariateShading
//------------------------------------------------------------------------

GfxUnivariateShading::GfxUnivariateShading(int typeA, double t0A, double t1A, std::vector<std::unique_ptr<Function>> &&funcsA, bool extend0A, bool extend1A) : GfxShading(typeA), funcs(std::move(funcsA))
{
    t0 = t0A;
    t1 = t1A;
    extend0 = extend0A;
    extend1 = extend1A;

    cacheSize = 0;
    lastMatch = 0;
    cacheBounds = nullptr;
    cacheCoeff = nullptr;
    cacheValues = nullptr;
}

GfxUnivariateShading::GfxUnivariateShading(const GfxUnivariateShading *shading) : GfxShading(shading)
{
    t0 = shading->t0;
    t1 = shading->t1;
    for (const auto &f : shading->funcs) {
        funcs.emplace_back(f->copy());
    }
    extend0 = shading->extend0;
    extend1 = shading->extend1;

    cacheSize = 0;
    lastMatch = 0;
    cacheBounds = nullptr;
    cacheCoeff = nullptr;
    cacheValues = nullptr;
}

GfxUnivariateShading::~GfxUnivariateShading()
{
    gfree(cacheBounds);
}

int GfxUnivariateShading::getColor(double t, GfxColor *color)
{
    double out[gfxColorMaxComps];

    // NB: there can be one function with n outputs or n functions with
    // one output each (where n = number of color components)
    const int nComps = getNFuncs() * funcs[0]->getOutputSize();

    if (cacheSize > 0) {
        double x, ix, *l, *u, *upper;

        if (cacheBounds[lastMatch - 1] >= t) {
            upper = std::lower_bound(cacheBounds, cacheBounds + lastMatch - 1, t);
            lastMatch = static_cast<int>(upper - cacheBounds);
            lastMatch = std::min<int>(std::max<int>(1, lastMatch), cacheSize - 1);
        } else if (cacheBounds[lastMatch] < t) {
            upper = std::lower_bound(cacheBounds + lastMatch + 1, cacheBounds + cacheSize, t);
            lastMatch = static_cast<int>(upper - cacheBounds);
            lastMatch = std::min<int>(std::max<int>(1, lastMatch), cacheSize - 1);
        }

        x = (t - cacheBounds[lastMatch - 1]) * cacheCoeff[lastMatch];
        ix = 1.0 - x;
        u = cacheValues + lastMatch * nComps;
        l = u - nComps;

        for (int i = 0; i < nComps; ++i) {
            out[i] = ix * l[i] + x * u[i];
        }
    } else {
        for (int i = 0; i < nComps; ++i) {
            out[i] = 0;
        }
        for (int i = 0; i < getNFuncs(); ++i) {
            funcs[i]->transform(&t, &out[i]);
        }
    }

    for (int i = 0; i < nComps; ++i) {
        color->c[i] = dblToCol(out[i]);
    }
    return nComps;
}

void GfxUnivariateShading::setupCache(const Matrix *ctm, double xMin, double yMin, double xMax, double yMax)
{
    double sMin, sMax, tMin, tMax, upperBound;
    int i, j, nComps, maxSize;

    gfree(cacheBounds);
    cacheBounds = nullptr;
    cacheSize = 0;

    if (unlikely(getNFuncs() < 1)) {
        return;
    }

    // NB: there can be one function with n outputs or n functions with
    // one output each (where n = number of color components)
    nComps = getNFuncs() * funcs[0]->getOutputSize();