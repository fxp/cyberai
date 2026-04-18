// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 3/19



    // read xref table
    xref = new XRef(str.get(), getStartXRef(), getMainXRefEntriesOffset(), &wasReconstructed, false, xrefReconstructedCallback);
    if (!xref->isOk()) {
        if (wasReconstructed) {
            delete xref;
            startXRefPos = -1;
            xref = new XRef(str.get(), getStartXRef(true), getMainXRefEntriesOffset(true), &wasReconstructed, false, xrefReconstructedCallback);
        }
        if (!xref->isOk()) {
            error(errSyntaxError, -1, "Couldn't read xref table");
            errCode = xref->getErrorCode();
            return false;
        }
    }

    // check for encryption
    if (!checkEncryption(ownerPassword, userPassword)) {
        errCode = errEncrypted;
        return false;
    }

    // read catalog
    catalog = new Catalog(this);
    if (catalog && !catalog->isOk()) {
        if (!wasReconstructed) {
            // try one more time to construct the Catalog, maybe the problem is damaged XRef
            delete catalog;
            delete xref;
            xref = new XRef(str.get(), 0, 0, nullptr, true, xrefReconstructedCallback);
            catalog = new Catalog(this);
        }

        if (catalog && !catalog->isOk()) {
            error(errSyntaxError, -1, "Couldn't read page catalog");
            errCode = errBadCatalog;
            return false;
        }
    }

    // Extract PDF Subtype information
    extractPDFSubtype();

    // done
    return true;
}

PDFDoc::~PDFDoc()
{
    delete secHdlr;
    delete outline;
    delete catalog;
    delete xref;
    delete hints;
    delete linearization;
}

// Check for a %%EOF at the end of this stream
bool PDFDoc::checkFooter()
{
    // we look in the last 1024 chars because Adobe does the same
    char *eof = new char[1025];
    Goffset pos = str->getPos();
    str->setPos(1024, -1);
    int i, ch;
    for (i = 0; i < 1024; i++) {
        ch = str->getChar();
        if (ch == EOF) {
            break;
        }
        eof[i] = ch;
    }
    eof[i] = '\0';

    bool found = false;
    for (i = i - 5; i >= 0; i--) {
        if (strncmp(&eof[i], "%%EOF", 5) == 0) {
            found = true;
            break;
        }
    }
    if (!found) {
        error(errSyntaxError, -1, "Document has not the mandatory ending %%EOF");
        errCode = errDamaged;
        delete[] eof;
        return false;
    }
    delete[] eof;
    str->setPos(pos);
    return true;
}

// Check for a PDF header on this stream.  Skip past some garbage
// if necessary.
void PDFDoc::checkHeader()
{
    char hdrBuf[headerSearchSize + 1];
    char *p;
    char *tokptr;
    int i;
    int bytesRead;

    headerPdfMajorVersion = 0;
    headerPdfMinorVersion = 0;

    // read up to headerSearchSize bytes from the beginning of the document
    for (i = 0; i < headerSearchSize; ++i) {
        const int c = str->getChar();
        if (c == EOF) {
            break;
        }
        hdrBuf[i] = c;
    }
    bytesRead = i;
    hdrBuf[bytesRead] = '\0';

    // find the start of the PDF header if it exists and parse the version
    bool headerFound = false;
    for (i = 0; i < bytesRead - 5; ++i) {
        if (!strncmp(&hdrBuf[i], "%PDF-", 5)) {
            headerFound = true;
            break;
        }
    }
    if (!headerFound) {
        error(errSyntaxWarning, -1, "May not be a PDF file (continuing anyway)");
        return;
    }
    str->moveStart(i);
    if (!(p = strtok_r(&hdrBuf[i + 5], " \t\n\r", &tokptr))) {
        error(errSyntaxWarning, -1, "May not be a PDF file (continuing anyway)");
        return;
    }
    sscanf(p, "%d.%d", &headerPdfMajorVersion, &headerPdfMinorVersion);
    // We don't do the version check. Don't add it back in.
}

bool PDFDoc::checkEncryption(const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword)
{
    bool encrypted;
    bool ret;

    Object encrypt = xref->getTrailerDict()->dictLookup("Encrypt");
    if ((encrypted = encrypt.isDict())) {
        if ((secHdlr = SecurityHandler::make(this, &encrypt))) {
            if (secHdlr->isUnencrypted()) {
                // no encryption
                ret = true;
            } else if (secHdlr->checkEncryption(ownerPassword, userPassword)) {
                // authorization succeeded
                xref->setEncryption(secHdlr->getPermissionFlags(), secHdlr->getOwnerPasswordOk(), secHdlr->getFileKey(), secHdlr->getFileKeyLength(), secHdlr->getEncVersion(), secHdlr->getEncRevision(), secHdlr->getEncAlgorithm());
                ret = true;
            } else {
                // authorization failed
                ret = false;
            }
        } else {
            // couldn't find the matching security handler
            ret = false;
        }
    } else {
        // document is not encrypted
        ret = true;
    }
    return ret;
}