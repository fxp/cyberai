// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 3/13



        // if there was a problem with the xref table,
        // try to reconstruct it
        if (!ok) {
            if (!(ok = constructXRef(wasReconstructed))) {
                errCode = errDamaged;
                return;
            }
        }
    }

    // set size to (at least) the size specified in trailer dict
    obj = trailerDict.dictLookupNF("Size").copy();
    if (!obj.isInt()) {
        error(errSyntaxWarning, -1, "No valid XRef size in trailer");
    } else {
        if (obj.getInt() > size) {
            if (resize(obj.getInt()) != obj.getInt()) {
                if (!(ok = constructXRef(wasReconstructed))) {
                    errCode = errDamaged;
                    return;
                }
            }
        }
    }

    // get the root dictionary (catalog) object
    obj = trailerDict.dictLookupNF("Root").copy();
    if (obj.isRef()) {
        rootNum = obj.getRefNum();
        rootGen = obj.getRefGen();
    } else {
        if (!(ok = constructXRef(wasReconstructed))) {
            errCode = errDamaged;
            return;
        }
    }

    // now set the trailer dictionary's xref pointer so we can fetch
    // indirect objects from it
    trailerDict.getDict()->setXRef(this);
}

XRef::~XRef()
{
    for (int i = 0; i < size; i++) {
        if (entries[i].type == xrefEntryFree) {
            continue;
        }

        entries[i].obj.~Object();
    }
    gfree(entries);

    if (streamEnds) {
        gfree(streamEnds);
    }
}

XRef *XRef::copy() const
{
    XRef *xref = new XRef();
    xref->strOwner = str->copy();
    xref->str = xref->strOwner.get();
    xref->encrypted = encrypted;
    xref->permFlags = permFlags;
    xref->ownerPasswordOk = ownerPasswordOk;
    xref->rootGen = rootGen;
    xref->rootNum = rootNum;

    xref->start = start;
    xref->prevXRefOffset = prevXRefOffset;
    xref->mainXRefEntriesOffset = mainXRefEntriesOffset;
    xref->xRefStream = xRefStream;
    xref->trailerDict = trailerDict.copy();
    xref->encAlgorithm = encAlgorithm;
    xref->encRevision = encRevision;
    xref->encVersion = encVersion;
    xref->permFlags = permFlags;
    xref->keyLength = keyLength;
    xref->permFlags = permFlags;
    for (int i = 0; i < 32; i++) {
        xref->fileKey[i] = fileKey[i];
    }

    if (xref->reserve(size) == 0) {
        error(errSyntaxError, -1, "unable to allocate {0:d} entries", size);
        delete xref;
        return nullptr;
    }
    xref->size = size;
    for (int i = 0; i < size; ++i) {
        xref->entries[i].offset = entries[i].offset;
        xref->entries[i].type = entries[i].type;
        // set the object to null, it will be fetched from the stream when needed
        new (&xref->entries[i].obj) Object();
        xref->entries[i].obj = Object::null();
        xref->entries[i].flags = entries[i].flags;
        xref->entries[i].gen = entries[i].gen;

        // If entry has been changed from the stream value we need to copy it
        // otherwise it's lost
        if (entries[i].getFlag(XRefEntry::Updated)) {
            xref->entries[i].obj = entries[i].obj.copy();
        }
    }
    xref->streamEndsLen = streamEndsLen;
    if (streamEndsLen != 0) {
        xref->streamEnds = (Goffset *)gmalloc(streamEndsLen * sizeof(Goffset));
        for (int i = 0; i < streamEndsLen; i++) {
            xref->streamEnds[i] = streamEnds[i];
        }
    }
    return xref;
}

int XRef::reserve(int newSize)
{
    if (newSize > capacity) {
        int newCapacity = 1024;
        if (capacity) {
            if (capacity <= INT_MAX / 2) {
                newCapacity = capacity * 2;
            } else {
                newCapacity = newSize;
            }
        }
        while (newSize > newCapacity) {
            if (newCapacity > INT_MAX / 2) {
                std::fputs("Too large XRef size\n", stderr);
                return 0;
            }
            newCapacity *= 2;
        }
        if (newCapacity >= INT_MAX / (int)sizeof(XRefEntry)) {
            std::fputs("Too large XRef size\n", stderr);
            return 0;
        }

        void *p = grealloc(entries, newCapacity * sizeof(XRefEntry),
                           /* checkoverflow=*/true);
        if (p == nullptr) {
            return 0;
        }

        entries = (XRefEntry *)p;
        capacity = newCapacity;
    }

    return capacity;
}

int XRef::resize(int newSize)
{
    if (newSize > size) {

        if (reserve(newSize) < newSize) {
            return size;
        }

        for (int i = size; i < newSize; ++i) {
            entries[i].offset = -1;
            entries[i].type = xrefEntryNone;
            new (&entries[i].obj) Object();
            entries[i].obj = Object::null();
            entries[i].flags = 0;
            entries[i].gen = 0;
        }
    } else {
        for (int i = newSize; i < size; i++) {
            entries[i].obj.~Object();
        }
    }

    size = newSize;

    return size;
}