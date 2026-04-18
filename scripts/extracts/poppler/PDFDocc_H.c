// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 8/19



    yRef->add(rootNum + 2, 0, outStr->getPos(), true);
    outStr->printf("%d 0 obj\n", rootNum + 2);
    outStr->printf("<< ");
    for (int n = 0; n < pageDict->getLength(); n++) {
        if (n > 0) {
            outStr->printf(" ");
        }
        const char *key = pageDict->getKey(n);
        Object value = pageDict->getValNF(n).copy();
        if (strcmp(key, "Parent") == 0) {
            outStr->printf("/Parent %d 0 R", rootNum + 1);
        } else {
            outStr->printf("/%s ", key);
            writeObject(&value, outStr.get(), getXRef(), 0, nullptr, cryptRC4, 0, 0, 0);
        }
    }
    outStr->printf(" >>\nendobj\n");

    Goffset uxrefOffset = outStr->getPos();
    Ref ref;
    ref.num = rootNum;
    ref.gen = 0;
    Object trailerDict = createTrailerDict(rootNum + 3, false, 0, &ref, getXRef(), name.c_str(), uxrefOffset);
    writeXRefTableTrailer(std::move(trailerDict), yRef.get(), false /* do not write unnecessary entries */, uxrefOffset, outStr.get(), getXRef());

    outStr->close();

    return errNone;
}

int PDFDoc::saveAs(const std::string &name, PDFWriteMode mode)
{
    FILE *f;
    OutStream *outStr;
    int res;

    if (!(f = openFile(name.c_str(), "wb"))) {
        error(errIO, -1, "Couldn't open file '{0:s}'", name.c_str());
        return errOpenFile;
    }
    outStr = new FileOutStream(f, 0);
    res = saveAs(outStr, mode);
    delete outStr;
    fclose(f);
    return res;
}

int PDFDoc::saveAs(OutStream *outStr, PDFWriteMode mode)
{
    if (file && file->modificationTimeChangedSinceOpen()) {
        return errFileChangedSinceOpen;
    }

    if (!xref->isModified() && mode == writeStandard) {
        // simply copy the original file
        saveWithoutChangesAs(outStr);
    } else if (mode == writeForceRewrite) {
        saveCompleteRewrite(outStr);
    } else {
        saveIncrementalUpdate(outStr);
    }

    return errNone;
}

int PDFDoc::saveWithoutChangesAs(const std::string &name)
{
    FILE *f;
    OutStream *outStr;
    int res;

    if (!(f = openFile(name.c_str(), "wb"))) {
        error(errIO, -1, "Couldn't open file '{0:s}'", name.c_str());
        return errOpenFile;
    }

    outStr = new FileOutStream(f, 0);
    res = saveWithoutChangesAs(outStr);
    delete outStr;

    fclose(f);

    return res;
}

int PDFDoc::saveWithoutChangesAs(OutStream *outStr)
{
    if (file && file->modificationTimeChangedSinceOpen()) {
        return errFileChangedSinceOpen;
    }

    std::unique_ptr<BaseStream> copyStr { str->copy() };
    if (!copyStr->rewind()) {
        return errFileIO;
    }
    while (copyStr->lookChar() != EOF) {
        std::array<unsigned char, 4096> array;
        size_t size = copyStr->doGetChars(array.size(), array.data());
        auto sizeWritten = outStr->write(std::span(array.data(), size));
        if (size != sizeWritten) {
            return errFileIO;
        }
    }
    copyStr->close();

    return errNone;
}

void PDFDoc::saveIncrementalUpdate(OutStream *outStr)
{
    // copy the original file
    std::unique_ptr<BaseStream> copyStr { str->copy() };
    if (!copyStr->rewind()) {
        // some err;
    }
    while (copyStr->lookChar() != EOF) {
        std::array<unsigned char, 4096> array;
        size_t size = copyStr->doGetChars(array.size(), array.data());
        auto sizeWritten = outStr->write(std::span(array.data(), size));
        if (size != sizeWritten) {
            // Write error of some sort
        }
    }
    copyStr->close();

    unsigned char *fileKey;
    CryptAlgorithm encAlgorithm;
    int keyLength;
    xref->getEncryptionParameters(&fileKey, &encAlgorithm, &keyLength);

    XRef *uxref = new XRef();
    uxref->add(0, 65535, 0, false);
    xref->lock();
    for (int i = 0; i < xref->getNumObjects(); i++) {
        if ((xref->getEntry(i)->type == xrefEntryFree) && (xref->getEntry(i)->gen == 0)) { // we skip the irrelevant free objects
            continue;
        }

        if (xref->getEntry(i)->getFlag(XRefEntry::Updated)) { // we have an updated object
            Ref ref;
            ref.num = i;
            ref.gen = xref->getEntry(i)->type == xrefEntryCompressed ? 0 : xref->getEntry(i)->gen;
            if (xref->getEntry(i)->type != xrefEntryFree) {
                Object obj1 = xref->fetch(ref, 1 /* recursion */);
                Goffset offset = writeObjectHeader(&ref, outStr);
                writeObject(&obj1, outStr, fileKey, encAlgorithm, keyLength, ref);
                writeObjectFooter(outStr);
                uxref->add(ref, offset, true);
            } else {
                uxref->add(ref, 0, false);
            }
        }
    }
    xref->unlock();
    // because of "uxref->add(0, 65535, 0, false);" uxref->getNumObjects() will
    // always be >= 1; if it is 1, it means there is nothing to update
    if (uxref->getNumObjects() == 1) {
        delete uxref;
        return;
    }