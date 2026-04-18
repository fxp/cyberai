// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 8/24



    // childs
    Object obj1 = dict->lookup("Kids");
    if (obj1.isArray()) {
        // Load children
        for (int i = 0; i < obj1.arrayGetLength(); i++) {
            Ref childRef;
            Object childObj = obj1.getArray()->get(i, &childRef);
            if (childRef == Ref::INVALID()) {
                error(errSyntaxError, -1, "Invalid form field renference");
                continue;
            }
            if (!childObj.isDict()) {
                error(errSyntaxError, -1, "Form field child is not a dictionary");
                continue;
            }

            if (!usedParents->contains(childRef.num)) {
                // Field child: it could be a form field or a widget or composed dict
                const Object &objParent = childObj.dictLookupNF("Parent");
                Object obj3 = childObj.dictLookup("Parent");
                const bool hasParent = objParent.isRef() || obj3.isDict();
                const bool isWidget = childObj.dictLookup("Subtype").isName("Widget");
                // Handle hypothetical case of root Field with no children and no "T" key (as "T" is optional for PDF < 2.0)
                const bool isField = childObj.dictLookup("T").isString() || (!hasParent && childObj.dictLookup("FT").isName());
                const bool isComposedWidget = isWidget && isField;
                const bool isPureWidget = isWidget && !isComposedWidget;
                if (hasParent && !isPureWidget) {
                    // Child is a form field or composed dict
                    // We create the field, if it's composed
                    // it will create the widget as a child
                    std::set<int> usedParentsAux = *usedParents;
                    usedParentsAux.insert(childRef.num);

                    if (terminal) {
                        error(errSyntaxWarning, -1, "Field can't have both Widget AND Field as kids");
                        continue;
                    }

                    children.push_back(Form::createFieldFromDict(std::move(childObj), doc, childRef, this, &usedParentsAux));
                } else if (isPureWidget) {
                    if (hasParent) {
                        // PDF 1.7 wrt 1.4 introduced "Parent" for Widget Annotations, this is
                        // such a case so let's validate the Parent just for warning purposes
                        const bool parentOK = (objParent.isRef() && objParent.getRef() == ref) || (obj3.isDict() && &obj3 == &obj);
                        if (!parentOK) {
                            error(errSyntaxWarning, -1, "Widget Annotation's Parent entry not pointing to its parent FormField");
                        }
                    }
                    // Child is a widget annotation
                    if (!terminal && !children.empty()) {
                        error(errSyntaxWarning, -1, "Field can't have both Widget AND Field as kids");
                        continue;
                    }
                    _createWidget(&childObj, childRef);
                }
            }
        }
    } else {
        // No children, if it's a composed dict, create the child widget
        obj1 = dict->lookup("Subtype");
        if (obj1.isName("Widget")) {
            _createWidget(&obj, ref);
        }
    }

    // flags
    obj1 = Form::fieldLookup(dict, "Ff");
    if (obj1.isInt()) {
        int flags = obj1.getInt();
        if (flags & 0x1) { // 1 -> ReadOnly
            readOnly = true;
        }
        if (flags & 0x2) { // 2 -> Required
            // TODO
        }
        if (flags & 0x4) { // 3 -> NoExport
            noExport = true;
        }
    }

    // Variable Text
    obj1 = Form::fieldLookup(dict, "DA");
    if (obj1.isString()) {
        defaultAppearance = obj1.takeString();
    }

    obj1 = Form::fieldLookup(dict, "Q");
    if (obj1.isInt()) {
        const auto aux = static_cast<VariableTextQuadding>(obj1.getInt());
        hasQuadding = aux == VariableTextQuadding::leftJustified || aux == VariableTextQuadding::centered || aux == VariableTextQuadding::rightJustified;
        if (likely(hasQuadding)) {
            quadding = static_cast<VariableTextQuadding>(aux);
        }
    }

    obj1 = dict->lookup("T");
    if (obj1.isString()) {
        partialName = obj1.takeString();
    }

    obj1 = dict->lookup("TU");
    if (obj1.isString()) {
        alternateUiName = obj1.takeString();
    }

    obj1 = dict->lookup("TM");
    if (obj1.isString()) {
        mappingName = obj1.takeString();
    }
    widgets.shrink_to_fit();
    children.shrink_to_fit();
}

void FormField::setDefaultAppearance(const std::string &appearance)
{
    defaultAppearance = std::make_unique<GooString>(appearance);
}

void FormField::setPartialName(const GooString &name)
{
    partialName = name.copy();

    obj.getDict()->set("T", Object(name.copy()));
    xref->setModifiedObject(&obj, ref);
}

FormField::~FormField() = default;