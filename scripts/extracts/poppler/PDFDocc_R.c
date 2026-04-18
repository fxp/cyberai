// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 18/19



    Object annotObj = Object(std::make_unique<Dict>(getXRef()));
    annotObj.dictSet("Type", Object(objName, "Annot"));
    annotObj.dictSet("Subtype", Object(objName, "Widget"));
    annotObj.dictSet("FT", Object(objName, "Sig"));
    annotObj.dictSet("T", Object(std::move(partialFieldName)));
    auto rectArray = std::make_unique<Array>(getXRef());
    rectArray->add(Object(rect.x1));
    rectArray->add(Object(rect.y1));
    rectArray->add(Object(rect.x2));
    rectArray->add(Object(rect.y2));
    annotObj.dictSet("Rect", Object(std::move(rectArray)));

    if (!signatureText.empty() || !signatureTextLeft.empty()) {
        const std::string pdfFontName = form->findPdfFontNameToUseForSigning();
        if (pdfFontName.empty()) {
            return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ERROR_IN_CODE_LOCATION };
        }

        const DefaultAppearance da { pdfFontName, fontSize, std::move(fontColor) };
        const std::string daStr = da.toAppearanceString();
        annotObj.dictSet("DA", Object(std::make_unique<GooString>(daStr)));

        form->ensureFontsForAllCharacters(&signatureText, pdfFontName);
        form->ensureFontsForAllCharacters(&signatureTextLeft, pdfFontName);
    }

    const Ref ref = getXRef()->addIndirectObject(annotObj);
    catalog->addFormToAcroForm(ref);
    catalog->setAcroFormModified();

    std::unique_ptr<::FormFieldSignature> field = std::make_unique<::FormFieldSignature>(this, std::move(annotObj), ref, nullptr, nullptr);
    field->setCustomAppearanceContent(signatureText);
    field->setCustomAppearanceLeftContent(signatureTextLeft);
    field->setCustomAppearanceLeftFontSize(leftFontSize);
    field->setImageResource(imageResourceRef);

    Object refObj(ref);
    auto signatureAnnot = std::make_shared<AnnotWidget>(this, field->getObj(), &refObj, field.get());
    signatureAnnot->setFlags(signatureAnnot->getFlags() | Annot::flagPrint | /*Annot::flagLocked | TODO */ Annot::flagNoRotate);
    Dict dummy(getXRef());
    auto appearCharacs = std::make_unique<AnnotAppearanceCharacs>(&dummy);
    appearCharacs->setBorderColor(std::move(borderColor));
    appearCharacs->setBackColor(std::move(backgroundColor));
    signatureAnnot->setAppearCharacs(std::move(appearCharacs));

    std::unique_ptr<AnnotBorder> border(new AnnotBorderArray());
    border->setWidth(borderWidth);
    signatureAnnot->setBorder(std::move(border));

    FormWidget *formWidget = field->getWidget(field->getNumWidgets() - 1);
    formWidget->setWidgetAnnotation(signatureAnnot);

    return SignatureData { .ref = { .num = ref.num, .gen = ref.gen }, .annotWidget = signatureAnnot, .formWidget = formWidget, .field = std::move(field) };
}

std::optional<CryptoSign::SigningErrorMessage> PDFDoc::sign(const std::string &saveFilename, const std::string &certNickname, const std::string &password, std::unique_ptr<GooString> &&partialFieldName, int page, const PDFRectangle &rect,
                                                            const GooString &signatureText, const GooString &signatureTextLeft, double fontSize, double leftFontSize, std::unique_ptr<AnnotColor> &&fontColor, double borderWidth,
                                                            std::unique_ptr<AnnotColor> &&borderColor, std::unique_ptr<AnnotColor> &&backgroundColor, const GooString *reason, const GooString *location, const std::string &imagePath,
                                                            const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword)
{
    ::Page *destPage = getPage(page);
    if (destPage == nullptr) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::InternalError, .message = ERROR_IN_CODE_LOCATION };
    }

    std::variant<SignatureData, CryptoSign::SigningErrorMessage> result =
            createSignature(destPage, std::move(partialFieldName), rect, signatureText, signatureTextLeft, fontSize, leftFontSize, std::move(fontColor), borderWidth, std::move(borderColor), std::move(backgroundColor), imagePath);

    if (std::holds_alternative<CryptoSign::SigningErrorMessage>(result)) {
        return std::get<CryptoSign::SigningErrorMessage>(result);
    }

    auto *sig = std::get_if<SignatureData>(&result);

    sig->annotWidget->setFlags(sig->annotWidget->getFlags() | Annot::flagLocked);

    // say that there a now signatures and that we should append only
    catalog->getAcroForm()->dictSet("SigFlags", Object(3));

    destPage->addAnnot(sig->annotWidget);

    auto *fws = dynamic_cast<FormWidgetSignature *>(sig->formWidget);
    if (fws) {
        const auto res = fws->signDocument(saveFilename, certNickname, password, reason, location, ownerPassword, userPassword);