// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 31/49



        // Ensure that gradients with both a and dr small are
        // considered degenerate.
        // The floating point version of the degeneracy test implemented
        // in _radial_pattern_is_degenerate() is:
        //
        //  1) The circles are practically the same size:
        //     |dr| < RADIAL_EPSILON
        //  AND
        //  2a) The circles are both very small:
        //      min (r0, r1) < RADIAL_EPSILON
        //   OR
        //  2b) The circles are very close to each other:
        //      max (|dx|, |dy|) < 2 * RADIAL_EPSILON
        //
        // Assuming that the gradient is not degenerate, we want to
        // show that |a| < RADIAL_EPSILON^2 implies |dr| >= RADIAL_EPSILON.
        //
        // If the gradient is not degenerate yet it has |dr| <
        // RADIAL_EPSILON, (2b) is false, thus:
        //
        //   max (|dx|, |dy|) >= 2*RADIAL_EPSILON
        // which implies:
        //   4*RADIAL_EPSILON^2 <= max (|dx|, |dy|)^2 <= dx^2 + dy^2
        //
        // From the definition of a, we get:
        //   a = dx^2 + dy^2 - dr^2 < RADIAL_EPSILON^2
        //   dx^2 + dy^2 - RADIAL_EPSILON^2 < dr^2
        //   3*RADIAL_EPSILON^2 < dr^2
        //
        // which is inconsistent with the hypotheses, thus |dr| <
        // RADIAL_EPSILON is false or the gradient is degenerate.

        assert(fabs(dr) >= RADIAL_EPSILON);

        // If a == 0, all the circles are tangent to a line in the
        // focus point. If this line is within the box extents, we
        // should add the circle with infinite radius, but this would
        // make the range unbounded. We will be limiting the range to
        // [0,1] anyway, so we simply add the biggest legitimate
        // circle (it happens for 0 or for 1).
        if (dr < 0) {
            valid = radialExtendRange(range, 0, valid);
        } else {
            valid = radialExtendRange(range, 1, valid);
        }

        // Nondegenerate, nonlimit circles passing through the corners.
        //
        // a == 0 && a*t^2 - 2*b*t + c == 0
        //
        // t = c / (2*b)
        //
        // The b == 0 case has just been handled, so we only have to
        // compute this if b != 0.

        // circles touching each corner
        radialCorner1(xMin, yMin, b, dx, dy, cr, dr, mindr, valid, range);
        radialCorner1(xMin, yMax, b, dx, dy, cr, dr, mindr, valid, range);
        radialCorner1(xMax, yMin, b, dx, dy, cr, dr, mindr, valid, range);
        radialCorner1(xMax, yMax, b, dx, dy, cr, dr, mindr, valid, range);
    } else {
        double inva, b, c, d;

        inva = 1 / a;

        // Nondegenerate, nonlimit circles passing through the corners.
        //
        // a != 0 && a*t^2 - 2*b*t + c == 0
        //
        // t = (b +- sqrt (b*b - a*c)) / a
        //
        // If the argument of sqrt() is negative, then no circle
        // passes through the corner.

        // circles touching each corner
        radialCorner2(xMin, yMin, a, b, c, d, dx, dy, cr, inva, dr, mindr, valid, range);
        radialCorner2(xMin, yMax, a, b, c, d, dx, dy, cr, inva, dr, mindr, valid, range);
        radialCorner2(xMax, yMin, a, b, c, d, dx, dy, cr, inva, dr, mindr, valid, range);
        radialCorner2(xMax, yMax, a, b, c, d, dx, dy, cr, inva, dr, mindr, valid, range);
    }

    *lower = std::max<double>(0., std::min<double>(1., range[0]));
    *upper = std::max<double>(0., std::min<double>(1., range[1]));
}

//------------------------------------------------------------------------
// GfxShadingBitBuf
//------------------------------------------------------------------------

class GfxShadingBitBuf
{
public:
    explicit GfxShadingBitBuf(Stream *strA);
    ~GfxShadingBitBuf();
    GfxShadingBitBuf(const GfxShadingBitBuf &) = delete;
    GfxShadingBitBuf &operator=(const GfxShadingBitBuf &) = delete;
    bool getBits(int n, unsigned int *val);
    void flushBits();

private:
    Stream *str;
    int bitBuf;
    int nBits;
};

GfxShadingBitBuf::GfxShadingBitBuf(Stream *strA)
{
    str = strA;
    (void)str->rewind();
    bitBuf = 0;
    nBits = 0;
}

GfxShadingBitBuf::~GfxShadingBitBuf()
{
    str->close();
}

bool GfxShadingBitBuf::getBits(int n, unsigned int *val)
{
    unsigned int x;

    if (nBits >= n) {
        x = (bitBuf >> (nBits - n)) & ((1 << n) - 1);
        nBits -= n;
    } else {
        x = 0;
        if (nBits > 0) {
            x = bitBuf & ((1 << nBits) - 1);
            n -= nBits;
            nBits = 0;
        }
        while (n > 0) {
            if ((bitBuf = str->getChar()) == EOF) {
                nBits = 0;
                return false;
            }
            if (n >= 8) {
                x = (x << 8) | bitBuf;
                n -= 8;
            } else {
                x = (x << n) | (bitBuf >> (8 - n));
                nBits = 8 - n;
                n = 0;
            }
        }
    }
    *val = x;
    return true;
}