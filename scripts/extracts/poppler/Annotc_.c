// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 63/79



    // Store dummy path with one null vertex only
    auto inkListArray = std::make_unique<Array>(doc->getXRef());
    auto vList = std::make_unique<Array>(doc->getXRef());
    vList->add(Object(0.));
    vList->add(Object(0.));
    inkListArray->add(Object(std::move(vList)));
    annotObj.dictSet("InkList", Object(std::move(inkListArray)));

    drawBelow = false;

    initialize(annotObj.getDict());
}

AnnotInk::AnnotInk(PDFDoc *docA, Object &&dictObject, const Object *obj) : AnnotMarkup(docA, std::move(dictObject), obj)
{
    type = typeInk;
    initialize(annotObj.getDict());
}

AnnotInk::~AnnotInk() = default;

void AnnotInk::initialize(Dict *dict)
{
    Object obj1;

    obj1 = dict->lookup("InkList");
    if (obj1.isArray()) {
        parseInkList(*obj1.getArray());
    } else {
        error(errSyntaxError, -1, "Bad Annot Ink List");

        obj1 = dict->lookup("AP");
        // Although InkList is required, it should be ignored
        // when there is an AP entry in the Annot, so do not fail
        // when that happens
        if (!obj1.isDict()) {
            ok = false;
        }
    }

    drawBelow = false;

    obj1 = getAppearanceResDict();
    if (obj1.isDict()) {
        const Object extGState = obj1.dictLookup("ExtGState");
        if (extGState.isDict()) {
            const Dict *extGStateDict = extGState.getDict();
            if (extGStateDict->getLength() > 0) {
                obj1 = extGStateDict->getVal(0);
                if (obj1.isDict() && obj1.dictLookup("BM").isName("Multiply")) {
                    drawBelow = true;
                }
            }
        }
    }

    obj1 = dict->lookup("BS");
    if (obj1.isDict()) {
        border = std::make_unique<AnnotBorderBS>(obj1.getDict());
    } else if (!border) {
        border = std::make_unique<AnnotBorderBS>();
    }
}

void AnnotInk::writeInkList(const std::vector<std::unique_ptr<AnnotPath>> &paths, Array *dest_array)
{
    for (const auto &path : paths) {
        auto a = std::make_unique<Array>(doc->getXRef());
        for (int j = 0; j < path->getCoordsLength(); ++j) {
            a->add(Object(path->getX(j)));
            a->add(Object(path->getY(j)));
        }
        dest_array->add(Object(std::move(a)));
    }
}

void AnnotInk::parseInkList(const Array &array)
{
    int inkListLength = array.getLength();
    inkList.clear();
    inkList.reserve(inkListLength);
    for (int i = 0; i < inkListLength; i++) {
        Object obj2 = array.get(i);
        if (obj2.isArray()) {
            inkList.push_back(std::make_unique<AnnotPath>(*obj2.getArray()));
        } else {
            inkList.emplace_back();
        }
    }
}

void AnnotInk::setInkList(const std::vector<std::unique_ptr<AnnotPath>> &paths)
{
    auto a = std::make_unique<Array>(doc->getXRef());
    writeInkList(paths, a.get());

    parseInkList(*a);
    annotObj.dictSet("InkList", Object(std::move(a)));
    invalidateAppearance();
    generateInkAppearance();
}

void AnnotInk::setDrawBelow(bool drawBelow_)
{
    drawBelow = drawBelow_;
    invalidateAppearance();
    generateInkAppearance();
}

bool AnnotInk::getDrawBelow() const
{
    return drawBelow;
}

void AnnotInk::generateInkAppearance()
{
    Object newAppearance;

    appearBBox = std::make_unique<AnnotAppearanceBBox>(rect.get());

    AnnotAppearanceBuilder appearBuilder;
    if (opacity != 1 || drawBelow) {
        appearBuilder.append("/GS0 gs\n");
    }
    appearBuilder.append("q\n");

    if (color) {
        appearBuilder.setDrawColor(*color, false);
    }

    if (border) {
        appearBuilder.setLineStyleForBorder(*border);
        /* It is easier to have the appearance bbox equal the annotation rect
        (especially when the appearance stream is saved to the file), so we
        don't add a border width to the bbox. This is not necessary as the
        border is never drawn and applications are responsible for setting a
        rect big enough in the first place.
        However, this border width was set before the drawBelow property was
        introduced and apps may rely on that behavior. Therefore, we set it in
        the case that appearance streams are not saved (i.e. when drawBelow is
        false) as it does not introduce bugs. */
        if (!drawBelow) {
            appearBBox->setBorderWidth(std::max(1.0, border->getWidth()));
        }
    }

    for (const auto &path : inkList) {
        if (path && path->getCoordsLength() != 0) {
            appearBuilder.appendf("{0:.2f} {1:.2f} m\n", path->getX(0) - rect->x1, path->getY(0) - rect->y1);
            appearBBox->extendTo(path->getX(0) - rect->x1, path->getY(0) - rect->y1);

            for (int j = 1; j < path->getCoordsLength(); ++j) {
                appearBuilder.appendf("{0:.2f} {1:.2f} l\n", path->getX(j) - rect->x1, path->getY(j) - rect->y1);
                appearBBox->extendTo(path->getX(j) - rect->x1, path->getY(j) - rect->y1);
            }

            appearBuilder.append("S\n");
        }
    }