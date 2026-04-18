// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 19/24



    XRef *xref = doc->getXRef();

    quadding = VariableTextQuadding::leftJustified;
    defaultResources = nullptr;

    Object *acroForm = doc->getCatalog()->getAcroForm();

    needAppearances = acroForm->dictLookup("NeedAppearances").getBoolWithDefaultValue(false);

    obj1 = acroForm->dictLookup("DA");
    if (obj1.isString()) {
        defaultAppearance = obj1.takeString();
    }

    obj1 = acroForm->dictLookup("Q");
    if (obj1.isInt()) {
        const auto aux = static_cast<VariableTextQuadding>(obj1.getInt());
        if (aux == VariableTextQuadding::leftJustified || aux == VariableTextQuadding::centered || aux == VariableTextQuadding::rightJustified) {
            quadding = static_cast<VariableTextQuadding>(aux);
        }
    }

    resDict = acroForm->dictLookup("DR");
    if (resDict.isDict()) {
        // At a minimum, this dictionary shall contain a Font entry
        obj1 = resDict.dictLookup("Font");
        if (obj1.isDict()) {
            defaultResources = new GfxResources(xref, resDict.getDict(), nullptr);
        }
    }
    if (!defaultResources) {
        resDict.setToNull();
    }

    obj1 = acroForm->dictLookup("Fields");
    if (obj1.isArray()) {
        Array *array = obj1.getArray();
        RefRecursionChecker alreadyReadRefs;
        for (int i = 0; i < array->getLength(); i++) {
            Object obj2 = array->get(i);
            const Object &oref = array->getNF(i);
            if (!oref.isRef()) {
                error(errSyntaxWarning, -1, "Direct object in rootFields");
                continue;
            }

            if (!obj2.isDict()) {
                error(errSyntaxWarning, -1, "Reference in Fields array to an invalid or non existent object");
                continue;
            }

            if (!alreadyReadRefs.insert(oref.getRef())) {
                continue;
            }

            std::set<int> usedParents;
            rootFields.push_back(createFieldFromDict(std::move(obj2), doc, oref.getRef(), nullptr, &usedParents));
        }
    } else {
        error(errSyntaxError, -1, "Can't get Fields array");
    }

    obj1 = acroForm->dictLookup("CO");
    if (obj1.isArray()) {
        Array *array = obj1.getArray();
        calculateOrder.reserve(array->getLength());
        for (int i = 0; i < array->getLength(); i++) {
            const Object &oref = array->getNF(i);
            if (!oref.isRef()) {
                error(errSyntaxWarning, -1, "Direct object in CO");
                continue;
            }
            calculateOrder.push_back(oref.getRef());
        }
    }

    //   for (int i = 0; i < numFields; i++)
    //     rootFields[i]->printTree();
}

Form::~Form()
{
    delete defaultResources;
}

// Look up an inheritable field dictionary entry.
static Object fieldLookup(Dict *field, const char *key, RefRecursionChecker *usedParents)
{
    Dict *dict = field;
    Object obj = dict->lookup(key);
    if (!obj.isNull()) {
        return obj;
    }
    const Object &parent = dict->lookupNF("Parent");
    if (parent.isRef()) {
        const Ref ref = parent.getRef();
        if (usedParents->insert(ref)) {
            Object obj2 = parent.fetch(dict->getXRef());
            if (obj2.isDict()) {
                return fieldLookup(obj2.getDict(), key, usedParents);
            }
        }
    } else if (parent.isDict()) {
        return fieldLookup(parent.getDict(), key, usedParents);
    }
    return Object::null();
}

Object Form::fieldLookup(Dict *field, const char *key)
{
    RefRecursionChecker usedParents;
    return ::fieldLookup(field, key, &usedParents);
}

std::unique_ptr<FormField> Form::createFieldFromDict(Object &&obj, PDFDoc *docA, const Ref aref, FormField *parent, std::set<int> *usedParents)
{
    const Object obj2 = Form::fieldLookup(obj.getDict(), "FT");
    if (obj2.isName("Btn")) {
        return std::make_unique<FormFieldButton>(docA, std::move(obj), aref, parent, usedParents);
    }
    if (obj2.isName("Tx")) {
        return std::make_unique<FormFieldText>(docA, std::move(obj), aref, parent, usedParents);
    }
    if (obj2.isName("Ch")) {
        return std::make_unique<FormFieldChoice>(docA, std::move(obj), aref, parent, usedParents);
    }
    if (obj2.isName("Sig")) {
        return std::make_unique<FormFieldSignature>(docA, std::move(obj), aref, parent, usedParents);
    }
    // we don't have an FT entry => non-terminal field
    return std::make_unique<FormField>(docA, std::move(obj), aref, parent, usedParents);

    return {};
}

static const std::string kOurDictFontNamePrefix = "popplerfont";

std::string Form::findFontInDefaultResources(const std::string &fontFamily, const std::string &fontStyle) const
{
    if (!resDict.isDict()) {
        return {};
    }

    const std::string fontFamilyAndStyle = fontStyle.empty() ? fontFamily : fontFamily + " " + fontStyle;

    Object fontDictObj = resDict.dictLookup("Font");
    assert(fontDictObj.isDict());