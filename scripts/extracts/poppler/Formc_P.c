// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 16/24



void FormFieldChoice::deselectAll()
{
    editedChoice.reset();

    unselectAll();
    updateSelection();
}

void FormFieldChoice::toggle(int i)
{
    editedChoice.reset();

    choices[i].selected = !choices[i].selected;
    updateSelection();
}

void FormFieldChoice::select(int i)
{
    editedChoice.reset();

    if (!multiselect) {
        unselectAll();
    }

    choices[i].selected = true;
    updateSelection();
}

void FormFieldChoice::setEditChoice(std::unique_ptr<GooString> new_content)
{
    editedChoice.reset();

    unselectAll();

    if (new_content) {
        editedChoice = std::move(new_content);

        // append the unicode marker <FE FF> if needed
        if (!hasUnicodeByteOrderMark(editedChoice->toStr())) {
            prependUnicodeByteOrderMark(editedChoice->toNonConstStr());
        }
    }
    updateSelection();
}

void FormFieldChoice::setAppearanceChoiceContent(std::unique_ptr<GooString> new_content)
{
    appearanceSelectedChoice.reset();

    if (new_content) {
        appearanceSelectedChoice = std::move(new_content);

        // append the unicode marker <FE FF> if needed
        if (!hasUnicodeByteOrderMark(appearanceSelectedChoice->toStr())) {
            prependUnicodeByteOrderMark(appearanceSelectedChoice->toNonConstStr());
        }
    }
    updateChildrenAppearance();
}

const GooString *FormFieldChoice::getEditChoice() const
{
    return editedChoice.get();
}

int FormFieldChoice::getNumSelected()
{
    int cnt = 0;
    for (int i = 0; i < numChoices; i++) {
        if (choices[i].selected) {
            cnt++;
        }
    }
    return cnt;
}

const GooString *FormFieldChoice::getSelectedChoice() const
{
    if (edit && editedChoice) {
        return editedChoice.get();
    }

    for (int i = 0; i < numChoices; i++) {
        if (choices[i].optionName && choices[i].selected) {
            return choices[i].optionName.get();
        }
    }

    return nullptr;
}

void FormFieldChoice::reset(const std::vector<std::string> &excludedFields)
{
    if (!isAmongExcludedFields(excludedFields)) {
        editedChoice.reset();

        if (defaultChoices) {
            for (int i = 0; i < numChoices; i++) {
                choices[i].selected = defaultChoices[i];
            }
        } else {
            unselectAll();
        }
    }

    resetChildren(excludedFields);

    updateSelection();
}

//------------------------------------------------------------------------
// FormFieldSignature
//------------------------------------------------------------------------
FormFieldSignature::FormFieldSignature(PDFDoc *docA, Object &&dict, const Ref refA, FormField *parentA, std::set<int> *usedParents) : FormField(docA, std::move(dict), refA, parentA, usedParents, formSignature)
{
    signature_info = new SignatureInfo();
    parseInfo();
}

FormFieldSignature::~FormFieldSignature()
{
    delete signature_info;
}

void FormFieldSignature::setSignature(std::vector<unsigned char> &&sig)
{
    signature = std::move(sig);
}

const GooString &FormFieldSignature::getCustomAppearanceContent() const
{
    return customAppearanceContent;
}

void FormFieldSignature::setCustomAppearanceContent(const GooString &s)
{
    customAppearanceContent = GooString(s.toStr());
}

const GooString &FormFieldSignature::getCustomAppearanceLeftContent() const
{
    return customAppearanceLeftContent;
}

void FormFieldSignature::setCustomAppearanceLeftContent(const GooString &s)
{
    customAppearanceLeftContent = GooString(s.toStr());
}

double FormFieldSignature::getCustomAppearanceLeftFontSize() const
{
    return customAppearanceLeftFontSize;
}

void FormFieldSignature::setCustomAppearanceLeftFontSize(double size)
{
    customAppearanceLeftFontSize = size;
}

Ref FormFieldSignature::getImageResource() const
{
    return imageResource;
}

void FormFieldSignature::setImageResource(const Ref imageResourceA)
{
    imageResource = imageResourceA;
}

void FormFieldSignature::setCertificateInfo(std::unique_ptr<X509CertificateInfo> &certInfo)
{
    certificate_info.swap(certInfo);
}

FormWidget *FormFieldSignature::getCreateWidget()
{
    ::FormWidget *fw = getWidget(0);
    if (!fw) {
        error(errSyntaxError, 0, "FormFieldSignature: was asked for widget and didn't had one, creating it");
        _createWidget(&obj, ref);
        fw = getWidget(0);
        fw->createWidgetAnnotation();
    }
    return fw;
}

void FormFieldSignature::parseInfo()
{
    if (!obj.isDict()) {
        return;
    }

    // retrieve PKCS#7
    Object sig_dict = obj.dictLookup("V");
    if (!sig_dict.isDict()) {
        return;
    }

    byte_range = sig_dict.dictLookup("ByteRange");

    Object contents_obj = sig_dict.dictLookup("Contents");
    if (contents_obj.isString()) {
        auto signatureString = contents_obj.takeString();
        signature = std::vector<unsigned char>(signatureString->c_str(), signatureString->c_str() + signatureString->size());
    }