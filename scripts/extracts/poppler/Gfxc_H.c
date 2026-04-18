// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 40/41



    if (numArgs == 2 && args[1].isDict()) {
        out->markPoint(args[0].getName(), args[1].getDict());
    } else {
        out->markPoint(args[0].getName());
    }
}

//------------------------------------------------------------------------
// misc
//------------------------------------------------------------------------

struct GfxStackStateSaver
{
    explicit GfxStackStateSaver(Gfx *gfxA) : gfx(gfxA) { gfx->saveState(); }

    ~GfxStackStateSaver() { gfx->restoreState(); }

    GfxStackStateSaver(const GfxStackStateSaver &) = delete;
    GfxStackStateSaver &operator=(const GfxStackStateSaver &) = delete;

    Gfx *const gfx;
};

void Gfx::drawAnnot(Object *str, AnnotBorder *border, AnnotColor *aColor, double xMin, double yMin, double xMax, double yMax, int rotate)
{
    Dict *dict, *resDict;
    double formXMin, formYMin, formXMax, formYMax;
    double x, y, sx, sy, tx, ty;
    GfxColor color;
    int i;

    // this function assumes that we are in the default user space,
    // i.e., baseMatrix = ctm

    // if the bounding box has zero width or height, don't draw anything
    // at all
    if (xMin == xMax || yMin == yMax) {
        return;
    }

    // saves gfx state and automatically restores it on return
    GfxStackStateSaver stackStateSaver(this);

    // Rotation around the topleft corner (for the NoRotate flag)
    if (rotate != 0) {
        const double angle_rad = rotate * std::numbers::pi / 180;
        const double c = cos(angle_rad);
        const double s = sin(angle_rad);

        // (xMin, yMax) is the pivot
        const double unrotateMTX[6] = { +c, -s, +s, +c, -c * xMin - s * yMax + xMin, -c * yMax + s * xMin + yMax };

        state->concatCTM(unrotateMTX[0], unrotateMTX[1], unrotateMTX[2], unrotateMTX[3], unrotateMTX[4], unrotateMTX[5]);
        out->updateCTM(state, unrotateMTX[0], unrotateMTX[1], unrotateMTX[2], unrotateMTX[3], unrotateMTX[4], unrotateMTX[5]);
    }

    // draw the appearance stream (if there is one)
    if (str->isStream()) {

        // get stream dict
        dict = str->streamGetDict();

        std::array<double, 4> bbox;
        std::array<double, 6> m;

        // get the form bounding box
        Object bboxObj = dict->lookup("BBox");
        if (!bboxObj.isArray()) {
            error(errSyntaxError, getPos(), "Bad form bounding box");
            return;
        }
        for (i = 0; i < 4; ++i) {
            Object obj1 = bboxObj.arrayGet(i);
            if (likely(obj1.isNum())) {
                bbox[i] = obj1.getNum();
            } else {
                error(errSyntaxError, getPos(), "Bad form bounding box value");
                return;
            }
        }

        // get the form matrix
        Object matrixObj = dict->lookup("Matrix");
        if (matrixObj.isArrayOfLengthAtLeast(6)) {
            for (i = 0; i < 6; ++i) {
                Object obj1 = matrixObj.arrayGet(i);
                if (likely(obj1.isNum())) {
                    m[i] = obj1.getNum();
                } else {
                    error(errSyntaxError, getPos(), "Bad form matrix");
                    return;
                }
            }
        } else {
            m[0] = 1;
            m[1] = 0;
            m[2] = 0;
            m[3] = 1;
            m[4] = 0;
            m[5] = 0;
        }

        // transform the four corners of the form bbox to default user
        // space, and construct the transformed bbox
        x = bbox[0] * m[0] + bbox[1] * m[2] + m[4];
        y = bbox[0] * m[1] + bbox[1] * m[3] + m[5];
        formXMin = formXMax = x;
        formYMin = formYMax = y;
        x = bbox[0] * m[0] + bbox[3] * m[2] + m[4];
        y = bbox[0] * m[1] + bbox[3] * m[3] + m[5];
        if (x < formXMin) {
            formXMin = x;
        } else if (x > formXMax) {
            formXMax = x;
        }
        if (y < formYMin) {
            formYMin = y;
        } else if (y > formYMax) {
            formYMax = y;
        }
        x = bbox[2] * m[0] + bbox[1] * m[2] + m[4];
        y = bbox[2] * m[1] + bbox[1] * m[3] + m[5];
        if (x < formXMin) {
            formXMin = x;
        } else if (x > formXMax) {
            formXMax = x;
        }
        if (y < formYMin) {
            formYMin = y;
        } else if (y > formYMax) {
            formYMax = y;
        }
        x = bbox[2] * m[0] + bbox[3] * m[2] + m[4];
        y = bbox[2] * m[1] + bbox[3] * m[3] + m[5];
        if (x < formXMin) {
            formXMin = x;
        } else if (x > formXMax) {
            formXMax = x;
        }
        if (y < formYMin) {
            formYMin = y;
        } else if (y > formYMax) {
            formYMax = y;
        }