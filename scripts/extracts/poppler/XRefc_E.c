// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 5/13



    // get the 'Prev' pointer
    obj2 = obj.getDict()->lookupNF("Prev").copy();
    if (obj2.isInt() || obj2.isInt64()) {
        if (obj2.isInt()) {
            pos2 = obj2.getInt();
        } else {
            pos2 = obj2.getInt64();
        }
        if (pos2 != *pos) {
            *pos = pos2;
            more = true;
        } else {
            error(errSyntaxWarning, -1, "Infinite loop in xref table");
            more = false;
        }
    } else if (obj2.isRef()) {
        // certain buggy PDF generators generate "/Prev NNN 0 R" instead
        // of "/Prev NNN"
        pos2 = (unsigned int)obj2.getRefNum();
        if (pos2 != *pos) {
            *pos = pos2;
            more = true;
        } else {
            error(errSyntaxWarning, -1, "Infinite loop in xref table");
            more = false;
        }
    } else {
        more = false;
    }

    // save the first trailer dictionary
    if (trailerDict.isNone()) {
        trailerDict = obj.copy();
    }

    // check for an 'XRefStm' key
    obj2 = obj.getDict()->lookup("XRefStm");
    if (obj2.isInt() || obj2.isInt64()) {
        if (obj2.isInt()) {
            pos2 = obj2.getInt();
        } else {
            pos2 = obj2.getInt64();
        }
        for (size_t i = 0; ok && i < followedXRefStm->size(); ++i) {
            if (followedXRefStm->at(i) == pos2) {
                ok = false;
            }
        }
        // Arbitrary limit because otherwise we exhaust the stack
        // calling readXRef + readXRefTable
        if (followedXRefStm->size() > 4096) {
            error(errSyntaxError, -1, "File has more than 4096 XRefStm, aborting");
            ok = false;
        }
        if (ok) {
            followedXRefStm->push_back(pos2);
            readXRef(&pos2, followedXRefStm, xrefStreamObjsNum);
        }
        if (!ok) {
            goto err0;
        }
    }

    return more;

err0:
    ok = false;
    return false;
}

bool XRef::readXRefStream(Stream *xrefStr, Goffset *pos)
{
    int w[3];
    bool more;
    Object obj;

    ok = false;

    Dict *dict = xrefStr->getDict();
    obj = dict->lookupNF("Size").copy();
    if (!obj.isInt()) {
        return false;
    }
    int newSize = obj.getInt();
    if (newSize < 0) {
        return false;
    }
    if (newSize > size) {
        if (resize(newSize) != newSize) {
            error(errSyntaxError, -1, "Invalid 'size' parameter");
            return false;
        }
    }

    obj = dict->lookupNF("W").copy();
    if (!obj.isArrayOfLengthAtLeast(3)) {
        return false;
    }
    for (int i = 0; i < 3; ++i) {
        Object obj2 = obj.arrayGet(i);
        if (!obj2.isInt()) {
            return false;
        }
        w[i] = obj2.getInt();
        if (w[i] < 0) {
            return false;
        }
    }
    constexpr int intNBytes = sizeof(int);
    constexpr int longLongNBytes = sizeof(long long);
    if (w[0] > intNBytes || w[1] > longLongNBytes || w[2] > longLongNBytes) {
        return false;
    }

    if (!xrefStr->rewind()) {
        return false;
    }

    const Object &idx = dict->lookupNF("Index");
    if (idx.isArray()) {
        for (int i = 0; i + 1 < idx.arrayGetLength(); i += 2) {
            obj = idx.arrayGet(i);
            if (!obj.isInt()) {
                return false;
            }
            int first = obj.getInt();
            obj = idx.arrayGet(i + 1);
            if (!obj.isInt()) {
                return false;
            }
            int n = obj.getInt();
            if (first < 0 || n < 0 || !readXRefStreamSection(xrefStr, w, first, n)) {
                return false;
            }
        }
    } else {
        if (!readXRefStreamSection(xrefStr, w, 0, newSize)) {
            return false;
        }
    }

    obj = dict->lookupNF("Prev").copy();
    if (obj.isInt() && obj.getInt() >= 0) {
        *pos = obj.getInt();
        more = true;
    } else if (obj.isInt64() && obj.getInt64() >= 0) {
        *pos = obj.getInt64();
        more = true;
    } else {
        more = false;
    }
    if (trailerDict.isNone()) {
        trailerDict = xrefStr->getDictObject()->copy();
    }

    ok = true;
    return more;
}

bool XRef::readXRefStreamSection(Stream *xrefStr, const int *w, int first, int n)
{
    unsigned long long offset, gen;
    int type, c, i, j;