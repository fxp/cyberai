// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 35/49



    obj1 = dict->lookup("BitsPerCoordinate");
    if (obj1.isInt()) {
        coordBits = obj1.getInt();
    } else {
        error(errSyntaxWarning, -1, "Missing or invalid BitsPerCoordinate in shading dictionary");
        return {};
    }
    if (unlikely(coordBits <= 0)) {
        error(errSyntaxWarning, -1, "Invalid BitsPerCoordinate in shading dictionary");
        return {};
    }
    obj1 = dict->lookup("BitsPerComponent");
    if (obj1.isInt()) {
        compBits = obj1.getInt();
    } else {
        error(errSyntaxWarning, -1, "Missing or invalid BitsPerComponent in shading dictionary");
        return {};
    }
    if (unlikely(compBits <= 0 || compBits > 31)) {
        error(errSyntaxWarning, -1, "Invalid BitsPerComponent in shading dictionary");
        return {};
    }
    obj1 = dict->lookup("BitsPerFlag");
    if (obj1.isInt()) {
        flagBits = obj1.getInt();
    } else {
        error(errSyntaxWarning, -1, "Missing or invalid BitsPerFlag in shading dictionary");
        return {};
    }
    obj1 = dict->lookup("Decode");
    if (obj1.isArrayOfLengthAtLeast(6)) {
        bool decodeOk = true;
        xMin = obj1.arrayGet(0).getNum(&decodeOk);
        xMax = obj1.arrayGet(1).getNum(&decodeOk);
        xMul = (xMax - xMin) / (pow(2.0, coordBits) - 1);
        yMin = obj1.arrayGet(2).getNum(&decodeOk);
        yMax = obj1.arrayGet(3).getNum(&decodeOk);
        yMul = (yMax - yMin) / (pow(2.0, coordBits) - 1);
        for (i = 0; 5 + 2 * i < obj1.arrayGetLength() && i < gfxColorMaxComps; ++i) {
            cMin[i] = obj1.arrayGet(4 + 2 * i).getNum(&decodeOk);
            cMax[i] = obj1.arrayGet(5 + 2 * i).getNum(&decodeOk);
            cMul[i] = (cMax[i] - cMin[i]) / (double)((1U << compBits) - 1);
        }
        nComps = i;

        if (!decodeOk) {
            error(errSyntaxWarning, -1, "Missing or invalid Decode array in shading dictionary");
            return {};
        }
    } else {
        error(errSyntaxWarning, -1, "Missing or invalid Decode array in shading dictionary");
        return {};
    }

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