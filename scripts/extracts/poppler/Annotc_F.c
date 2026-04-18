// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 38/79



        obj2 = obj1.arrayGet(1);
        if (obj2.isName()) {
            endStyle = parseAnnotLineEndingStyle(obj2);
        } else {
            endStyle = annotLineEndingNone;
        }

    } else {
        startStyle = endStyle = annotLineEndingNone;
    }

    obj1 = dict->lookup("IC");
    if (obj1.isArray()) {
        interiorColor = std::make_unique<AnnotColor>(*obj1.getArray());
    }

    leaderLineLength = dict->lookup("LL").getNumWithDefaultValue(0);

    leaderLineExtension = dict->lookup("LLE").getNumWithDefaultValue(0);
    if (leaderLineExtension < 0) {
        leaderLineExtension = 0;
    }

    caption = dict->lookup("Cap").getBoolWithDefaultValue(false);

    obj1 = dict->lookup("IT");
    if (obj1.isName()) {
        const char *intentName = obj1.getName();

        if (!strcmp(intentName, "LineArrow")) {
            intent = intentLineArrow;
        } else if (!strcmp(intentName, "LineDimension")) {
            intent = intentLineDimension;
        } else {
            intent = intentLineArrow;
        }
    } else {
        intent = intentLineArrow;
    }

    leaderLineOffset = dict->lookup("LLO").getNumWithDefaultValue(0);
    if (leaderLineOffset < 0) {
        leaderLineOffset = 0;
    }

    obj1 = dict->lookup("CP");
    if (obj1.isName()) {
        const char *captionName = obj1.getName();

        if (!strcmp(captionName, "Inline")) {
            captionPos = captionPosInline;
        } else if (!strcmp(captionName, "Top")) {
            captionPos = captionPosTop;
        } else {
            captionPos = captionPosInline;
        }
    } else {
        captionPos = captionPosInline;
    }

    obj1 = dict->lookup("Measure");
    if (obj1.isDict()) {
        measure = nullptr;
    } else {
        measure = nullptr;
    }

    obj1 = dict->lookup("CO");
    if (obj1.isArrayOfLength(2)) {
        captionTextHorizontal = obj1.arrayGet(0).getNumWithDefaultValue(0);
        captionTextVertical = obj1.arrayGet(1).getNumWithDefaultValue(0);
    } else {
        captionTextHorizontal = captionTextVertical = 0;
    }

    obj1 = dict->lookup("BS");
    if (obj1.isDict()) {
        border = std::make_unique<AnnotBorderBS>(obj1.getDict());
    } else if (!border) {
        border = std::make_unique<AnnotBorderBS>();
    }
}

void AnnotLine::setContents(std::unique_ptr<GooString> &&new_content)
{
    Annot::setContents(std::move(new_content));
    if (caption) {
        invalidateAppearance();
    }
}

void AnnotLine::setVertices(double x1, double y1, double x2, double y2)
{
    coord1 = std::make_unique<AnnotCoord>(x1, y1);
    coord2 = std::make_unique<AnnotCoord>(x2, y2);

    auto lArray = std::make_unique<Array>(doc->getXRef());
    lArray->add(Object(x1));
    lArray->add(Object(y1));
    lArray->add(Object(x2));
    lArray->add(Object(y2));

    update("L", Object(std::move(lArray)));
    invalidateAppearance();
}

void AnnotLine::setStartEndStyle(AnnotLineEndingStyle start, AnnotLineEndingStyle end)
{
    startStyle = start;
    endStyle = end;

    auto leArray = std::make_unique<Array>(doc->getXRef());
    leArray->add(Object(objName, convertAnnotLineEndingStyle(startStyle)));
    leArray->add(Object(objName, convertAnnotLineEndingStyle(endStyle)));

    update("LE", Object(std::move(leArray)));
    invalidateAppearance();
}

void AnnotLine::setInteriorColor(std::unique_ptr<AnnotColor> &&new_color)
{
    if (new_color) {
        Object obj1 = new_color->writeToObject(doc->getXRef());
        update("IC", std::move(obj1));
        interiorColor = std::move(new_color);
    } else {
        interiorColor = nullptr;
    }
    invalidateAppearance();
}

void AnnotLine::setLeaderLineLength(double len)
{
    leaderLineLength = len;
    update("LL", Object(len));
    invalidateAppearance();
}

void AnnotLine::setLeaderLineExtension(double len)
{
    leaderLineExtension = len;
    update("LLE", Object(len));

    // LL is required if LLE is present
    update("LL", Object(leaderLineLength));
    invalidateAppearance();
}

void AnnotLine::setCaption(bool new_cap)
{
    caption = new_cap;
    update("Cap", Object(new_cap));
    invalidateAppearance();
}

void AnnotLine::setIntent(AnnotLineIntent new_intent)
{
    const char *intentName;

    intent = new_intent;
    if (new_intent == intentLineArrow) {
        intentName = "LineArrow";
    } else { // intentLineDimension
        intentName = "LineDimension";
    }
    update("IT", Object(objName, intentName));
}

void AnnotLine::generateLineAppearance()
{
    double borderWidth, ca = opacity;
    bool fill = false;