// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 16/19



void PDFDoc::markAcroForm(Object *afObj, XRef *xRef, XRef *countRef, unsigned int numOffset, int oldRefNum, int newRefNum)
{
    bool modified = false;
    Object acroform = afObj->fetch(getXRef());
    if (acroform.isDict()) {
        Dict *dict = acroform.getDict();
        for (int i = 0; i < dict->getLength(); i++) {
            if (strcmp(dict->getKey(i), "Fields") == 0) {
                Object fields = dict->getValNF(i).copy();
                modified = markAnnotations(&fields, xRef, countRef, numOffset, oldRefNum, newRefNum);
            } else {
                Object obj = dict->getValNF(i).copy();
                markObject(&obj, xRef, countRef, numOffset, oldRefNum, newRefNum);
            }
        }
    }
    if (afObj->isRef()) {
        if (afObj->getRef().num + (int)numOffset >= xRef->getNumObjects() || xRef->getEntry(afObj->getRef().num + numOffset)->type == xrefEntryFree) {
            if (getXRef()->getEntry(afObj->getRef().num)->type == xrefEntryFree) {
                return; // already marked as free => should be replaced
            }
            xRef->add(afObj->getRef().num + numOffset, afObj->getRef().gen, 0, true);
            if (getXRef()->getEntry(afObj->getRef().num)->type == xrefEntryCompressed) {
                xRef->getEntry(afObj->getRef().num + numOffset)->type = xrefEntryCompressed;
            }
        }
        if (afObj->getRef().num + (int)numOffset >= countRef->getNumObjects() || countRef->getEntry(afObj->getRef().num + numOffset)->type == xrefEntryFree) {
            countRef->add(afObj->getRef().num + numOffset, 1, 0, true);
        } else {
            XRefEntry *entry = countRef->getEntry(afObj->getRef().num + numOffset);
            entry->gen++;
        }
        if (modified) {
            getXRef()->setModifiedObject(&acroform, afObj->getRef());
        }
    }
}

unsigned int PDFDoc::writePageObjects(OutStream *outStr, XRef *xRef, unsigned int numOffset, bool combine)
{
    unsigned int objectsCount = 0; // count the number of objects in the XRef(s)
    unsigned char *fileKey;
    CryptAlgorithm encAlgorithm;
    int keyLength;
    xRef->getEncryptionParameters(&fileKey, &encAlgorithm, &keyLength);

    for (int n = numOffset; n < xRef->getNumObjects(); n++) {
        if (xRef->getEntry(n)->type != xrefEntryFree) {
            Ref ref;
            ref.num = n;
            ref.gen = xRef->getEntry(n)->gen;
            objectsCount++;
            Object obj = getXRef()->fetch(ref.num - numOffset, ref.gen);
            Goffset offset = writeObjectHeader(&ref, outStr);
            if (combine) {
                writeObject(&obj, outStr, getXRef(), numOffset, nullptr, cryptRC4, 0, 0, 0);
            } else if (xRef->getEntry(n)->getFlag(XRefEntry::Unencrypted)) {
                writeObject(&obj, outStr, nullptr, cryptRC4, 0, 0, 0);
            } else {
                writeObject(&obj, outStr, fileKey, encAlgorithm, keyLength, ref);
            }
            writeObjectFooter(outStr);
            xRef->add(ref, offset, true);
        }
    }
    return objectsCount;
}

Outline *PDFDoc::getOutline()
{
    if (!outline) {
        pdfdocLocker();
        // read outline
        outline = new Outline(catalog->getOutline(), xref, this);
    }

    return outline;
}

std::unique_ptr<PDFDoc> PDFDoc::ErrorPDFDoc(int errorCode, std::unique_ptr<GooString> &&fileNameA)
{
    // We cannot call std::make_unique here because the PDFDoc constructor is private
    auto *doc = new PDFDoc();
    doc->errCode = errorCode;
    doc->fileName = std::move(fileNameA);

    return std::unique_ptr<PDFDoc>(doc);
}

long long PDFDoc::strToLongLong(const char *s)
{
    long long x, d;
    const char *p;

    x = 0;
    for (p = s; *p && isdigit(*p & 0xff); ++p) {
        d = *p - '0';
        if (x > (LLONG_MAX - d) / 10) {
            break;
        }
        x = 10 * x + d;
    }
    return x;
}

// Read the 'startxref' position.
Goffset PDFDoc::getStartXRef(bool tryingToReconstruct)
{
    if (startXRefPos == -1) {

        if (isLinearized(tryingToReconstruct)) {
            char buf[linearizationSearchSize + 1];
            int c, n, i;

            str->setPos(0);
            for (n = 0; n < linearizationSearchSize; ++n) {
                if ((c = str->getChar()) == EOF) {
                    break;
                }
                buf[n] = c;
            }
            buf[n] = '\0';

            // find end of first obj (linearization dictionary)
            startXRefPos = 0;
            for (i = 0; i < n; i++) {
                if (!strncmp("endobj", &buf[i], 6)) {
                    i += 6;
                    // skip whitespace
                    while (buf[i] && Lexer::isSpace(buf[i])) {
                        ++i;
                    }
                    startXRefPos = i;
                    break;
                }
            }
        } else {
            char buf[xrefSearchSize + 1];
            const char *p;
            int c, n, i;