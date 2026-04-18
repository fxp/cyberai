// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 38/49

hesA - 1].x[1][3];
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
                p->x[1][1] = x[8];
                p->y[1][1] = y[8];
                p->x[1][2] = x[9];
                p->y[1][2] = y[9];
                p->x[2][2] = x[10];
                p->y[2][2] = y[10];
                p->x[2][1] = x[11];
                p->y[2][1] = y[11];
                for (j = 0; j < nComps; ++j) {
                    p->color[0][0].c[j] = patchesA[nPatchesA - 1].color[0][1].c[j];
                    p->color[0][1].c[j] = patchesA[nPatchesA - 1].color[1][1].c[j];
                    p->color[1][1].c[j] = c[0][j];
                    p->color[1][0].c[j] = c[1][j];
                }
                break;
            case 2:
                if (nPatchesA == 0) {
                    gfree(patchesA);
                    return {};
                }
                p->x[0][0] = patchesA[nPatchesA - 1].x[3][3];
                p->y[0][0] = patchesA[nPatchesA - 1].y[3][3];
                p->x[0][1] = patchesA[nPatchesA - 1].x[3][2];
                p->y[0][1] = patchesA[nPatchesA - 1].y[3][2];
                p->x[0][2] = patchesA[nPatchesA - 1].x[3][1];
                p->y[0][2] = patchesA[nPatchesA - 1].y[3][1];
                p->x[0][3] = patchesA[nPatchesA - 1].x[3][0];
                p->y[0][3] = patchesA[nPatchesA - 1].y[3][0];
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
                p->x[1][1] = x[8];
                p->y[1][1] = y[8];
                p->x[1][2] = x[9];
                p->y[1][2] = y[9];
                p->x[2][2] = x[10];
                p->y[2][2] = y[10];
                p->x[2][1] = x[11];
                p->y[2][1] = y[11];
                for (j = 0; j < nComps; ++j) {
                    p->color[0][0].c[j] = patchesA[nPatchesA - 1].color[1][1].c[j];
                    p->color[0][1].c[j] = patchesA[nPatchesA - 1].color[1][0].c[j];
                    p->color[1][1].c[j] = c[0][j];
                    p->color[1][0].c[j] = c[1][j];
                }
                break;
            case 3:
                if (nPatchesA == 0) {
                    gfree(patchesA);
                    return {};
                }
                p->x[0][0] = patchesA[nPatchesA - 1].x[3][0];
                p->y[0][0] = patchesA[nPatchesA - 1].y[3][0];
                p->x[0][1] = patchesA[nPatchesA - 1].x[2][0];
                p->y[0][1] = patchesA[nPatchesA - 1].y[2][0];
                p->x[0][2] = patchesA[nPatchesA - 1].x[1][0];
                p->y[0][2] = patchesA[nPatchesA - 1].y[1][0];
                p->x[0][3] = patchesA[nPatchesA - 1].x[0][0];
                p->y[0][3] = patchesA[nPatchesA - 1].y[0][0];
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
                p->x[1][1] = x[8];
                p->y[1][1] = y[8];
                p->x[1][2] = x[9];
                p->y[1][2] = y[9];
                p->x[2][2] = x[10];
                p->y[2][2] = y[10];
                p->x[2][1] = x[11];
                p->y[2][1] = y[11];
                for (j = 0; j < nComps; ++j) {
                    p->color[0][0].c[j] = patchesA[nPatchesA - 1].color[1][0].c[j];
            