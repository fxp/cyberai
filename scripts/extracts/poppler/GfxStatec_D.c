// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 36/49



    nPatchesA = 0;
    patchesA = nullptr;
    patchesSize = 0;
    auto bitBuf = std::make_unique<GfxShadingBitBuf>(str);
    while (true) {
        if (!bitBuf->getBits(flagBits, &flag)) {
            break;
        }
        if (typeA == 6) {
            switch (flag) {
            case 0:
                nPts = 12;
                nColors = 4;
                break;
            case 1:
            case 2:
            case 3:
            default:
                nPts = 8;
                nColors = 2;
                break;
            }
        } else {
            switch (flag) {
            case 0:
                nPts = 16;
                nColors = 4;
                break;
            case 1:
            case 2:
            case 3:
            default:
                nPts = 12;
                nColors = 2;
                break;
            }
        }
        for (i = 0; i < nPts; ++i) {
            if (!bitBuf->getBits(coordBits, &xi) || !bitBuf->getBits(coordBits, &yi)) {
                break;
            }
            x[i] = xMin + xMul * (double)xi;
            y[i] = yMin + yMul * (double)yi;
        }
        if (i < nPts) {
            break;
        }
        for (i = 0; i < nColors; ++i) {
            for (j = 0; j < nComps; ++j) {
                if (!bitBuf->getBits(compBits, &ci)) {
                    break;
                }
                c[i][j] = cMin[j] + cMul[j] * (double)ci;
                if (funcsA.empty()) {
                    // ... and colorspace values can also be stored into doubles.
                    // They will be casted later.
                    c[i][j] = dblToCol(c[i][j]);
                }
            }
            if (j < nComps) {
                break;
            }
        }
        if (i < nColors) {
            break;
        }
        if (nPatchesA == patchesSize) {
            int oldPatchesSize = patchesSize;
            patchesSize = (patchesSize == 0) ? 16 : 2 * patchesSize;
            patchesA = (GfxPatch *)greallocn_checkoverflow(patchesA, patchesSize, sizeof(GfxPatch));
            if (unlikely(!patchesA)) {
                return {};
            }
            memset(patchesA + oldPatchesSize, 0, (patchesSize - oldPatchesSize) * sizeof(GfxPatch));
        }
        p = &patchesA[nPatchesA];
        if (typeA == 6) {
            switch (flag) {
            case 0:
                p->x[0][0] = x[0];
                p->y[0][0] = y[0];
                p->x[0][1] = x[1];
                p->y[0][1] = y[1];
                p->x[0][2] = x[2];
                p->y[0][2] = y[2];
                p->x[0][3] = x[3];
                p->y[0][3] = y[3];
                p->x[1][3] = x[4];
                p->y[1][3] = y[4];
                p->x[2][3] = x[5];
                p->y[2][3] = y[5];
                p->x[3][3] = x[6];
                p->y[3][3] = y[6];
                p->x[3][2] = x[7];
                p->y[3][2] = y[7];
                p->x[3][1] = x[8];
                p->y[3][1] = y[8];
                p->x[3][0] = x[9];
                p->y[3][0] = y[9];
                p->x[2][0] = x[10];
                p->y[2][0] = y[10];
                p->x[1][0] = x[11];
                p->y[1][0] = y[11];
                for (j = 0; j < nComps; ++j) {
                    p->color[0][0].c[j] = c[0][j];
                    p->color[0][1].c[j] = c[1][j];
                    p->color[1][1].c[j] = c[2][j];
                    p->color[1][0].c[j] = c[3][j];
                }
                break;
            case 1:
                if (nPatchesA == 0) {
                    gfree(patchesA);
                    return nullptr;
                }
                p->x[0][0] = patchesA[nPatchesA - 1].x[0][3];
                p->y[0][0] = patchesA[nPatchesA - 1].y[0][3];
                p->x[0][1] = patchesA[nPatchesA - 1].x[1][3];
                p->y[0][1] = patchesA[nPatchesA - 1].y[1][3];
                p->x[0][2] = patchesA[nPatchesA - 1].x[2][3];
                p->y[0][2] = patchesA[nPatchesA - 1].y[2][3];
                p->x[0][3] = patchesA[nPatchesA - 1].x[3][3];
                p->y[0][3] = patchesA[nPatchesA - 1].y[3][3];
                p->x[1][3] = x[0];
                p->y[1][3] = y[0];
                p->x[2][3] = x[1];
                p->y[2][3] = y[1];
                p->x[3][3] = x[2];
                p->y[3][3] = y[2];
                p->x[3][2] = x[3];
                p->y[3][2] = y[3];
                p->x[3][1] = x[4];
                p->y[3][1] = y[4];
                p->x[3][0] = x[5];
                p->y[3][0] = y[5];
                p->x[2][0] = x[6];
                p->y[2][0] = y[6];
                p->x[1][0] = x[7];
                p->y[1][0] = y[7];
                for (j = 0; j < nComps; ++j) {
                    p->color[0][0].c[j] = patchesA[nPatchesA - 1].color[0][1].c[j];
                    p->color[0][1].c[j] = patchesA[nPatchesA - 1].color[1][1].c[j];
                    p->color[1][1].c[j] = c[0][j];
                    p-