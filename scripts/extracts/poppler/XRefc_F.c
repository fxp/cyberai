// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 6/13



    if (first > INT_MAX - n) {
        return false;
    }
    if (first + n < 0) {
        return false;
    }
    if (first + n > size) {
        if (resize(first + n) != size) {
            error(errSyntaxError, -1, "Invalid 'size' inside xref table");
            return false;
        }
        if (first + n > size) {
            error(errSyntaxError, -1, "Invalid 'first' or 'n' inside xref table");
            return false;
        }
    }
    for (i = first; i < first + n; ++i) {
        if (w[0] == 0) {
            type = 1;
        } else {
            for (type = 0, j = 0; j < w[0]; ++j) {
                if ((c = xrefStr->getChar()) == EOF) {
                    return false;
                }
                type = (type << 8) + c;
            }
        }
        for (offset = 0, j = 0; j < w[1]; ++j) {
            if ((c = xrefStr->getChar()) == EOF) {
                return false;
            }
            offset = (offset << 8) + c;
        }
        if (offset > (unsigned long long)GoffsetMax()) {
            error(errSyntaxError, -1, "Offset inside xref table too large for fseek");
            return false;
        }
        for (gen = 0, j = 0; j < w[2]; ++j) {
            if ((c = xrefStr->getChar()) == EOF) {
                return false;
            }
            gen = (gen << 8) + c;
        }
        if (gen > INT_MAX) {
            if (i == 0 && gen == std::numeric_limits<uint32_t>::max()) {
                // workaround broken generators
                gen = 65535;
            } else {
                error(errSyntaxError, -1, "Gen inside xref table too large (bigger than INT_MAX)");
                return false;
            }
        }
        if (entries[i].offset == -1) {
            switch (type) {
            case 0:
                entries[i].offset = offset;
                entries[i].gen = static_cast<int>(gen);
                entries[i].type = xrefEntryFree;
                break;
            case 1:
                entries[i].offset = offset;
                entries[i].gen = static_cast<int>(gen);
                entries[i].type = xrefEntryUncompressed;
                break;
            case 2:
                entries[i].offset = offset;
                entries[i].gen = static_cast<int>(gen);
                entries[i].type = xrefEntryCompressed;
                break;
            default:
                return false;
            }
        }
    }

    return true;
}

// Attempt to construct an xref table for a damaged file.
bool XRef::constructXRef(bool *wasReconstructed, bool needCatalogDict)
{
    int *streamObjNums = nullptr;
    int streamObjNumsLen = 0;
    int streamObjNumsSize = 0;
    int lastObjNum = -1;
    rootNum = -1;
    int streamEndsSize = 0;
    streamEndsLen = 0;

    resize(0); // free entries properly
    gfree(entries);
    capacity = 0;
    size = 0;
    entries = nullptr;

    if (wasReconstructed) {
        *wasReconstructed = true;
    }

    if (xrefReconstructedCb) {
        xrefReconstructedCb();
    }

    if (!str->rewind()) {
        return false;
    }
    char buf[4096 + 1];