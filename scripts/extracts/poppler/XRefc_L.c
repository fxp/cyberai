// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 12/13



void XRef::XRefTableWriter::startSection(int first, int count)
{
    outStr->printf("%i %i\r\n", first, count);
}

void XRef::XRefTableWriter::writeEntry(Goffset offset, int gen, XRefEntryType type)
{
    outStr->printf("%010lli %05i %c\r\n", (long long)offset, gen, (type == xrefEntryFree) ? 'f' : 'n');
}

void XRef::writeTableToFile(OutStream *outStr, bool writeAllEntries)
{
    XRefTableWriter writer(outStr);
    outStr->printf("xref\r\n");
    writeXRef(&writer, writeAllEntries);
}

XRef::XRefStreamWriter::XRefStreamWriter(Array *indexA, GooString *stmBufA, int offsetSizeA)
{
    index = indexA;
    stmBuf = stmBufA;
    offsetSize = offsetSizeA;
}

void XRef::XRefStreamWriter::startSection(int first, int count)
{
    index->add(Object(first));
    index->add(Object(count));
}

void XRef::XRefStreamWriter::writeEntry(Goffset offset, int gen, XRefEntryType type)
{
    const int entryTotalSize = 1 + offsetSize + 2; /* type + offset + gen */
    char data[16];
    data[0] = (type == xrefEntryFree) ? 0 : 1;
    for (int i = offsetSize; i > 0; i--) {
        data[i] = offset & 0xff;
        offset >>= 8;
    }
    data[offsetSize + 1] = (gen >> 8) & 0xff;
    data[offsetSize + 2] = gen & 0xff;
    stmBuf->append(data, entryTotalSize);
}

XRef::XRefPreScanWriter::XRefPreScanWriter()
{
    hasOffsetsBeyond4GB = false;
}

void XRef::XRefPreScanWriter::startSection(int /*first*/, int /*count*/) { }

void XRef::XRefPreScanWriter::writeEntry(Goffset offset, int /*gen*/, XRefEntryType /*type*/)
{
    if (offset >= 0x100000000LL) {
        hasOffsetsBeyond4GB = true;
    }
}

void XRef::writeStreamToBuffer(GooString *stmBuf, Dict *xrefDict, XRef *xref)
{
    auto index = std::make_unique<Array>(xref);
    stmBuf->clear();

    // First pass: determine whether all offsets fit in 4 bytes or not
    XRefPreScanWriter prescan;
    writeXRef(&prescan, false);
    const int offsetSize = prescan.hasOffsetsBeyond4GB ? sizeof(Goffset) : 4;

    // Second pass: actually write the xref stream
    XRefStreamWriter writer(index.get(), stmBuf, offsetSize);
    writeXRef(&writer, false);

    xrefDict->set("Type", Object(objName, "XRef"));
    xrefDict->set("Index", Object(std::move(index)));
    auto wArray = std::make_unique<Array>(xref);
    wArray->add(Object(1));
    wArray->add(Object(offsetSize));
    wArray->add(Object(2));
    xrefDict->set("W", Object(std::move(wArray)));
}

bool XRef::parseEntry(Goffset offset, XRefEntry *entry)
{
    bool r;

    if (unlikely(entry == nullptr)) {
        return false;
    }

    Parser parser(nullptr, str->makeSubStream(offset, false, 20, Object::null()), true);

    Object obj1, obj2, obj3;
    if (((obj1 = parser.getObj(), obj1.isInt()) || obj1.isInt64()) && (obj2 = parser.getObj(), obj2.isInt()) && (obj3 = parser.getObj(), obj3.isCmd("n") || obj3.isCmd("f"))) {
        if (obj1.isInt64()) {
            entry->offset = obj1.getInt64();
        } else {
            entry->offset = obj1.getInt();
        }
        entry->gen = obj2.getInt();
        entry->type = obj3.isCmd("n") ? xrefEntryUncompressed : xrefEntryFree;
        entry->obj.setToNull();
        entry->flags = 0;
        r = true;
    } else {
        r = false;
    }

    return r;
}

/* Traverse all XRef tables and, if untilEntryNum != -1, stop as soon as
 * untilEntryNum is found, or try to reconstruct the xref table if it's not
 * present in any xref.
 * If xrefStreamObjsNum is not NULL, it is filled with the list of the object
 * numbers of the XRef streams that have been traversed */
void XRef::readXRefUntil(int untilEntryNum, std::vector<int> *xrefStreamObjsNum)
{
    std::vector<Goffset> followedPrev;
    while (prevXRefOffset && (untilEntryNum == -1 || (untilEntryNum < size && entries[untilEntryNum].type == xrefEntryNone))) {
        bool followed = false;
        for (long long j : followedPrev) {
            if (j == prevXRefOffset) {
                followed = true;
                break;
            }
        }
        if (followed) {
            error(errSyntaxError, -1, "Circular XRef");
            if (!xRefStream && !(ok = constructXRef(nullptr))) {
                errCode = errDamaged;
            }
            break;
        }

        followedPrev.push_back(prevXRefOffset);

        std::vector<Goffset> followedXRefStm;
        if (!readXRef(&prevXRefOffset, &followedXRefStm, xrefStreamObjsNum)) {
            prevXRefOffset = 0;
        }

        // if there was a problem with the xref table, or we haven't found the entry
        // we were looking for, try to reconstruct the xref
        if (!ok || (!prevXRefOffset && untilEntryNum != -1 && entries[untilEntryNum].type == xrefEntryNone)) {
            if (!xRefStream && !(ok = constructXRef(nullptr))) {
                errCode = errDamaged;
                break;
            }
            break;
        }
    }
}

namespace {