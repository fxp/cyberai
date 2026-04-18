// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 33/49



    obj1 = dict->lookup("Function");
    if (!obj1.isNull()) {
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
    }

    nVerticesA = nTrianglesA = 0;
    verticesA = nullptr;
    trianglesA = nullptr;
    vertSize = triSize = 0;
    state = 0;
    flag = 0; // make gcc happy
    bitBuf = new GfxShadingBitBuf(str);
    while (true) {
        if (typeA == 4) {
            if (!bitBuf->getBits(flagBits, &flag)) {
                break;
            }
        }
        if (!bitBuf->getBits(coordBits, &x) || !bitBuf->getBits(coordBits, &y)) {
            break;
        }
        for (i = 0; i < nComps; ++i) {
            if (!bitBuf->getBits(compBits, &c[i])) {
                break;
            }
        }
        if (i < nComps) {
            break;
        }
        if (nVerticesA == vertSize) {
            int oldVertSize = vertSize;
            vertSize = (vertSize == 0) ? 16 : 2 * vertSize;
            verticesA = (GfxGouraudVertex *)greallocn_checkoverflow(verticesA, vertSize, sizeof(GfxGouraudVertex));
            if (unlikely(!verticesA)) {
                error(errSyntaxWarning, -1, "GfxGouraudTriangleShading::parse: vertices size overflow");
                gfree(trianglesA);
                delete bitBuf;
                return nullptr;
            }
            memset(verticesA + oldVertSize, 0, (vertSize - oldVertSize) * sizeof(GfxGouraudVertex));
        }
        verticesA[nVerticesA].x = xMin + xMul * (double)x;
        verticesA[nVerticesA].y = yMin + yMul * (double)y;
        for (i = 0; i < nComps; ++i) {
            verticesA[nVerticesA].color.c[i] = dblToCol(cMin[i] + cMul[i] * (double)c[i]);
        }
        ++nVerticesA;
        bitBuf->flushBits();
        if (typeA == 4) {
            if (state == 0 || state == 1) {
                ++state;
            } else if (state == 2 || flag > 0) {
                if (nTrianglesA == triSize) {
                    triSize = (triSize == 0) ? 16 : 2 * triSize;
                    trianglesA = (int (*)[3])greallocn(trianglesA, triSize * 3, sizeof(int));
                }
                if (state == 2) {
                    trianglesA[nTrianglesA][0] = nVerticesA - 3;
                    trianglesA[nTrianglesA][1] = nVerticesA - 2;
                    trianglesA[nTrianglesA][2] = nVerticesA - 1;
                    ++state;
                } else if (flag == 1) {
                    trianglesA[nTrianglesA][0] = trianglesA[nTrianglesA - 1][1];
                    trianglesA[nTrianglesA][1] = trianglesA[nTrianglesA - 1][2];
                    trianglesA[nTrianglesA][2] = nVerticesA - 1;
                } else { // flag == 2
                    trianglesA[nTrianglesA][0] = trianglesA[nTrianglesA - 1][0];
                    trianglesA[nTrianglesA][1] = trianglesA[nTrianglesA - 1][2];
                    trianglesA[nTrianglesA][2] = nVerticesA - 1;
                }
                ++nTrianglesA;
            } else { // state == 3 && flag == 0
                state = 1;
            }
        }
    }
    delete bitBuf;
    if (typeA == 5 && nVerticesA > 0 && vertsPerRow > 0) {
        nRows = nVerticesA / vertsPerRow;
        nTrianglesA = (nRows - 1) * 2 * (vertsPerRow - 1);
        trianglesA = (int (*)[3])gmallocn_checkoverflow(nTrianglesA * 3, sizeof(int));
        if (unlikely(!trianglesA)) {
            gfree(verticesA);
            return nullptr;
        }
        k = 0;
        for (i = 0; i < nRows - 1; ++i) {
            for (j = 0; j < vertsPerRow - 1; ++j) {
                trianglesA[k][0] = i * vertsPerRow + j;
                trianglesA[k][1] = i * vertsPerRow + j + 1;
                trianglesA[k][2] = (i + 1) * vertsPerRow + j;
                ++k;
                trianglesA[k][0] = i * vertsPerRow + j + 1;
                trianglesA[k][1] = (i + 1) * vertsPerRow + j;
                trianglesA[k][2] = (i + 1) * vertsPerRow + j + 1;
                ++k;
            }
        }
    }

    auto shading = std::make_unique<GfxGouraudTriangleShading>(typeA, verticesA, nVerticesA, trianglesA, nTrianglesA, std::move(funcsA));
    if (!shading->init(res, dict, out, gfxState)) {
        return {};
    }
    return shading;
}