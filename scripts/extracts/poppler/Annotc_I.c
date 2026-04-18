// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 41/79



AnnotTextMarkup::AnnotTextMarkup(PDFDoc *docA, Object &&dictObject, const Object *obj) : AnnotMarkup(docA, std::move(dictObject), obj)
{
    // the real type will be read in initialize()
    type = typeHighlight;
    initialize(annotObj.getDict());
}

AnnotTextMarkup::~AnnotTextMarkup() = default;

void AnnotTextMarkup::initialize(Dict *dict)
{
    Object obj1;

    obj1 = dict->lookup("Subtype");
    if (obj1.isName()) {
        if (obj1.isName("Highlight")) {
            type = typeHighlight;
        } else if (obj1.isName("Underline")) {
            type = typeUnderline;
        } else if (obj1.isName("Squiggly")) {
            type = typeSquiggly;
        } else if (obj1.isName("StrikeOut")) {
            type = typeStrikeOut;
        }
    }

    obj1 = dict->lookup("QuadPoints");
    if (obj1.isArray()) {
        quadrilaterals = std::make_unique<AnnotQuadrilaterals>(*obj1.getArray());
    } else {
        error(errSyntaxError, -1, "Bad Annot Text Markup QuadPoints");
        ok = false;
    }
}

void AnnotTextMarkup::setType(AnnotSubtype new_type)
{
    const char *typeName = nullptr; /* squelch bogus compiler warning */

    switch (new_type) {
    case typeHighlight:
        typeName = "Highlight";
        break;
    case typeUnderline:
        typeName = "Underline";
        break;
    case typeSquiggly:
        typeName = "Squiggly";
        break;
    case typeStrikeOut:
        typeName = "StrikeOut";
        break;
    default:
        assert(!"Invalid subtype");
    }

    type = new_type;
    update("Subtype", Object(objName, typeName));
    invalidateAppearance();
}

void AnnotTextMarkup::setQuadrilaterals(const AnnotQuadrilaterals &quadPoints)
{
    auto a = std::make_unique<Array>(doc->getXRef());

    for (int i = 0; i < quadPoints.getQuadrilateralsLength(); ++i) {
        a->add(Object(quadPoints.getX1(i)));
        a->add(Object(quadPoints.getY1(i)));
        a->add(Object(quadPoints.getX2(i)));
        a->add(Object(quadPoints.getY2(i)));
        a->add(Object(quadPoints.getX3(i)));
        a->add(Object(quadPoints.getY3(i)));
        a->add(Object(quadPoints.getX4(i)));
        a->add(Object(quadPoints.getY4(i)));
    }

    quadrilaterals = std::make_unique<AnnotQuadrilaterals>(*a);

    annotObj.dictSet("QuadPoints", Object(std::move(a)));
    invalidateAppearance();
}

bool AnnotTextMarkup::shouldCreateApperance(Gfx *gfx) const
{
    if (appearance.isNull()) {
        return true;
    }

    // Adobe Reader seems to have a complex condition for when to use the
    // appearance stream of typeHighlight, which is "use it if it has a Resources dictionary with ExtGState"
    // this is reverse engineering of me editing a file by hand and checking what it does so the real
    // condition may be more or less complex
    if (type == typeHighlight) {
        XRef *xref = gfx->getXRef();
        const Object fetchedApperance = appearance.fetch(xref);
        if (fetchedApperance.isStream()) {
            const Object resources = fetchedApperance.streamGetDict()->lookup("Resources");
            if (resources.isDict()) {
                if (resources.dictLookup("ExtGState").isDict()) {
                    return false;
                }
            }
        }
        return true;
    }

    return false;
}

void AnnotTextMarkup::draw(Gfx *gfx, bool printing)
{
    double ca = 1;
    int i;

    if (!isVisible(printing)) {
        return;
    }

    annotLocker();
    if (shouldCreateApperance(gfx)) {
        bool blendMultiply = true;
        ca = opacity;

        AnnotAppearanceBuilder appearBuilder;
        appearBuilder.append("q\n");

        /* Adjust BBox */
        appearBBox = std::make_unique<AnnotAppearanceBBox>(rect.get());
        for (i = 0; i < quadrilaterals->getQuadrilateralsLength(); ++i) {
            appearBBox->extendTo(quadrilaterals->getX1(i) - rect->x1, quadrilaterals->getY1(i) - rect->y1);
            appearBBox->extendTo(quadrilaterals->getX2(i) - rect->x1, quadrilaterals->getY2(i) - rect->y1);
            appearBBox->extendTo(quadrilaterals->getX3(i) - rect->x1, quadrilaterals->getY3(i) - rect->y1);
            appearBBox->extendTo(quadrilaterals->getX4(i) - rect->x1, quadrilaterals->getY4(i) - rect->y1);
        }

        switch (type) {
        case typeUnderline:
            if (color) {
                appearBuilder.setDrawColor(*color, false);
            }
            appearBuilder.append("[] 0 d 1 w\n");
            // use a borderwidth, which is consistent with the line width
            appearBBox->setBorderWidth(1.0);

            for (i = 0; i < quadrilaterals->getQuadrilateralsLength(); ++i) {
                double x3, y3, x4, y4;

                x3 = quadrilaterals->getX3(i);
                y3 = quadrilaterals->getY3(i);
                x4 = quadrilaterals->getX4(i);
                y4 = quadrilaterals->getY4(i);