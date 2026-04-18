// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 9/24



void FormField::print(int indent)
{
    printf("%*s- (%d %d): [container] terminal: %s children: %zu\n", indent, "", ref.num, ref.gen, terminal ? "Yes" : "No", terminal ? widgets.size() : children.size());
}

void FormField::printTree(int indent)
{
    print(indent);
    if (terminal) {
        for (const auto &widget : widgets) {
            widget->print(indent + 4);
        }
    } else {
        for (const auto &child : children) {
            child->printTree(indent + 4);
        }
    }
}

void FormField::fillChildrenSiblingsID()
{
    if (terminal) {
        return;
    }
    for (const auto &child : children) {
        child->fillChildrenSiblingsID();
    }
}

void FormField::createWidgetAnnotations()
{
    if (terminal) {
        for (auto &widget : widgets) {
            widget->createWidgetAnnotation();
        }
    } else {
        for (auto &child : children) {
            child->createWidgetAnnotations();
        }
    }
}

void FormField::_createWidget(Object *objA, Ref aref)
{
    terminal = true;
    // ID = index in "widgets" table
    switch (type) {
    case formButton:
        widgets.push_back(std::make_unique<FormWidgetButton>(doc, objA, widgets.size(), aref, this));
        break;
    case formText:
        widgets.push_back(std::make_unique<FormWidgetText>(doc, objA, widgets.size(), aref, this));
        break;
    case formChoice:
        widgets.push_back(std::make_unique<FormWidgetChoice>(doc, objA, widgets.size(), aref, this));
        break;
    case formSignature:
        widgets.push_back(std::make_unique<FormWidgetSignature>(doc, objA, widgets.size(), aref, this));
        break;
    default:
        error(errSyntaxWarning, -1, "SubType on non-terminal field, invalid document?");
    }
}

FormWidget *FormField::findWidgetByRef(Ref aref)
{
    if (terminal) {
        for (auto &widget : widgets) {
            if (widget->getRef() == aref) {
                return widget.get();
            }
        }
    } else {
        for (auto &child : children) {
            FormWidget *result = child->findWidgetByRef(aref);
            if (result) {
                return result;
            }
        }
    }
    return nullptr;
}

const GooString *FormField::getFullyQualifiedName() const
{
    Object parentObj;
    bool unicode_encoded = false;

    if (fullyQualifiedName) {
        return fullyQualifiedName.get();
    }

    fullyQualifiedName = std::make_unique<GooString>();

    std::set<int> parsedRefs;
    Ref parentRef;
    parentObj = obj.getDict()->lookup("Parent", &parentRef);
    if (parentRef != Ref::INVALID()) {
        parsedRefs.insert(parentRef.num);
    }
    while (parentObj.isDict()) {
        Object obj2 = parentObj.dictLookup("T");
        if (obj2.isString()) {
            const std::string &parent_name = obj2.getString();

            if (unicode_encoded) {
                fullyQualifiedName->insert(0, "\0.", 2); // 2-byte unicode period
                if (hasUnicodeByteOrderMark(parent_name)) {
                    fullyQualifiedName->insert(0, parent_name.c_str() + 2, parent_name.size() - 2); // Remove the unicode BOM
                } else {
                    std::string tmp_str = pdfDocEncodingToUTF16(parent_name);
                    fullyQualifiedName->insert(0, tmp_str.c_str() + 2, tmp_str.size() - 2); // Remove the unicode BOM
                }
            } else {
                fullyQualifiedName->insert(0, "."); // 1-byte ascii period
                if (hasUnicodeByteOrderMark(parent_name)) {
                    unicode_encoded = true;
                    fullyQualifiedName = convertToUtf16(fullyQualifiedName.get());
                    fullyQualifiedName->insert(0, parent_name.c_str() + 2, parent_name.size() - 2); // Remove the unicode BOM
                } else {
                    fullyQualifiedName->insert(0, parent_name);
                }
            }
        }
        parentObj = parentObj.getDict()->lookup("Parent", &parentRef);
        if (parentRef != Ref::INVALID() && !parsedRefs.insert(parentRef.num).second) {
            error(errSyntaxError, -1, "FormField: Loop while trying to look for Parents");
            return fullyQualifiedName.get();
        }
    }