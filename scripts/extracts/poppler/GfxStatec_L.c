// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 44/49



    for (i = 0; i < n; ++i) {
        x[i] += dx;
        y[i] += dy;
    }
}

GfxPath::GfxPath()
{
    justMoved = false;
    size = 16;
    n = 0;
    firstX = firstY = 0;
    subpaths = (GfxSubpath **)gmallocn(size, sizeof(GfxSubpath *));
}

GfxPath::~GfxPath()
{
    int i;

    for (i = 0; i < n; ++i) {
        delete subpaths[i];
    }
    gfree(static_cast<void *>(subpaths));
}

// Used for copy().
GfxPath::GfxPath(bool justMoved1, double firstX1, double firstY1, GfxSubpath **subpaths1, int n1, int size1)
{
    int i;

    justMoved = justMoved1;
    firstX = firstX1;
    firstY = firstY1;
    size = size1;
    n = n1;
    subpaths = (GfxSubpath **)gmallocn(size, sizeof(GfxSubpath *));
    for (i = 0; i < n; ++i) {
        subpaths[i] = subpaths1[i]->copy();
    }
}

void GfxPath::moveTo(double x, double y)
{
    justMoved = true;
    firstX = x;
    firstY = y;
}

void GfxPath::lineTo(double x, double y)
{
    if (justMoved || (n > 0 && subpaths[n - 1]->isClosed())) {
        if (n >= size) {
            size *= 2;
            subpaths = (GfxSubpath **)greallocn(static_cast<void *>(subpaths), size, sizeof(GfxSubpath *));
        }
        if (justMoved) {
            subpaths[n] = new GfxSubpath(firstX, firstY);
        } else {
            subpaths[n] = new GfxSubpath(subpaths[n - 1]->getLastX(), subpaths[n - 1]->getLastY());
        }
        ++n;
        justMoved = false;
    }
    subpaths[n - 1]->lineTo(x, y);
}

void GfxPath::curveTo(double x1, double y1, double x2, double y2, double x3, double y3)
{
    if (justMoved || (n > 0 && subpaths[n - 1]->isClosed())) {
        if (n >= size) {
            size *= 2;
            subpaths = (GfxSubpath **)greallocn(static_cast<void *>(subpaths), size, sizeof(GfxSubpath *));
        }
        if (justMoved) {
            subpaths[n] = new GfxSubpath(firstX, firstY);
        } else {
            subpaths[n] = new GfxSubpath(subpaths[n - 1]->getLastX(), subpaths[n - 1]->getLastY());
        }
        ++n;
        justMoved = false;
    }
    subpaths[n - 1]->curveTo(x1, y1, x2, y2, x3, y3);
}

void GfxPath::close()
{
    // this is necessary to handle the pathological case of
    // moveto/closepath/clip, which defines an empty clipping region
    if (justMoved) {
        if (n >= size) {
            size *= 2;
            subpaths = (GfxSubpath **)greallocn(static_cast<void *>(subpaths), size, sizeof(GfxSubpath *));
        }
        subpaths[n] = new GfxSubpath(firstX, firstY);
        ++n;
        justMoved = false;
    }
    subpaths[n - 1]->close();
}

void GfxPath::append(GfxPath *path)
{
    int i;

    if (n + path->n > size) {
        size = n + path->n;
        subpaths = (GfxSubpath **)greallocn(static_cast<void *>(subpaths), size, sizeof(GfxSubpath *));
    }
    for (i = 0; i < path->n; ++i) {
        subpaths[n++] = path->subpaths[i]->copy();
    }
    justMoved = false;
}

void GfxPath::offset(double dx, double dy)
{
    int i;

    for (i = 0; i < n; ++i) {
        subpaths[i]->offset(dx, dy);
    }
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------

#if USE_CMS

GfxLCMSProfilePtr GfxXYZ2DisplayTransforms::XYZProfile = nullptr;

GfxXYZ2DisplayTransforms::GfxXYZ2DisplayTransforms(const GfxLCMSProfilePtr &displayProfileA)
{
    if (!XYZProfile) {
        // This is probably the one of the first invocations of lcms2, so we set the error handler
        setCMSErrorHandler();

        XYZProfile = make_GfxLCMSProfilePtr(cmsCreateXYZProfile());
    }

    displayProfile = displayProfileA;
    if (displayProfile) {
        cmsHTRANSFORM transform;
        unsigned int nChannels;
        unsigned int localDisplayPixelType;

        localDisplayPixelType = getCMSColorSpaceType(cmsGetColorSpace(displayProfile.get()));
        nChannels = getCMSNChannels(cmsGetColorSpace(displayProfile.get()));
        // create transform from XYZ
        if ((transform = cmsCreateTransform(XYZProfile.get(), TYPE_XYZ_DBL, displayProfile.get(), COLORSPACE_SH(localDisplayPixelType) | CHANNELS_SH(nChannels) | BYTES_SH(1), INTENT_RELATIVE_COLORIMETRIC, LCMS_FLAGS)) == nullptr) {
            error(errSyntaxWarning, -1, "Can't create Lab transform");
        } else {
            XYZ2DisplayTransformRelCol = std::make_shared<GfxColorTransform>(transform, INTENT_RELATIVE_COLORIMETRIC, PT_XYZ, localDisplayPixelType);
        }

        if ((transform = cmsCreateTransform(XYZProfile.get(), TYPE_XYZ_DBL, displayProfile.get(), COLORSPACE_SH(localDisplayPixelType) | CHANNELS_SH(nChannels) | BYTES_SH(1), INTENT_ABSOLUTE_COLORIMETRIC, LCMS_FLAGS)) == nullptr) {
            error(errSyntaxWarning, -1, "Can't create Lab transform");
        } else {
            XYZ2DisplayTransformAbsCol = std::make_shared<GfxColorTransform>(transform, INTENT_ABSOLUTE_COLORIMETRIC, PT_XYZ, localDisplayPixelType);
        }