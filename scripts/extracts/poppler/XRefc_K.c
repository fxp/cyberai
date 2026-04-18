// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 11/13



            capacity = num + 1;
        }
        for (int i = size; i < num + 1; ++i) {
            entries[i].offset = -1;
            entries[i].type = xrefEntryFree;
            new (&entries[i].obj) Object();
            entries[i].obj = Object::null();
            entries[i].flags = 0;
            entries[i].gen = 0;
        }
        size = num + 1;
    }
    XRefEntry *e = getEntry(num);
    e->gen = gen;
    e->obj.setToNull();
    e->flags = 0;
    if (used) {
        e->type = xrefEntryUncompressed;
        e->offset = offs;
    } else {
        e->type = xrefEntryFree;
        e->offset = 0;
    }
    return true;
}

void XRef::setModifiedObject(const Object *o, Ref r)
{
    xrefLocker();
    if (r.num < 0 || r.num >= size) {
        error(errInternal, -1, "XRef::setModifiedObject on unknown ref: {0:d}, {1:d}", r.num, r.gen);
        return;
    }
    XRefEntry *e = getEntry(r.num);
    if (unlikely(e->type == xrefEntryFree)) {
        error(errInternal, -1, "XRef::setModifiedObject on ref: {0:d}, {1:d} that is marked as free. This will cause a memory leak", r.num, r.gen);
    }
    e->obj = o->copy();
    e->setFlag(XRefEntry::Updated, true);
    setModified();
}

Ref XRef::addIndirectObject(const Object &o)
{
    int entryIndexToUse = -1;
    for (int i = 1; entryIndexToUse == -1 && i < size; ++i) {
        XRefEntry *e = getEntry(i, false /* complainIfMissing */);
        if (e->type == xrefEntryFree && e->gen < 65535) {
            entryIndexToUse = i;
        }
    }

    XRefEntry *e;
    if (entryIndexToUse == -1) {
        entryIndexToUse = size;
        add(entryIndexToUse, 0, 0, false);
        e = getEntry(entryIndexToUse);
    } else {
        // reuse a free entry
        e = getEntry(entryIndexToUse);
        // we don't touch gen number, because it should have been
        // incremented when the object was deleted
    }
    e->type = xrefEntryUncompressed;
    e->obj = o.copy();
    e->setFlag(XRefEntry::Updated, true);
    setModified();

    Ref r;
    r.num = entryIndexToUse;
    r.gen = e->gen;
    return r;
}

void XRef::removeIndirectObject(Ref r)
{
    xrefLocker();
    if (r.num < 0 || r.num >= size) {
        error(errInternal, -1, "XRef::removeIndirectObject on unknown ref: {0:d}, {1:d}", r.num, r.gen);
        return;
    }
    XRefEntry *e = getEntry(r.num);
    if (e->type == xrefEntryFree) {
        return;
    }
    e->obj = Object();
    e->type = xrefEntryFree;
    if (likely(e->gen < 65535)) {
        e->gen++;
    }
    e->setFlag(XRefEntry::Updated, true);
    setModified();
}

Ref XRef::addStreamObject(std::unique_ptr<Dict> dict, std::vector<char> buffer, StreamCompression compression)
{
    dict->add("Length", Object((int)buffer.size()));
    auto stream = std::make_unique<AutoFreeMemStream>(std::move(buffer), Object(std::move(dict)));
    stream->setFilterRemovalForbidden(true);
    switch (compression) {
    case StreamCompression::None:;
        break;
    case StreamCompression::Compress:
        stream->getDict()->add("Filter", Object(objName, "FlateDecode"));
        break;
    }
    return addIndirectObject(Object(std::move(stream)));
}

void XRef::writeXRef(XRef::XRefWriter *writer, bool writeAllEntries)
{
    // create free entries linked-list
    if (getEntry(0)->gen != 65535) {
        error(errInternal, -1, "XRef::writeXRef, entry 0 of the XRef is invalid (gen != 65535)");
    }
    int lastFreeEntry = 0;
    for (int i = 0; i < size; i++) {
        if (getEntry(i)->type == xrefEntryFree) {
            getEntry(lastFreeEntry)->offset = i;
            lastFreeEntry = i;
        }
    }
    getEntry(lastFreeEntry)->offset = 0;

    if (writeAllEntries) {
        writer->startSection(0, size);
        for (int i = 0; i < size; i++) {
            XRefEntry *e = getEntry(i);
            if (e->gen > 65535) {
                e->gen = 65535; // cap generation number to 65535 (required by PDFReference)
            }
            writer->writeEntry(e->offset, e->gen, e->type);
        }
    } else {
        int i = 0;
        while (i < size) {
            int j;
            for (j = i; j < size; j++) { // look for consecutive entries
                if ((getEntry(j)->type == xrefEntryFree) && (getEntry(j)->gen == 0)) {
                    break;
                }
            }
            if (j - i != 0) {
                writer->startSection(i, j - i);
                for (int k = i; k < j; k++) {
                    XRefEntry *e = getEntry(k);
                    if (e->gen > 65535) {
                        e->gen = 65535; // cap generation number to 65535 (required by PDFReference)
                    }
                    writer->writeEntry(e->offset, e->gen, e->type);
                }
                i = j;
            } else {
                ++i;
            }
        }
    }
}

XRef::XRefTableWriter::XRefTableWriter(OutStream *outStrA)
{
    outStr = outStrA;
}