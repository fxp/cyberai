// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 14/19



    switch (obj->getType()) {
    case objArray:
        array = obj->getArray();
        for (int i = 0; i < array->getLength(); i++) {
            Object obj1 = array->getNF(i).copy();
            const bool success = markObject(&obj1, xRef, countRef, numOffset, oldRefNum, newRefNum, alreadyMarkedDicts);
            if (unlikely(!success)) {
                return false;
            }
        }
        break;
    case objDict: {
        const bool success = markDictionary(obj->getDict(), xRef, countRef, numOffset, oldRefNum, newRefNum, alreadyMarkedDicts);
        if (unlikely(!success)) {
            return false;
        }
    } break;
    case objStream: {
        Stream *stream = obj->getStream();
        const bool success = markDictionary(stream->getDict(), xRef, countRef, numOffset, oldRefNum, newRefNum, alreadyMarkedDicts);
        if (unlikely(!success)) {
            return false;
        }
    } break;
    case objRef: {
        if (obj->getRef().num + (int)numOffset >= xRef->getNumObjects() || xRef->getEntry(obj->getRef().num + numOffset)->type == xrefEntryFree) {
            if (getXRef()->getEntry(obj->getRef().num)->type == xrefEntryFree) {
                return true; // already marked as free => should be replaced
            }
            const bool success = xRef->add(obj->getRef().num + numOffset, obj->getRef().gen, 0, true);
            if (unlikely(!success)) {
                return false;
            }
            if (getXRef()->getEntry(obj->getRef().num)->type == xrefEntryCompressed) {
                xRef->getEntry(obj->getRef().num + numOffset)->type = xrefEntryCompressed;
            }
        }
        if (obj->getRef().num + (int)numOffset >= countRef->getNumObjects() || countRef->getEntry(obj->getRef().num + numOffset)->type == xrefEntryFree) {
            countRef->add(obj->getRef().num + numOffset, 1, 0, true);
        } else {
            XRefEntry *entry = countRef->getEntry(obj->getRef().num + numOffset);
            entry->gen++;
            if (entry->gen > 9) {
                break;
            }
        }
        Object obj1 = getXRef()->fetch(obj->getRef());
        const bool success = markObject(&obj1, xRef, countRef, numOffset, oldRefNum, newRefNum);
        if (unlikely(!success)) {
            return false;
        }
    } break;
    default:
        break;
    }

    return true;
}

bool PDFDoc::replacePageDict(int pageNo, int rotate, const PDFRectangle *mediaBox, const PDFRectangle *cropBox) const
{
    Ref *refPage = getCatalog()->getPageRef(pageNo);
    Object page = getXRef()->fetch(*refPage);
    if (!page.isDict()) {
        return false;
    }
    Dict *pageDict = page.getDict();
    pageDict->remove("MediaBoxssdf");
    pageDict->remove("MediaBox");
    pageDict->remove("CropBox");
    pageDict->remove("ArtBox");
    pageDict->remove("BleedBox");
    pageDict->remove("TrimBox");
    pageDict->remove("Rotate");
    auto mediaBoxArray = std::make_unique<Array>(getXRef());
    mediaBoxArray->add(Object(mediaBox->x1));
    mediaBoxArray->add(Object(mediaBox->y1));
    mediaBoxArray->add(Object(mediaBox->x2));
    mediaBoxArray->add(Object(mediaBox->y2));
    Object mediaBoxObject(std::move(mediaBoxArray));
    Object trimBoxObject = mediaBoxObject.copy();
    pageDict->add("MediaBox", std::move(mediaBoxObject));
    if (cropBox != nullptr) {
        auto cropBoxArray = std::make_unique<Array>(getXRef());
        cropBoxArray->add(Object(cropBox->x1));
        cropBoxArray->add(Object(cropBox->y1));
        cropBoxArray->add(Object(cropBox->x2));
        cropBoxArray->add(Object(cropBox->y2));
        Object cropBoxObject(std::move(cropBoxArray));
        trimBoxObject = cropBoxObject.copy();
        pageDict->add("CropBox", std::move(cropBoxObject));
    }
    pageDict->add("TrimBox", std::move(trimBoxObject));
    pageDict->add("Rotate", Object(rotate));
    getXRef()->setModifiedObject(&page, *refPage);
    return true;
}

bool PDFDoc::markPageObjects(Dict *pageDict, XRef *xRef, XRef *countRef, unsigned int numOffset, int oldRefNum, int newRefNum, std::set<Dict *> *alreadyMarkedDicts)
{
    pageDict->remove("OpenAction");
    pageDict->remove("Outlines");
    pageDict->remove("StructTreeRoot");

    for (int n = 0; n < pageDict->getLength(); n++) {
        const char *key = pageDict->getKey(n);
        Object value = pageDict->getValNF(n).copy();
        if (strcmp(key, "Parent") != 0 && strcmp(key, "Pages") != 0 && strcmp(key, "AcroForm") != 0 && strcmp(key, "Annots") != 0 && strcmp(key, "P") != 0 && strcmp(key, "Root") != 0) {
            const bool success = markObject(&value, xRef, countRef, numOffset, oldRefNum, newRefNum, alreadyMarkedDicts);
            if (unlikely(!success)) {
                return false;
            }
        }
    }
    return true;
}