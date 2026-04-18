// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 6/19



    Object infoObj = getDocInfo();
    if (infoObj.isNull() && removeEntry) {
        // No info dictionary, so no entry to remove.
        return;
    }

    Ref infoObjRef;
    infoObj = xref->createDocInfoIfNeeded(&infoObjRef);
    if (removeEntry) {
        infoObj.dictSet(key, Object::null());
    } else {
        infoObj.dictSet(key, Object(std::move(value)));
    }

    if (infoObj.dictGetLength() == 0) {
        // Info dictionary is empty. Remove it altogether.
        removeDocInfo();
    } else {
        xref->setModifiedObject(&infoObj, infoObjRef);
    }
}

std::unique_ptr<GooString> PDFDoc::getDocInfoStringEntry(const char *key)
{
    Object infoObj = getDocInfo();
    if (!infoObj.isDict()) {
        return {};
    }

    Object entryObj = infoObj.dictLookup(key);
    if (!entryObj.isString()) {
        return {};
    }

    return entryObj.takeString();
}

static bool get_id(const std::string &encodedidstring, GooString *id)
{
    const char *encodedid = encodedidstring.c_str();
    char pdfid[pdfIdLength + 1];
    int n;

    if (encodedidstring.size() != pdfIdLength / 2) {
        return false;
    }

    n = sprintf(pdfid, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", encodedid[0] & 0xff, encodedid[1] & 0xff, encodedid[2] & 0xff, encodedid[3] & 0xff, encodedid[4] & 0xff, encodedid[5] & 0xff, encodedid[6] & 0xff,
                encodedid[7] & 0xff, encodedid[8] & 0xff, encodedid[9] & 0xff, encodedid[10] & 0xff, encodedid[11] & 0xff, encodedid[12] & 0xff, encodedid[13] & 0xff, encodedid[14] & 0xff, encodedid[15] & 0xff);
    if (n != pdfIdLength) {
        return false;
    }

    id->assign(pdfid, pdfIdLength);
    return true;
}

bool PDFDoc::getID(GooString *permanent_id, GooString *update_id) const
{
    Object obj = xref->getTrailerDict()->dictLookup("ID");

    if (obj.isArrayOfLength(2)) {
        if (permanent_id) {
            Object obj2 = obj.arrayGet(0);
            if (obj2.isString()) {
                if (!get_id(obj2.getString(), permanent_id)) {
                    return false;
                }
            } else {
                error(errSyntaxError, -1, "Invalid permanent ID");
                return false;
            }
        }

        if (update_id) {
            Object obj2 = obj.arrayGet(1);
            if (obj2.isString()) {
                if (!get_id(obj2.getString(), update_id)) {
                    return false;
                }
            } else {
                error(errSyntaxError, -1, "Invalid update ID");
                return false;
            }
        }

        return true;
    }

    return false;
}

Hints *PDFDoc::getHints()
{
    if (!hints && isLinearized()) {
        hints = new Hints(str.get(), getLinearization(), getXRef(), secHdlr);
    }

    return hints;
}

int PDFDoc::savePageAs(const std::string &name, int pageNo)
{
    FILE *f;

    if (file && file->modificationTimeChangedSinceOpen()) {
        return errFileChangedSinceOpen;
    }

    int rootNum = getXRef()->getNumObjects() + 1;

    // Make sure that special flags are set, because we are going to read
    // all objects, including Unencrypted ones.
    xref->scanSpecialFlags();

    unsigned char *fileKey;
    CryptAlgorithm encAlgorithm;
    int keyLength;
    xref->getEncryptionParameters(&fileKey, &encAlgorithm, &keyLength);

    if (pageNo < 1 || pageNo > getNumPages() || !getCatalog()->getPage(pageNo)) {
        error(errInternal, -1, "Illegal pageNo: {0:d}({1:d})", pageNo, getNumPages());
        return errOpenFile;
    }
    const PDFRectangle *cropBox = nullptr;
    if (getCatalog()->getPage(pageNo)->isCropped()) {
        cropBox = getCatalog()->getPage(pageNo)->getCropBox();
    }
    replacePageDict(pageNo, getCatalog()->getPage(pageNo)->getRotate(), getCatalog()->getPage(pageNo)->getMediaBox(), cropBox);
    Ref *refPage = getCatalog()->getPageRef(pageNo);
    Object page = getXRef()->fetch(*refPage);

    if (!(f = openFile(name.c_str(), "wb"))) {
        error(errIO, -1, "Couldn't open file '{0:s}'", name.c_str());
        return errOpenFile;
    }
    // Calls fclose on f when the fileCloser is destroyed because it goes out of scope
    const std::unique_ptr<FILE, FILECloser> fileCloser(f);
    const std::unique_ptr<OutStream> outStr = std::make_unique<FileOutStream>(f, 0);

    const std::unique_ptr<XRef> yRef = std::make_unique<XRef>(getXRef()->getTrailerDict());