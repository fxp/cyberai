// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 7/19



    if (secHdlr != nullptr && !secHdlr->isUnencrypted()) {
        yRef->setEncryption(secHdlr->getPermissionFlags(), secHdlr->getOwnerPasswordOk(), fileKey, keyLength, secHdlr->getEncVersion(), secHdlr->getEncRevision(), encAlgorithm);
    }
    const std::unique_ptr<XRef> countRef = std::make_unique<XRef>();
    Object *trailerObj = getXRef()->getTrailerDict();
    if (trailerObj->isDict()) {
        markPageObjects(trailerObj->getDict(), yRef.get(), countRef.get(), 0, refPage->num, rootNum + 2);
    }
    yRef->add(0, 65535, 0, false);
    writeHeader(outStr.get(), getPDFMajorVersion(), getPDFMinorVersion());

    // get and mark info dict
    Object infoObj = getXRef()->getDocInfo();
    if (infoObj.isDict()) {
        Dict *infoDict = infoObj.getDict();
        markPageObjects(infoDict, yRef.get(), countRef.get(), 0, refPage->num, rootNum + 2);
        if (trailerObj->isDict()) {
            Dict *trailerDict = trailerObj->getDict();
            const Object &ref = trailerDict->lookupNF("Info");
            if (ref.isRef()) {
                yRef->add(ref.getRef(), 0, true);
                if (getXRef()->getEntry(ref.getRef().num)->type == xrefEntryCompressed) {
                    yRef->getEntry(ref.getRef().num)->type = xrefEntryCompressed;
                }
            }
        }
    }

    // get and mark output intents etc.
    Object catObj = getXRef()->getCatalog();
    if (!catObj.isDict()) {
        error(errSyntaxError, -1, "XRef's Catalog is not a dictionary");
        return errOpenFile;
    }
    Dict *catDict = catObj.getDict();
    Object pagesObj = catDict->lookup("Pages");
    if (!pagesObj.isDict()) {
        error(errSyntaxError, -1, "Catalog Pages is not a dictionary");
        return errOpenFile;
    }
    Object afObj = catDict->lookupNF("AcroForm").copy();
    if (!afObj.isNull()) {
        markAcroForm(&afObj, yRef.get(), countRef.get(), 0, refPage->num, rootNum + 2);
    }
    Dict *pagesDict = pagesObj.getDict();
    Object resourcesObj = pagesDict->lookup("Resources");
    if (resourcesObj.isDict()) {
        markPageObjects(resourcesObj.getDict(), yRef.get(), countRef.get(), 0, refPage->num, rootNum + 2);
    }
    if (!markPageObjects(catDict, yRef.get(), countRef.get(), 0, refPage->num, rootNum + 2)) {
        error(errSyntaxError, -1, "markPageObjects failed");
        return errDamaged;
    }

    if (!page.isDict()) {
        error(errSyntaxError, -1, "page is not a dictionary");
        return errOpenFile;
    }
    Dict *pageDict = page.getDict();
    if (resourcesObj.isNull() && !pageDict->hasKey("Resources")) {
        Object *resourceDictObject = getCatalog()->getPage(pageNo)->getResourceDictObject();
        if (resourceDictObject->isDict()) {
            resourcesObj = resourceDictObject->copy();
            markPageObjects(resourcesObj.getDict(), yRef.get(), countRef.get(), 0, refPage->num, rootNum + 2);
        }
    }
    markPageObjects(pageDict, yRef.get(), countRef.get(), 0, refPage->num, rootNum + 2);
    Object annotsObj = pageDict->lookupNF("Annots").copy();
    if (!annotsObj.isNull()) {
        markAnnotations(&annotsObj, yRef.get(), countRef.get(), 0, refPage->num, rootNum + 2);
    }
    yRef->markUnencrypted();
    writePageObjects(outStr.get(), yRef.get(), 0);

    yRef->add(rootNum, 0, outStr->getPos(), true);
    outStr->printf("%d 0 obj\n", rootNum);
    outStr->printf("<< /Type /Catalog /Pages %d 0 R", rootNum + 1);
    for (int j = 0; j < catDict->getLength(); j++) {
        const char *key = catDict->getKey(j);
        if (strcmp(key, "Type") != 0 && strcmp(key, "Catalog") != 0 && strcmp(key, "Pages") != 0) {
            if (j > 0) {
                outStr->printf(" ");
            }
            Object value = catDict->getValNF(j).copy();
            outStr->printf("/%s ", key);
            writeObject(&value, outStr.get(), getXRef(), 0, nullptr, cryptRC4, 0, 0, 0);
        }
    }
    outStr->printf(">>\nendobj\n");

    yRef->add(rootNum + 1, 0, outStr->getPos(), true);
    outStr->printf("%d 0 obj\n", rootNum + 1);
    outStr->printf("<< /Type /Pages /Kids [ %d 0 R ] /Count 1 ", rootNum + 2);
    if (resourcesObj.isDict()) {
        outStr->printf("/Resources ");
        writeObject(&resourcesObj, outStr.get(), getXRef(), 0, nullptr, cryptRC4, 0, 0, 0);
    }
    outStr->printf(">>\n");
    outStr->printf("endobj\n");