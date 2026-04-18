// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 5/19



    // First search
    const Form *f = catalog->getForm();
    if (f) {
        const int nRootFields = f->getNumFields();
        for (int i = 0; i < nRootFields; ++i) {
            FormField *ff = f->getRootField(i);
            addSignatureFieldsToVector(ff, res);
        }
    }

    // Second search
    for (int page = 1; page <= getNumPages(); ++page) {
        Page *p = getPage(page);
        if (p) {
            const std::unique_ptr<FormPageWidgets> pw = p->getFormWidgets();
            for (int i = 0; i < pw->getNumWidgets(); ++i) {
                FormWidget *fw = pw->getWidget(i);
                if (fw->getType() == formSignature) {
                    assert(fw->getField()->getType() == formSignature);
                    auto *ffs = static_cast<FormFieldSignature *>(fw->getField());
                    if (std::ranges::find(res, ffs) == res.end()) {
                        res.push_back(ffs);
                    }
                }
            }
        }
    }

    return res;
}

void PDFDoc::displayPage(OutputDev *out, int page, double hDPI, double vDPI, int rotate, bool useMediaBox, bool crop, bool printing, bool (*abortCheckCbk)(void *data), void *abortCheckCbkData,
                         bool (*annotDisplayDecideCbk)(Annot *annot, void *user_data), void *annotDisplayDecideCbkData, bool copyXRef)
{
    if (globalParams->getPrintCommands()) {
        printf("***** page %d *****\n", page);
    }

    if (getPage(page)) {
        getPage(page)->display(out, hDPI, vDPI, rotate, useMediaBox, crop, printing, abortCheckCbk, abortCheckCbkData, annotDisplayDecideCbk, annotDisplayDecideCbkData, copyXRef);
    }
}

void PDFDoc::displayPages(OutputDev *out, int firstPage, int lastPage, double hDPI, double vDPI, int rotate, bool useMediaBox, bool crop, bool printing, bool (*abortCheckCbk)(void *data), void *abortCheckCbkData,
                          bool (*annotDisplayDecideCbk)(Annot *annot, void *user_data), void *annotDisplayDecideCbkData)
{
    int page;

    for (page = firstPage; page <= lastPage; ++page) {
        displayPage(out, page, hDPI, vDPI, rotate, useMediaBox, crop, printing, abortCheckCbk, abortCheckCbkData, annotDisplayDecideCbk, annotDisplayDecideCbkData);
    }
}

void PDFDoc::displayPageSlice(OutputDev *out, int page, double hDPI, double vDPI, int rotate, bool useMediaBox, bool crop, bool printing, int sliceX, int sliceY, int sliceW, int sliceH, bool (*abortCheckCbk)(void *data),
                              void *abortCheckCbkData, bool (*annotDisplayDecideCbk)(Annot *annot, void *user_data), void *annotDisplayDecideCbkData, bool copyXRef)
{
    if (getPage(page)) {
        getPage(page)->displaySlice(out, hDPI, vDPI, rotate, useMediaBox, crop, sliceX, sliceY, sliceW, sliceH, printing, abortCheckCbk, abortCheckCbkData, annotDisplayDecideCbk, annotDisplayDecideCbkData, copyXRef);
    }
}

std::unique_ptr<Links> PDFDoc::getLinks(int page)
{
    Page *p = getPage(page);
    if (!p) {
        return std::make_unique<Links>(nullptr);
    }
    return p->getLinks();
}

void PDFDoc::processLinks(OutputDev *out, int page)
{
    if (getPage(page)) {
        getPage(page)->processLinks(out);
    }
}

Linearization *PDFDoc::getLinearization()
{
    if (!linearization) {
        linearization = new Linearization(str.get());
        linearizationState = 0;
    }
    return linearization;
}

bool PDFDoc::checkLinearization()
{
    if (linearization == nullptr) {
        return false;
    }
    if (linearizationState == 1) {
        return true;
    }
    if (linearizationState == 2) {
        return false;
    }
    if (!hints) {
        hints = new Hints(str.get(), linearization, getXRef(), secHdlr);
    }
    if (!hints->isOk()) {
        linearizationState = 2;
        return false;
    }
    for (int page = 1; page <= linearization->getNumPages(); page++) {
        Ref pageRef;

        pageRef.num = hints->getPageObjectNum(page);
        if (!pageRef.num) {
            linearizationState = 2;
            return false;
        }

        // check for bogus ref - this can happen in corrupted PDF files
        if (pageRef.num < 0 || pageRef.num >= xref->getNumObjects()) {
            linearizationState = 2;
            return false;
        }

        pageRef.gen = xref->getEntry(pageRef.num)->gen;
        Object obj = xref->fetch(pageRef);
        if (!obj.isDict("Page")) {
            linearizationState = 2;
            return false;
        }
    }
    linearizationState = 1;
    return true;
}

bool PDFDoc::isLinearized(bool tryingToReconstruct)
{
    if ((str->getLength()) && (getLinearization()->getLength() == str->getLength())) {
        return true;
    }
    if (tryingToReconstruct) {
        return getLinearization()->getLength() > 0;
    }
    return false;
}

void PDFDoc::setDocInfoStringEntry(const char *key, std::unique_ptr<GooString> value)
{
    bool removeEntry = !value || value->empty() || (value->toStr() == unicodeByteOrderMark);