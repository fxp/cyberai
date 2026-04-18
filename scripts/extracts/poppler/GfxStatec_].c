// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 29/49



std::unique_ptr<GfxRadialShading> GfxRadialShading::parse(GfxResources *res, Dict *dict, OutputDev *out, GfxState *state)
{
    double x0A, y0A, r0A, x1A, y1A, r1A;
    double t0A, t1A;
    std::vector<std::unique_ptr<Function>> funcsA;
    bool extend0A, extend1A;
    Object obj1;
    int i;

    x0A = y0A = r0A = x1A = y1A = r1A = 0;
    obj1 = dict->lookup("Coords");
    if (obj1.isArrayOfLength(6)) {
        x0A = obj1.arrayGet(0).getNumWithDefaultValue(0);
        y0A = obj1.arrayGet(1).getNumWithDefaultValue(0);
        r0A = obj1.arrayGet(2).getNumWithDefaultValue(0);
        x1A = obj1.arrayGet(3).getNumWithDefaultValue(0);
        y1A = obj1.arrayGet(4).getNumWithDefaultValue(0);
        r1A = obj1.arrayGet(5).getNumWithDefaultValue(0);
    } else {
        error(errSyntaxWarning, -1, "Missing or invalid Coords in shading dictionary");
        return {};
    }

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
        if (nFuncsA > gfxColorMaxComps) {
            error(errSyntaxWarning, -1, "Invalid Function array in shading dictionary");
            return {};
        }
        for (i = 0; i < nFuncsA; ++i) {
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
        extend0A = obj1.arrayGet(0).getBoolWithDefaultValue(false);
        extend1A = obj1.arrayGet(1).getBoolWithDefaultValue(false);
    }

    auto shading = std::make_unique<GfxRadialShading>(x0A, y0A, r0A, x1A, y1A, r1A, t0A, t1A, std::move(funcsA), extend0A, extend1A);
    if (!shading->init(res, dict, out, state)) {
        return {};
    }
    return shading;
}

std::unique_ptr<GfxShading> GfxRadialShading::copy() const
{
    return std::make_unique<GfxRadialShading>(this);
}

double GfxRadialShading::getDistance(double sMin, double sMax) const
{
    double xMin, yMin, rMin, xMax, yMax, rMax;

    xMin = x0 + sMin * (x1 - x0);
    yMin = y0 + sMin * (y1 - y0);
    rMin = r0 + sMin * (r1 - r0);

    xMax = x0 + sMax * (x1 - x0);
    yMax = y0 + sMax * (y1 - y0);
    rMax = r0 + sMax * (r1 - r0);

    return hypot(xMax - xMin, yMax - yMin) + fabs(rMax - rMin);
}

// extend range, adapted from cairo, radialExtendRange
static bool radialExtendRange(double range[2], double value, bool valid)
{
    if (!valid) {
        range[0] = range[1] = value;
    } else if (value < range[0]) {
        range[0] = value;
    } else if (value > range[1]) {
        range[1] = value;
    }

    return true;
}

inline void radialEdge(double num, double den, double delta, double lower, double upper, double dr, double mindr, bool &valid, double *range)
{
    if (fabs(den) >= RADIAL_EPSILON) {
        double t_edge, v;
        t_edge = num / den;
        v = t_edge * delta;
        if (t_edge * dr >= mindr && lower <= v && v <= upper) {
            valid = radialExtendRange(range, t_edge, valid);
        }
    }
}

inline void radialCorner1(double x, double y, double &b, double dx, double dy, double cr, double dr, double mindr, bool &valid, double *range)
{
    b = x * dx + y * dy + cr * dr;
    if (fabs(b) >= RADIAL_EPSILON) {
        double t_corner;
        double x2 = x * x;
        double y2 = y * y;
        double cr2 = cr * cr;
        double c = x2 + y2 - cr2;

        t_corner = 0.5 * c / b;
        if (t_corner * dr >= mindr) {
            valid = radialExtendRange(range, t_corner, valid);
        }
    }
}

inline void radialCorner2(double x, double y, double a, double &b, double &c, double &d, double dx, double dy, double cr, double inva, double dr, double mindr, bool &valid, double *range)
{
    b = x * dx + y * dy + cr * dr;
    c = x * x + y * y - cr * cr;
    d = b * b - a * c;
    if (d >= 0) {
        double t_corner;

        d = sqrt(d);
        t_corner = (b + d) * inva;
        if (t_corner * dr >= mindr) {
            valid = radialExtendRange(range, t_corner, valid);
        }
        t_corner = (b - d) * inva;
        if (t_corner * dr >= mindr) {
            valid = radialExtendRange(range, t_corner, valid);
        }
    }
}
void GfxRadialShading::getParameterRange(double *lower, double *upper, double xMin, double yMin, double xMax, double yMax)
{
    double cx, cy, cr, dx, dy, dr;
    double a, x_focus, y_focus;
    double mindr, minx, miny, maxx, maxy;
    double range[2];
    bool valid;