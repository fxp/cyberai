// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 39/49

        p->color[0][1].c[j] = patchesA[nPatchesA - 1].color[0][0].c[j];
                    p->color[1][1].c[j] = c[0][j];
                    p->color[1][0].c[j] = c[1][j];
                }
                break;
            }
        }
        ++nPatchesA;
        bitBuf->flushBits();
    }

    if (typeA == 6) {
        for (i = 0; i < nPatchesA; ++i) {
            p = &patchesA[i];
            p->x[1][1] = (-4 * p->x[0][0] + 6 * (p->x[0][1] + p->x[1][0]) - 2 * (p->x[0][3] + p->x[3][0]) + 3 * (p->x[3][1] + p->x[1][3]) - p->x[3][3]) / 9;
            p->y[1][1] = (-4 * p->y[0][0] + 6 * (p->y[0][1] + p->y[1][0]) - 2 * (p->y[0][3] + p->y[3][0]) + 3 * (p->y[3][1] + p->y[1][3]) - p->y[3][3]) / 9;
            p->x[1][2] = (-4 * p->x[0][3] + 6 * (p->x[0][2] + p->x[1][3]) - 2 * (p->x[0][0] + p->x[3][3]) + 3 * (p->x[3][2] + p->x[1][0]) - p->x[3][0]) / 9;
            p->y[1][2] = (-4 * p->y[0][3] + 6 * (p->y[0][2] + p->y[1][3]) - 2 * (p->y[0][0] + p->y[3][3]) + 3 * (p->y[3][2] + p->y[1][0]) - p->y[3][0]) / 9;
            p->x[2][1] = (-4 * p->x[3][0] + 6 * (p->x[3][1] + p->x[2][0]) - 2 * (p->x[3][3] + p->x[0][0]) + 3 * (p->x[0][1] + p->x[2][3]) - p->x[0][3]) / 9;
            p->y[2][1] = (-4 * p->y[3][0] + 6 * (p->y[3][1] + p->y[2][0]) - 2 * (p->y[3][3] + p->y[0][0]) + 3 * (p->y[0][1] + p->y[2][3]) - p->y[0][3]) / 9;
            p->x[2][2] = (-4 * p->x[3][3] + 6 * (p->x[3][2] + p->x[2][3]) - 2 * (p->x[3][0] + p->x[0][3]) + 3 * (p->x[0][2] + p->x[2][0]) - p->x[0][0]) / 9;
            p->y[2][2] = (-4 * p->y[3][3] + 6 * (p->y[3][2] + p->y[2][3]) - 2 * (p->y[3][0] + p->y[0][3]) + 3 * (p->y[0][2] + p->y[2][0]) - p->y[0][0]) / 9;
        }
    }

    auto shading = std::make_unique<GfxPatchMeshShading>(typeA, patchesA, nPatchesA, std::move(funcsA));
    if (!shading->init(res, dict, out, state)) {
        return {};
    }
    return shading;
}

bool GfxPatchMeshShading::init(GfxResources *res, Dict *dict, OutputDev *out, GfxState *state)
{
    const bool parentInit = GfxShading::init(res, dict, out, state);
    if (!parentInit) {
        return false;
    }

    // funcs needs to be one of the three:
    //  * One function 1-in -> nComps-out
    //  * nComps functions 1-in -> 1-out
    //  * empty
    const int nComps = colorSpace->getNComps();
    const int nFuncs = funcs.size();
    if (nFuncs == 1) {
        if (funcs[0]->getInputSize() != 1) {
            error(errSyntaxWarning, -1, "GfxPatchMeshShading: function with input size != 2");
            return false;
        }
        if (funcs[0]->getOutputSize() != nComps) {
            error(errSyntaxWarning, -1, "GfxPatchMeshShading: function with wrong output size");
            return false;
        }
    } else if (nFuncs == nComps) {
        for (const std::unique_ptr<Function> &f : funcs) {
            if (f->getInputSize() != 1) {
                error(errSyntaxWarning, -1, "GfxPatchMeshShading: function with input size != 2");
                return false;
            }
            if (f->getOutputSize() != 1) {
                error(errSyntaxWarning, -1, "GfxPatchMeshShading: function with wrong output size");
                return false;
            }
        }
    } else if (nFuncs != 0) {
        return false;
    }

    return true;
}

void GfxPatchMeshShading::getParameterizedColor(double t, GfxColor *color) const
{
    double out[gfxColorMaxComps] = {};

    for (unsigned int j = 0; j < funcs.size(); ++j) {
        funcs[j]->transform(&t, &out[j]);
    }
    for (int j = 0; j < gfxColorMaxComps; ++j) {
        color->c[j] = dblToCol(out[j]);
    }
}

std::unique_ptr<GfxShading> GfxPatchMeshShading::copy() const
{
    return std::make_unique<GfxPatchMeshShading>(this);
}

//------------------------------------------------------------------------
// GfxImageColorMap
//------------------------------------------------------------------------

GfxImageColorMap::GfxImageColorMap(int bitsA, Object *decode, std::unique_ptr<GfxColorSpace> &&colorSpaceA) : colorSpace(std::move(colorSpaceA))
{
    int maxPixel, indexHigh;
    unsigned char *indexedLookup;
    const Function *sepFunc;
    double x[gfxColorMaxComps];
    double y[gfxColorMaxComps] = {};
    int i, j, k;
    double mapped;
    bool useByteLookup;

    ok = true;
    useMatte = false;

    // initialize
    for (k = 0; k < gfxColorMaxComps; ++k) {
        lookup[k] = nullptr;
        lookup2[k] = nullptr;
    }
    byte_lookup = nullptr;

    // bits per component and color space
    if (unlikely(bitsA <= 0 || bitsA > 30)) {
        goto err1;
    }

    bits = bitsA;
    maxPixel = (1 << bits) - 1;

    // this is a hack to support 16 bits images, everywhere
    // we assume a component fits in 8 bits, with this hack
    // we treat 16 bit images as 8 bit ones until it's fixed correctly.
    // The hack has another part on ImageStream::getLine
    if (maxPixel > 255) {
        maxPixel = 255;
    }