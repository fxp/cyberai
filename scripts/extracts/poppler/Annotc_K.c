// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 43/79



//------------------------------------------------------------------------
// AnnotWidget
//------------------------------------------------------------------------

AnnotWidget::AnnotWidget(PDFDoc *docA, Object &&dictObject, const Object *obj) : Annot(docA, std::move(dictObject), obj)
{
    type = typeWidget;
    field = nullptr;
    initialize(annotObj.getDict());
}

AnnotWidget::AnnotWidget(PDFDoc *docA, Object *dictObject, Object *obj, FormField *fieldA) : Annot(docA, dictObject->copy(), obj)
{
    type = typeWidget;
    field = fieldA;
    initialize(dictObject->getDict());
}

AnnotWidget::~AnnotWidget() = default;

void AnnotWidget::initialize(Dict *dict)
{
    Object obj1;

    form = doc->getCatalog()->getForm();

    obj1 = dict->lookup("H");
    if (obj1.isName()) {
        const char *modeName = obj1.getName();

        if (!strcmp(modeName, "N")) {
            mode = highlightModeNone;
        } else if (!strcmp(modeName, "O")) {
            mode = highlightModeOutline;
        } else if (!strcmp(modeName, "P") || !strcmp(modeName, "T")) {
            mode = highlightModePush;
        } else {
            mode = highlightModeInvert;
        }
    } else {
        mode = highlightModeInvert;
    }

    obj1 = dict->lookup("MK");
    if (obj1.isDict()) {
        appearCharacs = std::make_unique<AnnotAppearanceCharacs>(obj1.getDict());
    }

    obj1 = dict->lookup("A");
    if (obj1.isDict()) {
        action = LinkAction::parseAction(&obj1, doc->getCatalog()->getBaseURI());
    }

    additionalActions = dict->lookupNF("AA").copy();

    obj1 = dict->lookup("Parent");
    if (obj1.isDict()) {
        parent = nullptr;
    } else {
        parent = nullptr;
    }

    obj1 = dict->lookup("BS");
    if (obj1.isDict()) {
        border = std::make_unique<AnnotBorderBS>(obj1.getDict());
    }

    updatedAppearanceStream = Ref::INVALID();
}

std::unique_ptr<LinkAction> AnnotWidget::getAdditionalAction(AdditionalActionsType additionalActionType)
{
    return ::getAdditionalAction(additionalActionType, &additionalActions, doc);
}

std::unique_ptr<LinkAction> AnnotWidget::getFormAdditionalAction(FormAdditionalActionsType formAdditionalActionType)
{
    Object additionalActionsObject = additionalActions.fetch(doc->getXRef());

    if (additionalActionsObject.isDict()) {
        const char *key = getFormAdditionalActionKey(formAdditionalActionType);

        Object actionObject = additionalActionsObject.dictLookup(key);
        if (actionObject.isDict()) {
            return LinkAction::parseAction(&actionObject, doc->getCatalog()->getBaseURI());
        }
    }

    return nullptr;
}

bool AnnotWidget::setFormAdditionalAction(FormAdditionalActionsType formAdditionalActionType, const std::string &js)
{
    Object additionalActionsObject = additionalActions.fetch(doc->getXRef());

    if (!additionalActionsObject.isDict()) {
        additionalActionsObject = Object(std::make_unique<Dict>(doc->getXRef()));
        annotObj.dictSet("AA", additionalActionsObject.copy());
    }

    additionalActionsObject.dictSet(getFormAdditionalActionKey(formAdditionalActionType), LinkJavaScript::createObject(doc->getXRef(), js));

    if (additionalActions.isRef()) {
        doc->getXRef()->setModifiedObject(&additionalActionsObject, additionalActions.getRef());
    } else if (hasRef) {
        doc->getXRef()->setModifiedObject(&annotObj, ref);
    } else {
        error(errInternal, -1, "AnnotWidget::setFormAdditionalAction, where neither additionalActions is ref nor annotobj itself is ref");
        return false;
    }
    return true;
}