// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 28/49



    t0A = 0;
    t1A = 1;
    obj1 = dict->lookup("Domain");
    if (obj1.isArrayOfLength(2)) {
        t0A = obj1.arrayGet(0).getNumWithDefaultValue(0);
        t1A = obj1.arrayGet(1).getNumWithDefaultValue(1);
    }

    obj1 = dict->lookup("Function");
    if (obj1.isArray()) {
        const int nFuncsA = obj1.arrayGetLength();
        if (nFuncsA > gfxColorMaxComps || nFuncsA == 0) {
            error(errSyntaxWarning, -1, "Invalid Function array in shading dictionary");
            return {};
        }
        for (int i = 0; i < nFuncsA; ++i) {
            Object obj2 = obj1.arrayGet(i);
            std::unique_ptr<Function> f = Function::parse(&obj2);
            if (!f) {
                return {};
            }
            funcsA.emplace_back(std::move(f));
        }
    } else {
        std::unique_ptr<Function> f = Function::parse(&obj1);
        if (!f) {
            return {};
        }
        funcsA.emplace_back(std::move(f));
    }

    extend0A = extend1A = false;
    obj1 = dict->lookup("Extend");
    if (obj1.isArrayOfLength(2)) {
        Object obj2 = obj1.arrayGet(0);
        if (obj2.isBool()) {
            extend0A = obj2.getBool();
        } else {
            error(errSyntaxWarning, -1, "Invalid axial shading extend (0)");
        }
        obj2 = obj1.arrayGet(1);
        if (obj2.isBool()) {
            extend1A = obj2.getBool();
        } else {
            error(errSyntaxWarning, -1, "Invalid axial shading extend (1)");
        }
    }

    auto shading = std::make_unique<GfxAxialShading>(x0A, y0A, x1A, y1A, t0A, t1A, std::move(funcsA), extend0A, extend1A);
    if (!shading->init(res, dict, out, state)) {
        return {};
    }
    return shading;
}

std::unique_ptr<GfxShading> GfxAxialShading::copy() const
{
    return std::make_unique<GfxAxialShading>(this);
}

double GfxAxialShading::getDistance(double sMin, double sMax) const
{
    double xMin, yMin, xMax, yMax;

    xMin = x0 + sMin * (x1 - x0);
    yMin = y0 + sMin * (y1 - y0);
    xMax = x0 + sMax * (x1 - x0);
    yMax = y0 + sMax * (y1 - y0);

    return hypot(xMax - xMin, yMax - yMin);
}

void GfxAxialShading::getParameterRange(double *lower, double *upper, double xMin, double yMin, double xMax, double yMax)
{
    double pdx, pdy, invsqnorm, tdx, tdy, t, range[2];

    // Linear gradients are orthogonal to the line passing through their
    // extremes. Because of convexity, the parameter range can be
    // computed as the convex hull (one the real line) of the parameter
    // values of the 4 corners of the box.
    //
    // The parameter value t for a point (x,y) can be computed as:
    //
    //   t = (p2 - p1) . (x,y) / |p2 - p1|^2
    //
    // t0  is the t value for the top left corner
    // tdx is the difference between left and right corners
    // tdy is the difference between top and bottom corners

    pdx = x1 - x0;
    pdy = y1 - y0;
    const double invsqnorm_denominator = (pdx * pdx + pdy * pdy);
    if (unlikely(invsqnorm_denominator == 0)) {
        *lower = 0;
        *upper = 0;
        return;
    }
    invsqnorm = 1.0 / invsqnorm_denominator;
    pdx *= invsqnorm;
    pdy *= invsqnorm;

    t = (xMin - x0) * pdx + (yMin - y0) * pdy;
    tdx = (xMax - xMin) * pdx;
    tdy = (yMax - yMin) * pdy;

    // Because of the linearity of the t value, tdx can simply be added
    // the t0 to move along the top edge. After this, *lower and *upper
    // represent the parameter range for the top edge, so extending it
    // to include the whole box simply requires adding tdy to the
    // correct extreme.

    range[0] = range[1] = t;
    if (tdx < 0) {
        range[0] += tdx;
    } else {
        range[1] += tdx;
    }

    if (tdy < 0) {
        range[0] += tdy;
    } else {
        range[1] += tdy;
    }

    *lower = std::max<double>(0., std::min<double>(1., range[0]));
    *upper = std::max<double>(0., std::min<double>(1., range[1]));
}

//------------------------------------------------------------------------
// GfxRadialShading
//------------------------------------------------------------------------

#ifndef RADIAL_EPSILON
#    define RADIAL_EPSILON (1. / 1024 / 1024)
#endif

GfxRadialShading::GfxRadialShading(double x0A, double y0A, double r0A, double x1A, double y1A, double r1A, double t0A, double t1A, std::vector<std::unique_ptr<Function>> &&funcsA, bool extend0A, bool extend1A)
    : GfxUnivariateShading(3, t0A, t1A, std::move(funcsA), extend0A, extend1A)
{
    x0 = x0A;
    y0 = y0A;
    r0 = r0A;
    x1 = x1A;
    y1 = y1A;
    r1 = r1A;
}

GfxRadialShading::GfxRadialShading(const GfxRadialShading *shading) : GfxUnivariateShading(shading)
{
    x0 = shading->x0;
    y0 = shading->y0;
    r0 = shading->r0;
    x1 = shading->x1;
    y1 = shading->y1;
    r1 = shading->r1;
}

GfxRadialShading::~GfxRadialShading() = default;