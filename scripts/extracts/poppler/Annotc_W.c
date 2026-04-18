// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 55/79



        // Write the AP dictionary
        obj1 = Object(std::make_unique<Dict>(doc->getXRef()));
        obj1.dictAdd("N", Object(updatedAppearanceStream));

        // Update our internal pointers to the appearance dictionary
        appearStreams = std::make_unique<AnnotAppearance>(doc, &obj1);

        update("AP", std::move(obj1));
    } else {
        // Replace the existing appearance stream
        doc->getXRef()->setModifiedObject(&obj1, updatedAppearanceStream);
    }
}

void AnnotWidget::draw(Gfx *gfx, bool printing)
{
    if (!isVisible(printing)) {
        return;
    }

    annotLocker();

    // Only construct the appearance stream when
    // - annot doesn't have an AP or
    // - NeedAppearances is true and it isn't a Signature. There isn't enough data in our objects to generate it for signatures
    bool addedDingbatsResource = false;
    if (field) {
        if (appearance.isNull() || (field->getType() != FormFieldType::formSignature && form && form->getNeedAppearances())) {
            generateFieldAppearance(&addedDingbatsResource);
        }
    }

    // draw the appearance stream
    Object obj = appearance.fetch(gfx->getXRef());
    if (addedDingbatsResource) {
        // We are forcing ZaDb but the font does not exist
        // so create a fake one
        // If refactoring this code remember to test issue #1642 afterwards
        auto fontDict = std::make_unique<Dict>(gfx->getXRef());
        fontDict->add("BaseFont", Object(objName, "ZapfDingbats"));
        fontDict->add("Subtype", Object(objName, "Type1"));

        auto fontsDict = std::make_unique<Dict>(gfx->getXRef());
        fontsDict->add("ZaDb", Object(std::move(fontDict)));

        Dict dict(gfx->getXRef());
        dict.add("Font", Object(std::move(fontsDict)));
        gfx->pushResources(&dict);
    }
    gfx->drawAnnot(&obj, nullptr, color.get(), rect->x1, rect->y1, rect->x2, rect->y2, getRotation());
    if (addedDingbatsResource) {
        gfx->popResources();
    }
}

//------------------------------------------------------------------------
// AnnotMovie
//------------------------------------------------------------------------
AnnotMovie::AnnotMovie(PDFDoc *docA, PDFRectangle *rectA, Movie *movieA) : Annot(docA, rectA)
{
    type = typeMovie;
    annotObj.dictSet("Subtype", Object(objName, "Movie"));

    movie = movieA->copy();
    // TODO: create movie dict from movieA

    initialize(annotObj.getDict());
}

AnnotMovie::AnnotMovie(PDFDoc *docA, Object &&dictObject, const Object *obj) : Annot(docA, std::move(dictObject), obj)
{
    type = typeMovie;
    initialize(annotObj.getDict());
}

AnnotMovie::~AnnotMovie() = default;

void AnnotMovie::initialize(Dict *dict)
{
    Object obj1;

    obj1 = dict->lookup("T");
    if (obj1.isString()) {
        title = obj1.takeString();
    }

    Object movieDict = dict->lookup("Movie");
    if (movieDict.isDict()) {
        Object obj2 = dict->lookup("A");
        if (obj2.isDict()) {
            movie = std::make_unique<Movie>(&movieDict, &obj2);
        } else {
            movie = std::make_unique<Movie>(&movieDict);
        }
        if (!movie->isOk()) {
            movie = nullptr;
            ok = false;
        }
    } else {
        error(errSyntaxError, -1, "Bad Annot Movie");
        ok = false;
    }
}

void AnnotMovie::draw(Gfx *gfx, bool printing)
{
    if (!isVisible(printing)) {
        return;
    }

    annotLocker();
    if (appearance.isNull() && movie->getShowPoster()) {
        int width, height;
        Object poster = movie->getPoster();
        movie->getAspect(&width, &height);

        if (width != -1 && height != -1 && !poster.isNone()) {
            auto appearBuf = std::make_unique<GooString>();
            appearBuf->append("q\n");
            appearBuf->appendf("{0:d} 0 0 {1:d} 0 0 cm\n", width, height);
            appearBuf->append("/MImg Do\n");
            appearBuf->append("Q\n");

            auto imgDict = std::make_unique<Dict>(gfx->getXRef());
            imgDict->set("MImg", std::move(poster));

            auto resDict = std::make_unique<Dict>(gfx->getXRef());
            resDict->set("XObject", Object(std::move(imgDict)));