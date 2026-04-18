// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 13/19



    if (!xRef->getTrailerDict()->isNone()) {
        Object obj5 = xRef->getDocInfoNF();
        if (!obj5.isNull()) {
            trailerDict->set("Info", std::move(obj5));
        }
    }

    return Object(std::move(trailerDict));
}

void PDFDoc::writeXRefTableTrailer(Object &&trailerDict, XRef *uxref, bool writeAllEntries, Goffset uxrefOffset, OutStream *outStr, XRef *xRef)
{
    uxref->writeTableToFile(outStr, writeAllEntries);
    outStr->printf("trailer\r\n");
    writeDictionary(trailerDict.getDict(), outStr, xRef, 0, nullptr, cryptRC4, 0, { .num = 0, .gen = 0 }, nullptr);
    outStr->printf("\r\nstartxref\r\n");
    outStr->printf("%lli\r\n", uxrefOffset);
    outStr->printf("%%%%EOF\r\n");
}

void PDFDoc::writeXRefStreamTrailer(Object &&trailerDict, XRef *uxref, Ref *uxrefStreamRef, Goffset uxrefOffset, OutStream *outStr, XRef *xRef)
{
    GooString stmData;

    // Fill stmData and some trailerDict fields
    uxref->writeStreamToBuffer(&stmData, trailerDict.getDict(), xRef);

    // Create XRef stream object and write it
    auto mStream = std::make_unique<MemStream>(stmData.c_str(), 0, stmData.size(), std::move(trailerDict));
    writeObjectHeader(uxrefStreamRef, outStr);
    Object obj1(std::move(mStream));
    writeObject(&obj1, outStr, xRef, 0, nullptr, cryptRC4, 0, 0, 0);
    writeObjectFooter(outStr);

    outStr->printf("startxref\r\n");
    outStr->printf("%lli\r\n", uxrefOffset);
    outStr->printf("%%%%EOF\r\n");
}

void PDFDoc::writeXRefTableTrailer(Goffset uxrefOffset, XRef *uxref, bool writeAllEntries, int uxrefSize, OutStream *outStr, bool incrUpdate)
{
    const char *fileNameA = fileName ? fileName->c_str() : nullptr;
    // file size (doesn't include the trailer)
    unsigned int fileSize = 0;
    int c;
    if (!str->rewind()) {
        return;
    }
    while ((c = str->getChar()) != EOF) {
        fileSize++;
    }
    str->close();
    Ref ref;
    ref.num = getXRef()->getRootNum();
    ref.gen = getXRef()->getRootGen();
    Object trailerDict = createTrailerDict(uxrefSize, incrUpdate, getStartXRef(), &ref, getXRef(), fileNameA, fileSize);
    writeXRefTableTrailer(std::move(trailerDict), uxref, writeAllEntries, uxrefOffset, outStr, getXRef());
}

void PDFDoc::writeHeader(OutStream *outStr, int major, int minor)
{
    outStr->printf("%%PDF-%d.%d\n", major, minor);
    outStr->printf("%%%c%c%c%c\n", 0xE2, 0xE3, 0xCF, 0xD3);
}

bool PDFDoc::markDictionary(Dict *dict, XRef *xRef, XRef *countRef, unsigned int numOffset, int oldRefNum, int newRefNum, std::set<Dict *> *alreadyMarkedDicts)
{
    bool deleteSet = false;
    if (!alreadyMarkedDicts) {
        alreadyMarkedDicts = new std::set<Dict *>;
        deleteSet = true;
    }

    const auto [_, inserted] = alreadyMarkedDicts->insert(dict);
    if (!inserted) {
        error(errSyntaxWarning, -1, "PDFDoc::markDictionary: Found recursive dicts");
        if (deleteSet) {
            delete alreadyMarkedDicts;
        }
        return true;
    }

    for (int i = 0; i < dict->getLength(); i++) {
        const char *key = dict->getKey(i);
        if (strcmp(key, "Annots") != 0) {
            Object obj1 = dict->getValNF(i).copy();
            const bool success = markObject(&obj1, xRef, countRef, numOffset, oldRefNum, newRefNum, alreadyMarkedDicts);
            if (unlikely(!success)) {
                return false;
            }
        } else {
            Object annotsObj = dict->getValNF(i).copy();
            if (!annotsObj.isNull()) {
                markAnnotations(&annotsObj, xRef, countRef, 0, oldRefNum, newRefNum, alreadyMarkedDicts);
            }
        }
    }

    if (deleteSet) {
        delete alreadyMarkedDicts;
    }

    return true;
}

bool PDFDoc::markObject(Object *obj, XRef *xRef, XRef *countRef, unsigned int numOffset, int oldRefNum, int newRefNum, std::set<Dict *> *alreadyMarkedDicts)
{
    Array *array;