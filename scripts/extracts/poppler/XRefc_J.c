// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 10/13



        ObjectStream *objStr = nullptr;
        if (const auto it = objStrs.find(e->offset); it != objStrs.end()) {
            objStr = it->second.get();
        } else {
            objStr = new ObjectStream(this, static_cast<int>(e->offset), recursion + 1);
            if (!objStr->isOk()) {
                delete objStr;
                objStr = nullptr;
                goto err;
            } else {
                // XRef could be reconstructed in constructor of ObjectStream:
                e = getEntry(num);
                const auto [_, inserted] = objStrs.emplace(e->offset, std::unique_ptr<ObjectStream>(objStr));
                if (!inserted) {
                    // this happens when indeed the xref was reconstructed, and the reconstuction ended up adding an ObjectStream
                    // for that offset, so fetch the good objStr because the one we had was just deleted by emplace failing
                    objStr = objStrs.find(e->offset)->second.get();
                }
            }
        }
        if (endPos) {
            *endPos = -1;
        }
        return objStr->getObject(e->gen, num);
    }

    default:
        goto err;
    }

err:
    if (!xRefStream && !xrefReconstructed) {
        // Check if there has been any updated object, if there has been we can't reconstruct because that would mean losing the changes
        bool xrefHasChanges = false;
        for (int i = 0; i < size; i++) {
            if (entries[i].getFlag(XRefEntry::Updated)) {
                xrefHasChanges = true;
                break;
            }
        }
        if (xrefHasChanges) {
            error(errInternal, -1, "xref num {0:d} not found but needed, document has changes, reconstruct aborted", num);
            // pretend we constructed the xref, otherwise we will do this check again and again
            xrefReconstructed = true;
            return Object::null();
        }

        error(errInternal, -1, "xref num {0:d} not found but needed, try to reconstruct", num);
        rootNum = -1;
        constructXRef(&xrefReconstructed);
        remover.reset(); // Manually delete the remover since we're calling ourselves so recursion for this one is valid
        return fetch(num, gen, ++recursion, endPos);
    }
    if (endPos) {
        *endPos = -1;
    }
    return Object::null();
}

void XRef::lock()
{
    mutex.lock();
}

void XRef::unlock()
{
    mutex.unlock();
}

Object XRef::getDocInfo()
{
    return trailerDict.dictLookup("Info");
}

// Added for the pdftex project.
Object XRef::getDocInfoNF()
{
    return trailerDict.dictLookupNF("Info").copy();
}

Object XRef::createDocInfoIfNeeded(Ref *ref)
{
    Object obj = trailerDict.getDict()->lookup("Info", ref);
    getDocInfo();

    if (obj.isDict() && *ref != Ref::INVALID()) {
        // Info is valid if it's a dict and to pointed by an indirect reference
        return obj;
    }

    removeDocInfo();

    obj = Object(std::make_unique<Dict>(this));
    *ref = addIndirectObject(obj);
    trailerDict.dictSet("Info", Object(*ref));

    return obj;
}

void XRef::removeDocInfo()
{
    Object infoObjRef = getDocInfoNF();
    if (infoObjRef.isNull()) {
        return;
    }

    trailerDict.dictRemove("Info");

    if (likely(infoObjRef.isRef())) {
        removeIndirectObject(infoObjRef.getRef());
    }
}

bool XRef::getStreamEnd(Goffset streamStart, Goffset *streamEnd)
{
    int a, b, m;

    if (streamEndsLen == 0 || streamStart > streamEnds[streamEndsLen - 1]) {
        return false;
    }

    a = -1;
    b = streamEndsLen - 1;
    // invariant: streamEnds[a] < streamStart <= streamEnds[b]
    while (b - a > 1) {
        m = (a + b) / 2;
        if (streamStart <= streamEnds[m]) {
            b = m;
        } else {
            a = m;
        }
    }
    *streamEnd = streamEnds[b];
    return true;
}

int XRef::getNumEntry(Goffset offset)
{
    if (size > 0) {
        int res = 0;
        Goffset resOffset = getEntry(0)->offset;
        XRefEntry *e;
        for (int i = 1; i < size; ++i) {
            e = getEntry(i, false);
            if (e->type != xrefEntryFree && e->offset < offset && e->offset >= resOffset) {
                res = i;
                resOffset = e->offset;
            }
        }
        return res;
    }
    return -1;
}

void XRef::add(Ref ref, Goffset offs, bool used)
{
    add(ref.num, ref.gen, offs, used);
}

bool XRef::add(int num, int gen, Goffset offs, bool used)
{
    xrefLocker();
    if (num >= size) {
        if (num >= capacity) {
            entries = (XRefEntry *)greallocn_checkoverflow(entries, num + 1, sizeof(XRefEntry));
            if (unlikely(entries == nullptr)) {
                size = 0;
                capacity = 0;
                return false;
            }