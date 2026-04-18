// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 13/13



struct DummyXRefEntry : XRefEntry
{
    DummyXRefEntry()
    {
        offset = -1;
        gen = 0;
        type = xrefEntryNone;
        flags = 0;
        obj = Object::null();
    }
};

DummyXRefEntry dummyXRefEntry;

}

XRefEntry *XRef::getEntry(int i, bool complainIfMissing)
{
    if (unlikely(i < 0)) {
        error(errInternal, -1, "Request for invalid XRef entry [{0:d}]", i);
        return &dummyXRefEntry;
    }

    if (i >= size || entries[i].type == xrefEntryNone) {

        if ((!xRefStream) && mainXRefEntriesOffset) {
            if (unlikely(i >= capacity)) {
                error(errInternal, -1, "Request for out-of-bounds XRef entry [{0:d}]", i);
                return &dummyXRefEntry;
            }

            if (!parseEntry(mainXRefEntriesOffset + 20 * i, &entries[i])) {
                error(errSyntaxError, -1, "Failed to parse XRef entry [{0:d}].", i);
                return &dummyXRefEntry;
            }
        } else {
            // Read XRef tables until the entry we're looking for is found
            readXRefUntil(i);

            // We might have reconstructed the xref
            // Check again i is in bounds
            if (unlikely(i >= size)) {
                return &dummyXRefEntry;
            }

            if (entries[i].type == xrefEntryNone) {
                if (complainIfMissing) {
                    error(errSyntaxError, -1, "Invalid XRef entry {0:d}", i);
                }
                entries[i].type = xrefEntryFree;
            }
        }
    }

    return &entries[i];
}

// Recursively sets the Unencrypted flag in all referenced xref entries
void XRef::markUnencrypted(Object *obj)
{
    Object obj1;

    switch (obj->getType()) {
    case objArray: {
        Array *array = obj->getArray();
        for (int i = 0; i < array->getLength(); i++) {
            obj1 = array->getNF(i).copy();
            markUnencrypted(&obj1);
        }
        break;
    }
    case objStream:
    case objDict: {
        Dict *dict;
        if (obj->getType() == objStream) {
            Stream *stream = obj->getStream();
            dict = stream->getDict();
        } else {
            dict = obj->getDict();
        }
        for (int i = 0; i < dict->getLength(); i++) {
            obj1 = dict->getValNF(i).copy();
            markUnencrypted(&obj1);
        }
        break;
    }
    case objRef: {
        const Ref ref = obj->getRef();
        XRefEntry *e = getEntry(ref.num);
        if (e->getFlag(XRefEntry::Unencrypted)) {
            return; // We've already been here: prevent infinite recursion
        }
        e->setFlag(XRefEntry::Unencrypted, true);
        obj1 = fetch(ref);
        markUnencrypted(&obj1);
        break;
    }
    default:
        break;
    }
}

void XRef::scanSpecialFlags()
{
    if (scannedSpecialFlags) {
        return;
    }
    scannedSpecialFlags = true;

    // "Rewind" the XRef linked list, so that readXRefUntil re-reads all XRef
    // tables/streams, even those that had already been parsed
    prevXRefOffset = mainXRefOffset;

    std::vector<int> xrefStreamObjNums;
    if (!streamEndsLen) { // don't do it for already reconstructed xref
        readXRefUntil(-1 /* read all xref sections */, &xrefStreamObjNums);
    }

    // Mark object streams as DontRewrite, because we write each object
    // individually in full rewrite mode.
    for (int i = 0; i < size; ++i) {
        if (entries[i].type == xrefEntryCompressed) {
            const Goffset objStmNum = entries[i].offset;
            if (unlikely(objStmNum < 0 || objStmNum >= size)) {
                error(errSyntaxError, -1, "Compressed object offset out of xref bounds");
            } else {
                getEntry(static_cast<int>(objStmNum))->setFlag(XRefEntry::DontRewrite, true);
            }
        }
    }

    // Mark XRef streams objects as Unencrypted and DontRewrite
    for (const int objNum : xrefStreamObjNums) {
        getEntry(objNum)->setFlag(XRefEntry::Unencrypted, true);
        getEntry(objNum)->setFlag(XRefEntry::DontRewrite, true);
    }

    // Mark objects referred from the Encrypt dict as Unencrypted
    markUnencrypted();
}

void XRef::markUnencrypted()
{
    // Mark objects referred from the Encrypt dict as Unencrypted
    const Object &obj = trailerDict.dictLookupNF("Encrypt");
    if (obj.isRef()) {
        XRefEntry *e = getEntry(obj.getRefNum());
        e->setFlag(XRefEntry::Unencrypted, true);
    }
}

XRef::XRefWriter::~XRefWriter() = default;
