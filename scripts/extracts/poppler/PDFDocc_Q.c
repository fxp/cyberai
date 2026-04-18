// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 17/19



            // read last xrefSearchSize bytes
            int segnum = 0;
            int maxXRefSearch = 24576;
            if (str->getLength() < maxXRefSearch) {
                maxXRefSearch = static_cast<int>(str->getLength());
            }
            for (; (xrefSearchSize - 16) * segnum < maxXRefSearch; segnum++) {
                str->setPos((xrefSearchSize - 16) * segnum + xrefSearchSize, -1);
                for (n = 0; n < xrefSearchSize; ++n) {
                    if ((c = str->getChar()) == EOF) {
                        break;
                    }
                    buf[n] = c;
                }
                buf[n] = '\0';

                // find startxref
                for (i = n - 9; i >= 0; --i) {
                    if (!strncmp(&buf[i], "startxref", 9)) {
                        break;
                    }
                }
                if (i < 0) {
                    startXRefPos = 0;
                } else {
                    for (p = &buf[i + 9]; isspace(*p); ++p) {
                        ;
                    }
                    startXRefPos = strToLongLong(p);
                    break;
                }
            }
        }
    }

    return startXRefPos;
}

Goffset PDFDoc::getMainXRefEntriesOffset(bool tryingToReconstruct)
{
    unsigned int mainXRefEntriesOffset = 0;

    if (isLinearized(tryingToReconstruct)) {
        mainXRefEntriesOffset = getLinearization()->getMainXRefEntriesOffset();
    }

    return mainXRefEntriesOffset;
}

int PDFDoc::getNumPages()
{
    if (isLinearized()) {
        int n;
        if ((n = getLinearization()->getNumPages())) {
            return n;
        }
    }

    return catalog->getNumPages();
}

std::unique_ptr<Page> PDFDoc::parsePage(int page)
{
    Ref pageRef;

    pageRef.num = getHints()->getPageObjectNum(page);
    if (!pageRef.num) {
        error(errSyntaxWarning, -1, "Failed to get object num from hint tables for page {0:d}", page);
        return nullptr;
    }

    // check for bogus ref - this can happen in corrupted PDF files
    if (pageRef.num < 0 || pageRef.num >= xref->getNumObjects()) {
        error(errSyntaxWarning, -1, "Invalid object num ({0:d}) for page {1:d}", pageRef.num, page);
        return nullptr;
    }

    pageRef.gen = xref->getEntry(pageRef.num)->gen;
    Object obj = xref->fetch(pageRef);
    if (!obj.isDict("Page")) {
        error(errSyntaxWarning, -1, "Object ({0:d} {1:d}) is not a pageDict", pageRef.num, pageRef.gen);
        return nullptr;
    }
    Dict *pageDict = obj.getDict();

    return std::make_unique<Page>(this, page, std::move(obj), pageRef, std::make_unique<PageAttrs>(nullptr, pageDict));
}

Page *PDFDoc::getPage(int page)
{
    if ((page < 1) || page > getNumPages()) {
        return nullptr;
    }

    if (isLinearized() && checkLinearization()) {
        pdfdocLocker();
        if (pageCache.empty()) {
            pageCache.resize(getNumPages());
        }
        if (!pageCache[page - 1]) {
            pageCache[page - 1] = parsePage(page);
        }
        if (pageCache[page - 1]) {
            return pageCache[page - 1].get();
        }
        error(errSyntaxWarning, -1, "Failed parsing page {0:d} using hint tables", page);
    }

    return catalog->getPage(page);
}

bool PDFDoc::hasJavascript()
{
    JSInfo jsInfo(this);
    jsInfo.scanJS(getNumPages(), true);
    return jsInfo.containsJS();
}

std::variant<PDFDoc::SignatureData, CryptoSign::SigningErrorMessage> PDFDoc::createSignature(::Page *destPage, std::unique_ptr<GooString> &&partialFieldName, const PDFRectangle &rect, const GooString &signatureText,
                                                                                             const GooString &signatureTextLeft, double fontSize, double leftFontSize, std::unique_ptr<AnnotColor> &&fontColor, double borderWidth,
                                                                                             std::unique_ptr<AnnotColor> &&borderColor, std::unique_ptr<AnnotColor> &&backgroundColor, const std::string &imagePath)
{
    if (destPage == nullptr) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::InternalError, .message = ERROR_IN_CODE_LOCATION };
    }

    Ref imageResourceRef = Ref::INVALID();
    if (!imagePath.empty()) {
        imageResourceRef = ImageEmbeddingUtils::embed(xref, imagePath);
        if (imageResourceRef == Ref::INVALID()) {
            return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
        }
    }

    Form *form = catalog->getCreateForm();