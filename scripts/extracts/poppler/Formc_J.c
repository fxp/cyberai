// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 10/24



    if (partialName) {
        if (unicode_encoded) {
            if (hasUnicodeByteOrderMark(partialName->toStr())) {
                fullyQualifiedName->append(partialName->c_str() + 2, partialName->size() - 2); // Remove the unicode BOM
            } else {
                std::string tmp_str = pdfDocEncodingToUTF16(partialName->toStr());
                fullyQualifiedName->append(tmp_str.c_str() + 2, tmp_str.size() - 2); // Remove the unicode BOM
            }
        } else {
            if (hasUnicodeByteOrderMark(partialName->toStr())) {
                unicode_encoded = true;
                fullyQualifiedName = convertToUtf16(fullyQualifiedName.get());
                fullyQualifiedName->append(partialName->c_str() + 2, partialName->size() - 2); // Remove the unicode BOM
            } else {
                fullyQualifiedName->append(partialName->toStr());
            }
        }
    } else {
        int len = fullyQualifiedName->size();
        // Remove the last period
        if (unicode_encoded) {
            if (len > 1) {
                fullyQualifiedName->erase(len - 2, 2);
            }
        } else {
            if (len > 0) {
                fullyQualifiedName->erase(len - 1, 1);
            }
        }
    }

    if (unicode_encoded) {
        prependUnicodeByteOrderMark(fullyQualifiedName->toNonConstStr());
    }

    return fullyQualifiedName.get();
}

void FormField::updateChildrenAppearance()
{
    // Recursively update each child's appearance
    if (terminal) {
        for (auto &widget : widgets) {
            widget->updateWidgetAppearance();
        }
    } else {
        for (auto &child : children) {
            child->updateChildrenAppearance();
        }
    }
}

void FormField::setReadOnly(bool value)
{
    if (value == readOnly) {
        return;
    }

    readOnly = value;

    Dict *dict = obj.getDict();

    const Object obj1 = Form::fieldLookup(dict, "Ff");
    int flags = 0;
    if (obj1.isInt()) {
        flags = obj1.getInt();
    }
    if (value) {
        flags |= 1;
    } else {
        flags &= ~1;
    }

    dict->set("Ff", Object(flags));
    xref->setModifiedObject(&obj, ref);
    updateChildrenAppearance();
}

void FormField::reset(const std::vector<std::string> &excludedFields)
{
    resetChildren(excludedFields);
}

void FormField::resetChildren(const std::vector<std::string> &excludedFields)
{
    if (!terminal) {
        for (auto &child : children) {
            child->reset(excludedFields);
        }
    }
}

bool FormField::isAmongExcludedFields(const std::vector<std::string> &excludedFields)
{
    Ref fieldRef;

    for (const std::string &field : excludedFields) {
        if (field.ends_with(" R")) {
            if (sscanf(field.c_str(), "%d %d R", &fieldRef.num, &fieldRef.gen) == 2 && fieldRef == getRef()) {
                return true;
            }
        } else {
            if (field == getFullyQualifiedName()->toStr()) {
                return true;
            }
        }
    }

    return false;
}

FormField *FormField::findFieldByRef(Ref aref)
{
    if (terminal) {
        if (this->getRef() == aref) {
            return this;
        }
    } else {
        for (auto &child : children) {
            FormField *result = child->findFieldByRef(aref);
            if (result) {
                return result;
            }
        }
    }
    return nullptr;
}

FormField *FormField::findFieldByFullyQualifiedName(const std::string &name)
{
    if (terminal) {
        if (getFullyQualifiedName()->compare(name) == 0) {
            return this;
        }
    } else {
        for (auto &child : children) {
            FormField *result = child->findFieldByFullyQualifiedName(name);
            if (result) {
                return result;
            }
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------
// FormFieldButton
//------------------------------------------------------------------------
FormFieldButton::FormFieldButton(PDFDoc *docA, Object &&dictObj, const Ref refA, FormField *parentA, std::set<int> *usedParents) : FormField(docA, std::move(dictObj), refA, parentA, usedParents, formButton)
{
    Dict *dict = obj.getDict();
    active_child = -1;
    noAllOff = false;
    appearanceState.setToNull();
    defaultAppearanceState.setToNull();

    btype = formButtonCheck;
    Object obj1 = Form::fieldLookup(dict, "Ff");
    if (obj1.isInt()) {
        int flags = obj1.getInt();

        if (flags & 0x10000) { // 17 -> push button
            btype = formButtonPush;
        } else if (flags & 0x8000) { // 16 -> radio button
            btype = formButtonRadio;
            if (flags & 0x4000) { // 15 -> noToggleToOff
                noAllOff = true;
            }
        }
        if (flags & 0x1000000) { // 26 -> radiosInUnison
            error(errUnimplemented, -1, "FormFieldButton:: radiosInUnison flag unimplemented, please report a bug with a testcase");
        }
    }