// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 33/79



void AnnotLink::draw(Gfx *gfx, bool printing)
{
    if (!isVisible(printing)) {
        return;
    }

    annotLocker();
    // draw the appearance stream
    Object obj = appearance.fetch(gfx->getXRef());
    gfx->drawAnnot(&obj, border.get(), color.get(), rect->x1, rect->y1, rect->x2, rect->y2, getRotation());
}

//------------------------------------------------------------------------
// AnnotFreeText
//------------------------------------------------------------------------
const double AnnotFreeText::undefinedFontPtSize = 10.;

AnnotFreeText::AnnotFreeText(PDFDoc *docA, PDFRectangle *rectA) : AnnotMarkup(docA, rectA)
{
    type = typeFreeText;

    annotObj.dictSet("Subtype", Object(objName, "FreeText"));
    annotObj.dictSet("DA", Object(std::make_unique<GooString>()));

    initialize(annotObj.getDict());
}

AnnotFreeText::AnnotFreeText(PDFDoc *docA, Object &&dictObject, const Object *obj) : AnnotMarkup(docA, std::move(dictObject), obj)
{
    type = typeFreeText;
    initialize(annotObj.getDict());
}

AnnotFreeText::~AnnotFreeText() = default;

void AnnotFreeText::initialize(Dict *dict)
{
    Object obj1;

    obj1 = dict->lookup("DA");
    if (obj1.isString()) {
        appearanceString = obj1.takeString();
    } else {
        appearanceString = std::make_unique<GooString>();
        error(errSyntaxWarning, -1, "Bad appearance for annotation");
    }

    obj1 = dict->lookup("Q");
    if (obj1.isInt()) {
        quadding = (VariableTextQuadding)obj1.getInt();
    } else {
        quadding = VariableTextQuadding::leftJustified;
    }

    obj1 = dict->lookup("DS");
    if (obj1.isString()) {
        styleString = obj1.takeString();
    }

    obj1 = dict->lookup("CL");
    if (obj1.isArrayOfLengthAtLeast(4)) {
        const double x1 = obj1.arrayGet(0).getNumWithDefaultValue(0);
        const double y1 = obj1.arrayGet(1).getNumWithDefaultValue(0);
        const double x2 = obj1.arrayGet(2).getNumWithDefaultValue(0);
        const double y2 = obj1.arrayGet(3).getNumWithDefaultValue(0);

        if (obj1.arrayGetLength() == 6) {
            const double x3 = obj1.arrayGet(4).getNumWithDefaultValue(0);
            const double y3 = obj1.arrayGet(5).getNumWithDefaultValue(0);
            calloutLine = std::make_unique<AnnotCalloutMultiLine>(x1, y1, x2, y2, x3, y3);
        } else {
            calloutLine = std::make_unique<AnnotCalloutLine>(x1, y1, x2, y2);
        }
    }

    obj1 = dict->lookup("IT");
    if (obj1.isName()) {
        const char *intentName = obj1.getName();

        if (!strcmp(intentName, "FreeText")) {
            intent = intentFreeText;
        } else if (!strcmp(intentName, "FreeTextCallout")) {
            intent = intentFreeTextCallout;
        } else if (!strcmp(intentName, "FreeTextTypeWriter")) {
            intent = intentFreeTextTypeWriter;
        } else {
            intent = intentFreeText;
        }
    } else {
        intent = intentFreeText;
    }

    obj1 = dict->lookup("BS");
    if (obj1.isDict()) {
        border = std::make_unique<AnnotBorderBS>(obj1.getDict());
    } else if (!border) {
        border = std::make_unique<AnnotBorderBS>();
    }

    obj1 = dict->lookup("BE");
    if (obj1.isDict()) {
        borderEffect = std::make_unique<AnnotBorderEffect>(obj1.getDict());
    }

    obj1 = dict->lookup("RD");
    if (obj1.isArray()) {
        rectangle = parseDiffRectangle(obj1.getArray(), rect.get());
    }

    obj1 = dict->lookup("LE");
    if (obj1.isName()) {
        endStyle = parseAnnotLineEndingStyle(obj1);
    } else {
        endStyle = annotLineEndingNone;
    }
}

void AnnotFreeText::setContents(std::unique_ptr<GooString> &&new_content)
{
    Annot::setContents(std::move(new_content));
    invalidateAppearance();
}

void AnnotFreeText::setDefaultAppearance(const DefaultAppearance &da)
{
    appearanceString = std::make_unique<GooString>(da.toAppearanceString());

    update("DA", Object(appearanceString->copy()));
    invalidateAppearance();
}

void AnnotFreeText::setQuadding(VariableTextQuadding new_quadding)
{
    quadding = new_quadding;
    update("Q", Object((int)quadding));
    invalidateAppearance();
}

void AnnotFreeText::setStyleString(GooString *new_string)
{
    if (new_string) {
        styleString = new_string->copy();
        // append the unicode marker <FE FF> if needed
        if (!hasUnicodeByteOrderMark(styleString->toStr())) {
            prependUnicodeByteOrderMark(styleString->toNonConstStr());
        }
    } else {
        styleString = std::make_unique<GooString>();
    }

    update("DS", Object(styleString->copy()));
}