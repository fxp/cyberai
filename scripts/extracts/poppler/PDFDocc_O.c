// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 15/19



bool PDFDoc::markAnnotations(Object *annotsObj, XRef *xRef, XRef *countRef, unsigned int numOffset, int oldPageNum, int newPageNum, std::set<Dict *> *alreadyMarkedDicts)
{
    bool modified = false;
    Object annots = annotsObj->fetch(getXRef());
    if (annots.isArray()) {
        Array *array = annots.getArray();
        for (int i = array->getLength() - 1; i >= 0; i--) {
            Object obj1 = array->get(i);
            if (obj1.isDict()) {
                Dict *dict = obj1.getDict();
                Object type = dict->lookup("Type");
                if (type.isNull()) {
                    Object subType = dict->lookup("SubType");
                    // Type is optional, subtype is required
                    // If neither of them exists, something is probably
                    // weird here, so let us just skip this entry
                    if (subType.isNull()) {
                        continue;
                    }
                }
                if (type.isName() && strcmp(type.getName(), "Annot") == 0) {
                    const Object &obj2 = dict->lookupNF("P");
                    if (obj2.isRef()) {
                        if (obj2.getRef().num == oldPageNum) {
                            const Object &obj3 = array->getNF(i);
                            if (obj3.isRef()) {
                                Ref r;
                                r.num = newPageNum;
                                r.gen = 0;
                                dict->set("P", Object(r));
                                getXRef()->setModifiedObject(&obj1, obj3.getRef());
                            }
                        } else if (obj2.getRef().num == newPageNum) {
                            continue;
                        } else {
                            Object page = getXRef()->fetch(obj2.getRef());
                            if (page.isDict()) {
                                Dict *pageDict = page.getDict();
                                Object pagetype = pageDict->lookup("Type");
                                if (!pagetype.isName() || strcmp(pagetype.getName(), "Page") != 0) {
                                    continue;
                                }
                            }
                            array->remove(i);
                            modified = true;
                            continue;
                        }
                    }
                }
                markPageObjects(dict, xRef, countRef, numOffset, oldPageNum, newPageNum, alreadyMarkedDicts);
            }
            obj1 = array->getNF(i).copy();
            if (obj1.isRef()) {
                if (obj1.getRef().num + (int)numOffset >= xRef->getNumObjects() || xRef->getEntry(obj1.getRef().num + numOffset)->type == xrefEntryFree) {
                    if (getXRef()->getEntry(obj1.getRef().num)->type == xrefEntryFree) {
                        continue; // already marked as free => should be replaced
                    }
                    xRef->add(obj1.getRef().num + numOffset, obj1.getRef().gen, 0, true);
                    if (getXRef()->getEntry(obj1.getRef().num)->type == xrefEntryCompressed) {
                        xRef->getEntry(obj1.getRef().num + numOffset)->type = xrefEntryCompressed;
                    }
                }
                if (obj1.getRef().num + (int)numOffset >= countRef->getNumObjects() || countRef->getEntry(obj1.getRef().num + numOffset)->type == xrefEntryFree) {
                    countRef->add(obj1.getRef().num + numOffset, 1, 0, true);
                } else {
                    XRefEntry *entry = countRef->getEntry(obj1.getRef().num + numOffset);
                    entry->gen++;
                }
            }
        }
    }
    if (annotsObj->isRef()) {
        if (annotsObj->getRef().num + (int)numOffset >= xRef->getNumObjects() || xRef->getEntry(annotsObj->getRef().num + numOffset)->type == xrefEntryFree) {
            if (getXRef()->getEntry(annotsObj->getRef().num)->type == xrefEntryFree) {
                return modified; // already marked as free => should be replaced
            }
            xRef->add(annotsObj->getRef().num + numOffset, annotsObj->getRef().gen, 0, true);
            if (getXRef()->getEntry(annotsObj->getRef().num)->type == xrefEntryCompressed) {
                xRef->getEntry(annotsObj->getRef().num + numOffset)->type = xrefEntryCompressed;
            }
        }
        if (annotsObj->getRef().num + (int)numOffset >= countRef->getNumObjects() || countRef->getEntry(annotsObj->getRef().num + numOffset)->type == xrefEntryFree) {
            countRef->add(annotsObj->getRef().num + numOffset, 1, 0, true);
        } else {
            XRefEntry *entry = countRef->getEntry(annotsObj->getRef().num + numOffset);
            entry->gen++;
        }
        getXRef()->setModifiedObject(&annots, annotsObj->getRef());
    }
    return modified;
}