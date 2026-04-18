// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 37/79



    Object resourceObj;
    if (form && form->getDefaultResourcesObj() && form->getDefaultResourcesObj()->isDict()) {
        resourceObj = form->getDefaultResourcesObj()->copy(); // No real copy, but increment refcount of /DR Dict

        Dict *resDict = resourceObj.getDict();
        Object fontResources = resDict->lookup("Font"); // The 'Font' subdictionary

        if (!fontResources.isDict()) {
            error(errSyntaxWarning, -1, "Font subdictionary is not a dictionary");
        } else {
            // Get the font dictionary for the actual requested font
            Ref fontReference;
            Object fontDictionary = fontResources.getDict()->lookup(da.getFontName(), &fontReference);

            if (fontDictionary.isDict()) {
                font = GfxFont::makeFont(doc->getXRef(), da.getFontName().c_str(), fontReference, *fontDictionary.getDict());
            } else {
                error(errSyntaxWarning, -1, "Font dictionary is not a dictionary");
            }
        }
    }

    // if fontname is not in the default resources, create a Helvetica fake font
    if (!font) {
        auto fontResDict = std::make_unique<Dict>(doc->getXRef());
        font = createAnnotDrawFont(doc->getXRef(), fontResDict.get(), da.getFontName().c_str());
        resourceObj = Object(std::move(fontResDict));
    }

    // Set font state
    appearBuilder.setDrawColor(*da.getFontColor(), true);
    appearBuilder.appendf("BT 1 0 0 1 {0:.2f} {1:.2f} Tm\n", textmargin, height - textmargin);
    const DrawMultiLineTextResult textCommands = drawMultiLineText(contents->toStr(), textwidth, form, *font, da.getFontName(), da.getFontPtSize(), quadding, 0 /*borderWidth*/);
    appearBuilder.append(textCommands.text.c_str());
    appearBuilder.append("ET Q\n");

    const std::array<double, 4> bbox = { 0, 0, rect->x2 - rect->x1, rect->y2 - rect->y1 };

    Object newAppearance;
    if (ca == 1) {
        newAppearance = createForm(appearBuilder.buffer(), bbox, false, std::move(resourceObj));
    } else {
        Object aStream = createForm(appearBuilder.buffer(), bbox, true, std::move(resourceObj));

        GooString appearBuf("/GS0 gs\n/Fm0 Do");
        std::unique_ptr<Dict> resDict = createResourcesDict("Fm0", std::move(aStream), "GS0", ca, nullptr);
        newAppearance = createForm(&appearBuf, bbox, false, std::move(resDict));
    }
    if (hasBeenUpdated) {
        // We should technically do this for all annots but AnnotFreeText
        // is particularly special since we're potentially embeddeing a font so we really need
        // to set the AP and not let other renderers guess it from the contents
        setNewAppearance(std::move(newAppearance));
    } else {
        appearance = std::move(newAppearance);
    }
}

void AnnotFreeText::draw(Gfx *gfx, bool printing)
{
    if (!isVisible(printing)) {
        return;
    }

    annotLocker();
    if (appearance.isNull()) {
        generateFreeTextAppearance();
    }

    // draw the appearance stream
    Object obj = appearance.fetch(gfx->getXRef());
    gfx->drawAnnot(&obj, nullptr, color.get(), rect->x1, rect->y1, rect->x2, rect->y2, getRotation());
}

// Before retrieving the res dict, regenerate the appearance stream if needed,
// because AnnotFreeText::draw needs to store font info in the res dict
Object AnnotFreeText::getAppearanceResDict()
{
    if (appearance.isNull()) {
        generateFreeTextAppearance();
    }
    return Annot::getAppearanceResDict();
}

//------------------------------------------------------------------------
// AnnotLine
//------------------------------------------------------------------------

AnnotLine::AnnotLine(PDFDoc *docA, PDFRectangle *rectA) : AnnotMarkup(docA, rectA)
{
    type = typeLine;
    annotObj.dictSet("Subtype", Object(objName, "Line"));

    initialize(annotObj.getDict());
}

AnnotLine::AnnotLine(PDFDoc *docA, Object &&dictObject, const Object *obj) : AnnotMarkup(docA, std::move(dictObject), obj)
{
    type = typeLine;
    initialize(annotObj.getDict());
}

AnnotLine::~AnnotLine() = default;

void AnnotLine::initialize(Dict *dict)
{
    Object obj1;

    obj1 = dict->lookup("L");
    if (obj1.isArrayOfLength(4)) {
        const double x1 = obj1.arrayGet(0).getNumWithDefaultValue(0);
        const double y1 = obj1.arrayGet(1).getNumWithDefaultValue(0);
        const double x2 = obj1.arrayGet(2).getNumWithDefaultValue(0);
        const double y2 = obj1.arrayGet(3).getNumWithDefaultValue(0);

        coord1 = std::make_unique<AnnotCoord>(x1, y1);
        coord2 = std::make_unique<AnnotCoord>(x2, y2);
    } else {
        coord1 = std::make_unique<AnnotCoord>();
        coord2 = std::make_unique<AnnotCoord>();
    }

    obj1 = dict->lookup("LE");
    if (obj1.isArrayOfLength(2)) {
        Object obj2;

        obj2 = obj1.arrayGet(0);
        if (obj2.isName()) {
            startStyle = parseAnnotLineEndingStyle(obj2);
        } else {
            startStyle = annotLineEndingNone;
        }