// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 59/79



    // draw the appearance stream
    Object obj = appearance.fetch(gfx->getXRef());
    if (appearBBox) {
        gfx->drawAnnot(&obj, nullptr, color.get(), appearBBox->getPageXMin(), appearBBox->getPageYMin(), appearBBox->getPageXMax(), appearBBox->getPageYMax(), getRotation());
    } else {
        gfx->drawAnnot(&obj, nullptr, color.get(), rect->x1, rect->y1, rect->x2, rect->y2, getRotation());
    }
}

void AnnotStamp::setIcon(const std::string &new_icon)
{
    icon = new_icon;

    update("Name", Object(objName, icon.c_str()));
    invalidateAppearance();
}

void AnnotStamp::setCustomImage(std::unique_ptr<AnnotStampImageHelper> &&stampImageHelperA)
{
    if (!stampImageHelperA) {
        return;
    }

    annotLocker();
    if (stampImageHelper) {
        stampImageHelper->removeAnnotStampImageObject();
    }

    stampImageHelper = std::move(stampImageHelperA);

    // Regenerate appearance stream
    invalidateAppearance();
    updateAppearanceResDict();
}

//------------------------------------------------------------------------
// AnnotGeometry
//------------------------------------------------------------------------
AnnotGeometry::AnnotGeometry(PDFDoc *docA, PDFRectangle *rectA, AnnotSubtype subType) : AnnotMarkup(docA, rectA)
{
    switch (subType) {
    case typeSquare:
        annotObj.dictSet("Subtype", Object(objName, "Square"));
        break;
    case typeCircle:
        annotObj.dictSet("Subtype", Object(objName, "Circle"));
        break;
    default:
        assert(0 && "Invalid subtype for AnnotGeometry\n");
    }

    initialize(annotObj.getDict());
}

AnnotGeometry::AnnotGeometry(PDFDoc *docA, Object &&dictObject, const Object *obj) : AnnotMarkup(docA, std::move(dictObject), obj)
{
    // the real type will be read in initialize()
    type = typeSquare;
    initialize(annotObj.getDict());
}

AnnotGeometry::~AnnotGeometry() = default;

void AnnotGeometry::initialize(Dict *dict)
{
    Object obj1;

    obj1 = dict->lookup("Subtype");
    if (obj1.isName()) {
        if (obj1.isName("Square")) {
            type = typeSquare;
        } else if (obj1.isName("Circle")) {
            type = typeCircle;
        }
    }

    obj1 = dict->lookup("IC");
    if (obj1.isArray()) {
        interiorColor = std::make_unique<AnnotColor>(*obj1.getArray());
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
        geometryRect = parseDiffRectangle(obj1.getArray(), rect.get());
    }
}

void AnnotGeometry::setType(AnnotSubtype new_type)
{
    const char *typeName = nullptr; /* squelch bogus compiler warning */

    switch (new_type) {
    case typeSquare:
        typeName = "Square";
        break;
    case typeCircle:
        typeName = "Circle";
        break;
    default:
        assert(!"Invalid subtype");
    }

    type = new_type;
    update("Subtype", Object(objName, typeName));
    invalidateAppearance();
}

void AnnotGeometry::setInteriorColor(std::unique_ptr<AnnotColor> &&new_color)
{
    if (new_color) {
        Object obj1 = new_color->writeToObject(doc->getXRef());
        update("IC", std::move(obj1));
        interiorColor = std::move(new_color);
    } else {
        interiorColor = nullptr;
        update("IC", Object::null());
    }
    invalidateAppearance();
}

void AnnotGeometry::draw(Gfx *gfx, bool printing)
{
    double ca = 1;

    if (!isVisible(printing)) {
        return;
    }

    annotLocker();
    if (appearance.isNull()) {
        const bool fill = interiorColor && interiorColor->getSpace() != AnnotColor::colorTransparent;
        ca = opacity;

        AnnotAppearanceBuilder appearBuilder;
        appearBuilder.append("q\n");
        if (color) {
            appearBuilder.setDrawColor(*color, false);
        }

        double borderWidth = border->getWidth();
        appearBuilder.setLineStyleForBorder(*border);

        if (interiorColor) {
            appearBuilder.setDrawColor(*interiorColor, true);
        }