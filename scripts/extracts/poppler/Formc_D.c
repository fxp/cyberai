// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 4/24



void FormWidgetChoice::setAppearanceChoiceContent(std::unique_ptr<GooString> new_content)
{
    parent()->setAppearanceChoiceContent(std::move(new_content));
}

int FormWidgetChoice::getNumChoices() const
{
    return parent()->getNumChoices();
}

const GooString *FormWidgetChoice::getChoice(int i) const
{
    return parent()->getChoice(i);
}

const GooString *FormWidgetChoice::getExportVal(int i) const
{
    return parent()->getExportVal(i);
}

bool FormWidgetChoice::isCombo() const
{
    return parent()->isCombo();
}

bool FormWidgetChoice::hasEdit() const
{
    return parent()->hasEdit();
}

bool FormWidgetChoice::isMultiSelect() const
{
    return parent()->isMultiSelect();
}

bool FormWidgetChoice::noSpellCheck() const
{
    return parent()->noSpellCheck();
}

bool FormWidgetChoice::commitOnSelChange() const
{
    return parent()->commitOnSelChange();
}

bool FormWidgetChoice::isListBox() const
{
    return parent()->isListBox();
}

FormFieldChoice *FormWidgetChoice::parent() const
{
    return static_cast<FormFieldChoice *>(field);
}

FormWidgetSignature::FormWidgetSignature(PDFDoc *docA, Object *dictObj, unsigned num, Ref refA, FormField *p) : FormWidget(docA, dictObj, num, refA, p)
{
    type = formSignature;
}

const std::vector<unsigned char> &FormWidgetSignature::getSignature() const
{
    return static_cast<FormFieldSignature *>(field)->getSignature();
}

SignatureInfo *FormWidgetSignature::validateSignatureAsync(bool doVerifyCert, bool forceRevalidation, time_t validationTime, bool ocspRevocationCheck, bool enableAIA, const std::function<void()> &doneCallback)
{
    return static_cast<FormFieldSignature *>(field)->validateSignatureAsync(doVerifyCert, forceRevalidation, validationTime, ocspRevocationCheck, enableAIA, doneCallback);
}

CertificateValidationStatus FormWidgetSignature::validateSignatureResult()
{
    return static_cast<FormFieldSignature *>(field)->validateSignatureResult();
}

// update hash with the specified range of data from the file
static bool hashFileRange(FILE *f, CryptoSign::SigningInterface *handler, Goffset start, Goffset end)
{
    if (!handler) {
        return false;
    }
    const int BUF_SIZE = 65536;

    auto *buf = new unsigned char[BUF_SIZE];

    while (start < end) {
        if (Gfseek(f, start, SEEK_SET) != 0) {
            delete[] buf;
            return false;
        }
        int len = BUF_SIZE;
        if (end - start < len) {
            len = static_cast<int>(end - start);
        }
        if (fread(buf, 1, len, f) != static_cast<size_t>(len)) {
            delete[] buf;
            return false;
        }
        handler->addData(buf, len);
        start += len;
    }
    delete[] buf;
    return true;
}

std::optional<CryptoSign::SigningErrorMessage> FormWidgetSignature::signDocument(const std::string &saveFilename, const std::string &certNickname, const std::string &password, const GooString *reason, const GooString *location,
                                                                                 const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword)
{
    auto backend = CryptoSign::Factory::createActive();
    if (!backend) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::InternalError, .message = ERROR_IN_CODE_LOCATION };
    }
    if (certNickname.empty()) {
        error(errInternal, -1, "signDocument: Empty nickname");
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::KeyMissing, .message = ERROR_IN_CODE_LOCATION };
    }

    auto sigHandler = backend->createSigningHandler(certNickname, HashAlgorithm::Sha256);

    const unsigned int maxExpectedSignatureSize = sigHandler->estimateSize();

    auto *signatureField = static_cast<FormFieldSignature *>(field);
    std::unique_ptr<X509CertificateInfo> certInfo = sigHandler->getCertificateInfo();
    if (!certInfo) {
        error(errInternal, -1, "signDocument: error getting signature info");
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::KeyMissing, .message = ERROR_IN_CODE_LOCATION };
    }
    const std::string signerName = certInfo->getSubjectInfo().commonName;
    signatureField->setCertificateInfo(certInfo);
    updateWidgetAppearance(); // add visible signing info to appearance

    Object vObj(std::make_unique<Dict>(xref));
    Ref vref = xref->addIndirectObject(vObj);
    if (!createSignature(vObj, vref, GooString(signerName), maxExpectedSignatureSize, reason, location, sigHandler->signatureType())) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::InternalError, .message = ERROR_IN_CODE_LOCATION };
    }