// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 75/79



    obj1 = dict->lookup("A");
    if (obj1.isName()) {
        const char *name = obj1.getName();

        if (!strcmp(name, "PO")) {
            aTrigger = aTriggerPageOpened;
        } else if (!strcmp(name, "PV")) {
            aTrigger = aTriggerPageVisible;
        } else if (!strcmp(name, "XA")) {
            aTrigger = aTriggerUserAction;
        } else {
            aTrigger = aTriggerUnknown;
        }
    } else {
        aTrigger = aTriggerUnknown;
    }

    obj1 = dict->lookup("AIS");
    if (obj1.isName()) {
        const char *name = obj1.getName();

        if (!strcmp(name, "I")) {
            aState = aStateEnabled;
        } else if (!strcmp(name, "L")) {
            aState = aStateDisabled;
        } else {
            aState = aStateUnknown;
        }
    } else {
        aState = aStateUnknown;
    }

    obj1 = dict->lookup("D");
    if (obj1.isName()) {
        const char *name = obj1.getName();

        if (!strcmp(name, "PC")) {
            dTrigger = dTriggerPageClosed;
        } else if (!strcmp(name, "PI")) {
            dTrigger = dTriggerPageInvisible;
        } else if (!strcmp(name, "XD")) {
            dTrigger = dTriggerUserAction;
        } else {
            dTrigger = dTriggerUnknown;
        }
    } else {
        dTrigger = dTriggerUnknown;
    }

    obj1 = dict->lookup("DIS");
    if (obj1.isName()) {
        const char *name = obj1.getName();

        if (!strcmp(name, "U")) {
            dState = dStateUninstantiaded;
        } else if (!strcmp(name, "I")) {
            dState = dStateInstantiated;
        } else if (!strcmp(name, "L")) {
            dState = dStateLive;
        } else {
            dState = dStateUnknown;
        }
    } else {
        dState = dStateUnknown;
    }

    displayToolbar = dict->lookup("TB").getBoolWithDefaultValue(true);

    displayNavigation = dict->lookup("NP").getBoolWithDefaultValue(false);
}

//------------------------------------------------------------------------
// AnnotRichMedia
//------------------------------------------------------------------------
AnnotRichMedia::AnnotRichMedia(PDFDoc *docA, PDFRectangle *rectA) : Annot(docA, rectA)
{
    type = typeRichMedia;

    annotObj.dictSet("Subtype", Object(objName, "RichMedia"));

    initialize(annotObj.getDict());
}

AnnotRichMedia::AnnotRichMedia(PDFDoc *docA, Object &&dictObject, const Object *obj) : Annot(docA, std::move(dictObject), obj)
{
    type = typeRichMedia;
    initialize(annotObj.getDict());
}

AnnotRichMedia::~AnnotRichMedia() = default;

void AnnotRichMedia::initialize(Dict *dict)
{
    Object obj1 = dict->lookup("RichMediaContent");
    if (obj1.isDict()) {
        content = std::make_unique<AnnotRichMedia::Content>(obj1.getDict());
    }

    obj1 = dict->lookup("RichMediaSettings");
    if (obj1.isDict()) {
        settings = std::make_unique<AnnotRichMedia::Settings>(obj1.getDict());
    }
}

AnnotRichMedia::Content *AnnotRichMedia::getContent() const
{
    return content.get();
}

AnnotRichMedia::Settings *AnnotRichMedia::getSettings() const
{
    return settings.get();
}

AnnotRichMedia::Settings::Settings(Dict *dict)
{
    Object obj1 = dict->lookup("Activation");
    if (obj1.isDict()) {
        activation = std::make_unique<AnnotRichMedia::Activation>(obj1.getDict());
    }

    obj1 = dict->lookup("Deactivation");
    if (obj1.isDict()) {
        deactivation = std::make_unique<AnnotRichMedia::Deactivation>(obj1.getDict());
    }
}

AnnotRichMedia::Settings::~Settings() = default;

AnnotRichMedia::Activation *AnnotRichMedia::Settings::getActivation() const
{
    return activation.get();
}

AnnotRichMedia::Deactivation *AnnotRichMedia::Settings::getDeactivation() const
{
    return deactivation.get();
}

AnnotRichMedia::Activation::Activation(Dict *dict)
{
    Object obj1 = dict->lookup("Condition");
    if (obj1.isName()) {
        const char *name = obj1.getName();

        if (!strcmp(name, "PO")) {
            condition = conditionPageOpened;
        } else if (!strcmp(name, "PV")) {
            condition = conditionPageVisible;
        } else if (!strcmp(name, "XA")) {
            condition = conditionUserAction;
        } else {
            condition = conditionUserAction;
        }
    } else {
        condition = conditionUserAction;
    }
}

AnnotRichMedia::Activation::Condition AnnotRichMedia::Activation::getCondition() const
{
    return condition;
}

AnnotRichMedia::Deactivation::Deactivation(Dict *dict)
{
    Object obj1 = dict->lookup("Condition");
    if (obj1.isName()) {
        const char *name = obj1.getName();

        if (!strcmp(name, "PC")) {
            condition = conditionPageClosed;
        } else if (!strcmp(name, "PI")) {
            condition = conditionPageInvisible;
        } else if (!strcmp(name, "XD")) {
            condition = conditionUserAction;
        } else {
            condition = conditionUserAction;
        }
    } else {
        condition = conditionUserAction;
    }
}