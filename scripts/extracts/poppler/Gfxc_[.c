// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 27/41



    for (i = 0; i < patchColorComps; ++i) {
        // these comparisons are done in double arithmetics.
        //
        // For non-parameterized shadings, they are done in color space
        // components.
        if (fabs(patch->color[0][0].c[i] - patch->color[0][1].c[i]) > refineColorThreshold || fabs(patch->color[0][1].c[i] - patch->color[1][1].c[i]) > refineColorThreshold
            || fabs(patch->color[1][1].c[i] - patch->color[1][0].c[i]) > refineColorThreshold || fabs(patch->color[1][0].c[i] - patch->color[0][0].c[i]) > refineColorThreshold) {
            break;
        }
    }
    if (i == patchColorComps || depth == patchMaxDepth) {
        GfxColor flatColor;
        if (shading->isParameterized()) {
            shading->getParameterizedColor(patch->color[0][0].c[0], &flatColor);
        } else {
            for (i = 0; i < colorComps; ++i) {
                // simply cast to the desired type; that's all what is needed.
                flatColor.c[i] = GfxColorComp(patch->color[0][0].c[i]);
            }
        }
        state->setFillColor(&flatColor);
        out->updateFillColor(state);
        state->moveTo(patch->x[0][0], patch->y[0][0]);
        state->curveTo(patch->x[0][1], patch->y[0][1], patch->x[0][2], patch->y[0][2], patch->x[0][3], patch->y[0][3]);
        state->curveTo(patch->x[1][3], patch->y[1][3], patch->x[2][3], patch->y[2][3], patch->x[3][3], patch->y[3][3]);
        state->curveTo(patch->x[3][2], patch->y[3][2], patch->x[3][1], patch->y[3][1], patch->x[3][0], patch->y[3][0]);
        state->curveTo(patch->x[2][0], patch->y[2][0], patch->x[1][0], patch->y[1][0], patch->x[0][0], patch->y[0][0]);
        state->closePath();
        out->fill(state);
        state->clearPath();
    } else {
        for (i = 0; i < 4; ++i) {
            xx[i][0] = patch->x[i][0];
            yy[i][0] = patch->y[i][0];
            xx[i][1] = 0.5 * (patch->x[i][0] + patch->x[i][1]);
            yy[i][1] = 0.5 * (patch->y[i][0] + patch->y[i][1]);
            xxm = 0.5 * (patch->x[i][1] + patch->x[i][2]);
            yym = 0.5 * (patch->y[i][1] + patch->y[i][2]);
            xx[i][6] = 0.5 * (patch->x[i][2] + patch->x[i][3]);
            yy[i][6] = 0.5 * (patch->y[i][2] + patch->y[i][3]);
            xx[i][2] = 0.5 * (xx[i][1] + xxm);
            yy[i][2] = 0.5 * (yy[i][1] + yym);
            xx[i][5] = 0.5 * (xxm + xx[i][6]);
            yy[i][5] = 0.5 * (yym + yy[i][6]);
            xx[i][3] = xx[i][4] = 0.5 * (xx[i][2] + xx[i][5]);
            yy[i][3] = yy[i][4] = 0.5 * (yy[i][2] + yy[i][5]);
            xx[i][7] = patch->x[i][3];
            yy[i][7] = patch->y[i][3];
        }
        for (i = 0; i < 4; ++i) {
            patch00.x[0][i] = xx[0][i];
            patch00.y[0][i] = yy[0][i];
            patch00.x[1][i] = 0.5 * (xx[0][i] + xx[1][i]);
            patch00.y[1][i] = 0.5 * (yy[0][i] + yy[1][i]);
            xxm = 0.5 * (xx[1][i] + xx[2][i]);
            yym = 0.5 * (yy[1][i] + yy[2][i]);
            patch10.x[2][i] = 0.5 * (xx[2][i] + xx[3][i]);
            patch10.y[2][i] = 0.5 * (yy[2][i] + yy[3][i]);
            patch00.x[2][i] = 0.5 * (patch00.x[1][i] + xxm);
            patch00.y[2][i] = 0.5 * (patch00.y[1][i] + yym);
            patch10.x[1][i] = 0.5 * (xxm + patch10.x[2][i]);
            patch10.y[1][i] = 0.5 * (yym + patch10.y[2][i]);
            patch00.x[3][i] = 0.5 * (patch00.x[2][i] + patch10.x[1][i]);
            patch00.y[3][i] = 0.5 * (patch00.y[2][i] + patch10.y[1][i]);
            patch10.x[0][i] = patch00.x[3][i];
            patch10.y[0][i] = patch00.y[3][i];
            patch10.x[3][i] = xx[3][i];
            patch10.y[3][i] = yy[3][i];
        }
        for (i = 4; i < 8; ++i) {
            patch01.x[0][i - 4] = xx[0][i];
            patch01.y[0][i - 4] = yy[0][i];
            patch01.x[1][i - 4] = 0.5 * (xx[0][i] + xx[1][i]);
            patch01.y[1][i - 4] = 0.5 * (yy[0][i] + yy[1][i]);
            xxm = 0.5 * (xx[1][i] + xx[2][i]);
            yym = 0.5 * (yy[1][i] + yy[2][i]);
            patch11.x[2][i - 4] = 0.5 * (xx[2][i] + xx[3][i]);
            patch11.y[2][i - 4] = 0.5 * (yy[2][i] + yy[3][i]);
            patch01.x[2][i - 4] = 0.5 * (patch01.x[1][i - 4] + xxm);
            patch01.y[2][i - 4] = 0.5 * (patch01.y[1][i - 4] + yym);
            patch11.x[1][i - 4] = 0.5 * (xxm + patch11.x[2][i - 4]);
            patch11.y[1][i - 4] = 0.5 * (yym + patch11.y[2][i - 4]);
            patch01.x[3][i - 4] = 0.5 * (patch01.x[2][i - 4] + patch11.x[1][i - 4]);
            patch01.y[3][i - 4] = 0.5 * (patch01.y[2][i - 4] + patch11.y[1][i - 4]);
            patch11.x[0][i - 4] = patch01.x[3][i - 4];
            patch11.y[0][i - 4] = patch01.y[3][i - 4];
            patch11.x[3][i - 4] = xx[3][i];
            patch11.y[3][i - 4] = yy[3][i];
        }
        for (i = 0; i < patchColorComps; ++i) {
            patch00.color[0][0].c[i] = patch->color[0][0].c[i];
            patch00.color[0][1].c[i] = (patch->color[0][0].c[i] + patch->color[0][1].c[i]) / 2.;
       