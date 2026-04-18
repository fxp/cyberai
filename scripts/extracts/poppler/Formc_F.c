// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 6/24



    double x1, y1, x2, y2;
    getRect(&x1, &y1, &x2, &y2);
    const PDFRectangle rect(x1, y1, x2, y2);
    std::unique_ptr<AnnotAppearanceCharacs> origAppearCharacs = getWidgetAnnotation()->getAppearCharacs() ? getWidgetAnnotation()->getAppearCharacs()->copy() : nullptr;
    const int rot = origAppearCharacs ? origAppearCharacs->getRotation() : 0;
    const auto dxdy = calculateDxDy(rot, &rect);
    const double dx = std::get<0>(dxdy);
    const double dy = std::get<1>(dxdy);
    const double wMax = dx - 2 * borderWidth - 4;
    const double hMax = dy - 2 * borderWidth;

    std::unique_ptr<AnnotBorder> origBorderCopy = getWidgetAnnotation()->getBorder() ? getWidgetAnnotation()->getBorder()->copy() : nullptr;
    std::unique_ptr<AnnotBorder> border(new AnnotBorderArray());
    border->setWidth(borderWidth);
    getWidgetAnnotation()->setBorder(std::move(border));

    if (!signatureText.empty() || !signatureTextLeft.empty()) {
        const std::string pdfFontName = form->findPdfFontNameToUseForSigning();
        if (pdfFontName.empty()) {
            return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::InternalError, .message = ERROR_IN_CODE_LOCATION };
        }
        std::shared_ptr<GfxFont> font = form->getDefaultResources()->lookupFont(pdfFontName.c_str());

        if (fontSize == 0) {
            fontSize = Annot::calculateFontSize(form, font.get(), &signatureText, wMax / 2.0, hMax);
        }
        if (leftFontSize == 0) {
            leftFontSize = Annot::calculateFontSize(form, font.get(), &signatureTextLeft, wMax / 2.0, hMax);
        }
        const DefaultAppearance da { pdfFontName, fontSize, std::move(fontColor) };
        getField()->setDefaultAppearance(da.toAppearanceString());
        form->ensureFontsForAllCharacters(&signatureText, pdfFontName);
        form->ensureFontsForAllCharacters(&signatureTextLeft, pdfFontName);
    }
    auto appearCharacs = std::make_unique<AnnotAppearanceCharacs>(nullptr);
    appearCharacs->setBorderColor(std::move(borderColor));
    appearCharacs->setBackColor(std::move(backgroundColor));
    getWidgetAnnotation()->setAppearCharacs(std::move(appearCharacs));

    getWidgetAnnotation()->generateFieldAppearance();
    getWidgetAnnotation()->updateAppearanceStream();

    auto *ffs = static_cast<::FormFieldSignature *>(getField());
    ffs->setCustomAppearanceContent(signatureText);
    ffs->setCustomAppearanceLeftContent(signatureTextLeft);
    ffs->setCustomAppearanceLeftFontSize(leftFontSize);

    // say that there a now signatures and that we should append only
    doc->getCatalog()->getAcroForm()->dictSet("SigFlags", Object(3));

    auto signingResult = signDocument(saveFilename, certNickname, password, reason, location, ownerPassword, userPassword);

    // Now bring back the annotation appearance back to what it was
    ffs->setDefaultAppearance(originalDefaultAppearance);
    ffs->setCustomAppearanceContent({});
    ffs->setCustomAppearanceLeftContent({});
    getWidgetAnnotation()->setAppearCharacs(std::move(origAppearCharacs));
    getWidgetAnnotation()->setBorder(std::move(origBorderCopy));
    getWidgetAnnotation()->updateAppearanceStream();

    return signingResult;
}

// Get start and end file position of objNum in the PDF named filename.
bool FormWidgetSignature::getObjectStartEnd(const GooString &filename, int objNum, Goffset *objStart, Goffset *objEnd, const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword)
{
    PDFDoc newDoc(filename.copy(), ownerPassword, userPassword);
    if (!newDoc.isOk()) {
        return false;
    }

    XRef *newXref = newDoc.getXRef();
    XRefEntry *entry = newXref->getEntry(objNum);
    if (entry->type != xrefEntryUncompressed) {
        return false;
    }

    *objStart = entry->offset;
    newXref->fetch(objNum, entry->gen, 0, objEnd);
    return true;
}

// find next offset containing the dummy offset '9999999999' and overwrite with offset
static char *setNextOffset(char *start, Goffset offset)
{
    char buf[50];
    sprintf(buf, "%lld", offset);
    strcat(buf, "                  "); // add some padding

    char *p = strstr(start, "9999999999");
    if (p) {
        memcpy(p, buf, 10); // overwrite exact size.
        p += 10;
    } else {
        return nullptr;
    }
    return p;
}

// Updates the ByteRange array of the signature object in the file.
// Returns start/end of signature string and file size.
bool FormWidgetSignature::updateOffsets(FILE *f, Goffset objStart, Goffset objEnd, Goffset *sigStart, Goffset *sigEnd, Goffset *fileSize)
{
    if (Gfseek(f, 0, SEEK_END) != 0) {
        return false;
    }
    *fileSize = Gftell(f);

    if (objEnd > *fileSize) {
        objEnd = *fileSize;
    }

    // sanity check object offsets
    if (objEnd <= objStart || (objEnd - objStart >= INT_MAX)) {
        return false;
    }