// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 9/19



    Goffset uxrefOffset = outStr->getPos();
    int numobjects = xref->getNumObjects();
    const char *fileNameA = fileName ? fileName->c_str() : nullptr;
    Ref rootRef, uxrefStreamRef;
    rootRef.num = getXRef()->getRootNum();
    rootRef.gen = getXRef()->getRootGen();

    // Output a xref stream if there is a xref stream already
    bool xRefStream = xref->isXRefStream();

    if (xRefStream) {
        // Append an entry for the xref stream itself
        uxrefStreamRef.num = numobjects++;
        uxrefStreamRef.gen = 0;
        uxref->add(uxrefStreamRef, uxrefOffset, true);
    }

    Object trailerDict = createTrailerDict(numobjects, true, getStartXRef(), &rootRef, getXRef(), fileNameA, uxrefOffset);
    if (xRefStream) {
        writeXRefStreamTrailer(std::move(trailerDict), uxref, &uxrefStreamRef, uxrefOffset, outStr, getXRef());
    } else {
        writeXRefTableTrailer(std::move(trailerDict), uxref, false, uxrefOffset, outStr, getXRef());
    }

    delete uxref;
}

void PDFDoc::saveCompleteRewrite(OutStream *outStr)
{
    // Make sure that special flags are set, because we are going to read
    // all objects, including Unencrypted ones.
    xref->scanSpecialFlags();

    unsigned char *fileKey;
    CryptAlgorithm encAlgorithm;
    int keyLength;
    xref->getEncryptionParameters(&fileKey, &encAlgorithm, &keyLength);

    writeHeader(outStr, getPDFMajorVersion(), getPDFMinorVersion());
    XRef *uxref = new XRef();
    uxref->add(0, 65535, 0, false);
    xref->lock();
    for (int i = 0; i < xref->getNumObjects(); i++) {
        Ref ref;
        XRefEntryType type = xref->getEntry(i)->type;
        if (type == xrefEntryFree) {
            ref.num = i;
            ref.gen = xref->getEntry(i)->gen;
            /* the XRef class adds a lot of irrelevant free entries, we only want the significant one
                and we don't want the one with num=0 because it has already been added (gen = 65535)*/
            if (ref.gen > 0 && ref.num > 0) {
                uxref->add(ref, 0, false);
            }
        } else if (xref->getEntry(i)->getFlag(XRefEntry::DontRewrite)) {
            // This entry must not be written, put a free entry instead (with incremented gen)
            ref.num = i;
            ref.gen = xref->getEntry(i)->gen + 1;
            uxref->add(ref, 0, false);
        } else if (type == xrefEntryUncompressed) {
            ref.num = i;
            ref.gen = xref->getEntry(i)->gen;
            Object obj1 = xref->fetch(ref, 1 /* recursion */);
            Goffset offset = writeObjectHeader(&ref, outStr);
            // Write unencrypted objects in unencrypted form
            if (xref->getEntry(i)->getFlag(XRefEntry::Unencrypted)) {
                writeObject(&obj1, outStr, nullptr, cryptRC4, 0, 0, 0);
            } else {
                writeObject(&obj1, outStr, fileKey, encAlgorithm, keyLength, ref);
            }
            writeObjectFooter(outStr);
            uxref->add(ref, offset, true);
        } else if (type == xrefEntryCompressed) {
            ref.num = i;
            ref.gen = 0; // compressed entries have gen == 0
            Object obj1 = xref->fetch(ref, 1 /* recursion */);
            Goffset offset = writeObjectHeader(&ref, outStr);
            writeObject(&obj1, outStr, fileKey, encAlgorithm, keyLength, ref);
            writeObjectFooter(outStr);
            uxref->add(ref, offset, true);
        }
    }
    xref->unlock();
    Goffset uxrefOffset = outStr->getPos();
    writeXRefTableTrailer(uxrefOffset, uxref, true /* write all entries */, uxref->getNumObjects(), outStr, false /* complete rewrite */);
    delete uxref;
}

std::string PDFDoc::sanitizedName(const std::string &name)
{
    std::string sanitizedName;

    for (const auto c : name) {
        if (c <= (char)0x20 || c >= (char)0x7f || c == ' ' || c == '(' || c == ')' || c == '<' || c == '>' || c == '[' || c == ']' || c == '{' || c == '}' || c == '/' || c == '%' || c == '#') {
            char buf[8];
            sprintf(buf, "#%02x", c & 0xff);
            sanitizedName.append(buf);
        } else {
            sanitizedName.push_back(c);
        }
    }

    return sanitizedName;
}

void PDFDoc::writeDictionary(Dict *dict, OutStream *outStr, XRef *xRef, unsigned int numOffset, const unsigned char *fileKey, CryptAlgorithm encAlgorithm, int keyLength, Ref ref, std::set<Dict *> *alreadyWrittenDicts)
{
    bool deleteSet = false;
    if (!alreadyWrittenDicts) {
        alreadyWrittenDicts = new std::set<Dict *>;
        deleteSet = true;
    }

    const auto [_, inserted] = alreadyWrittenDicts->insert(dict);
    if (!inserted) {
        error(errSyntaxWarning, -1, "PDFDoc::writeDictionary: Found recursive dicts");
        if (deleteSet) {
            delete alreadyWrittenDicts;
        }
        return;
    }