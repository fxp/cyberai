// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 12/24



            // Clear check button if it doesn't have default value.
            // This behaviour is what Adobe Reader does, it is not written in specification.
            if (btype == formButtonCheck) {
                setState("Off");
            }
        }
    }

    resetChildren(excludedFields);
}

//------------------------------------------------------------------------
// FormFieldText
//------------------------------------------------------------------------
FormFieldText::FormFieldText(PDFDoc *docA, Object &&dictObj, const Ref refA, FormField *parentA, std::set<int> *usedParents) : FormField(docA, std::move(dictObj), refA, parentA, usedParents, formText)
{
    Dict *dict = obj.getDict();
    Object obj1;
    multiline = password = fileSelect = doNotSpellCheck = doNotScroll = comb = richText = false;
    maxLen = 0;

    obj1 = Form::fieldLookup(dict, "Ff");
    if (obj1.isInt()) {
        int flags = obj1.getInt();
        if (flags & 0x1000) { // 13 -> Multiline
            multiline = true;
        }
        if (flags & 0x2000) { // 14 -> Password
            password = true;
        }
        if (flags & 0x100000) { // 21 -> FileSelect
            fileSelect = true;
        }
        if (flags & 0x400000) { // 23 -> DoNotSpellCheck
            doNotSpellCheck = true;
        }
        if (flags & 0x800000) { // 24 -> DoNotScroll
            doNotScroll = true;
        }
        if (flags & 0x1000000) { // 25 -> Comb
            comb = true;
        }
        if (flags & 0x2000000) { // 26 -> RichText
            richText = true;
        }
    }

    obj1 = Form::fieldLookup(dict, "MaxLen");
    if (obj1.isInt()) {
        maxLen = obj1.getInt();
    }

    fillContent(fillDefaultValue);
    fillContent(fillValue);
}

void FormFieldText::fillContent(FillValueType fillType)
{
    Dict *dict = obj.getDict();
    Object obj1;

    obj1 = Form::fieldLookup(dict, fillType == fillDefaultValue ? "DV" : "V");
    if (obj1.isString()) {
        if (hasUnicodeByteOrderMark(obj1.getString())) {
            if (obj1.getString().size() > 2) {
                if (fillType == fillDefaultValue) {
                    defaultContent = obj1.takeString();
                } else {
                    content = obj1.takeString();
                }
            }
        } else if (!obj1.getString().empty()) {
            // non-unicode string -- assume pdfDocEncoding and try to convert to UTF16BE
            std::string tmp_str = pdfDocEncodingToUTF16(obj1.getString());

            if (fillType == fillDefaultValue) {
                defaultContent = std::make_unique<GooString>(std::move(tmp_str));
            } else {
                content = std::make_unique<GooString>(std::move(tmp_str));
            }
        }
    }
}

void FormFieldText::print(int indent)
{
    printf("%*s- (%d %d): [text] terminal: %s children: %zu\n", indent, "", ref.num, ref.gen, terminal ? "Yes" : "No", terminal ? widgets.size() : children.size());
}

void FormFieldText::setContent(std::unique_ptr<GooString> new_content)
{
    content.reset();

    if (new_content) {
        content = std::move(new_content);