// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 49/49



    if (obj->isName()) {
        for (i = 0; i < nGfxBlendModeNames; ++i) {
            if (!strcmp(obj->getName(), gfxBlendModeNames[i].name)) {
                *mode = gfxBlendModeNames[i].mode;
                return true;
            }
        }
        return false;
    }
    if (obj->isArray()) {
        for (i = 0; i < obj->arrayGetLength(); ++i) {
            Object obj2 = obj->arrayGet(i);
            if (!obj2.isName()) {
                return false;
            }
            for (j = 0; j < nGfxBlendModeNames; ++j) {
                if (!strcmp(obj2.getName(), gfxBlendModeNames[j].name)) {
                    *mode = gfxBlendModeNames[j].mode;
                    return true;
                }
            }
        }
        *mode = gfxBlendNormal;
        return true;
    }
    return false;
}
