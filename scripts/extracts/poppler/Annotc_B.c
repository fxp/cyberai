// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 34/79



void AnnotFreeText::setCalloutLine(std::unique_ptr<AnnotCalloutLine> &&line)
{
    Object obj1;
    if (line == nullptr) {
        obj1.setToNull();
        calloutLine = nullptr;
    } else {
        double x1 = line->getX1(), y1 = line->getY1();
        double x2 = line->getX2(), y2 = line->getY2();
        obj1 = Object(std::make_unique<Array>(doc->getXRef()));
        obj1.arrayAdd(Object(x1));
        obj1.arrayAdd(Object(y1));
        obj1.arrayAdd(Object(x2));
        obj1.arrayAdd(Object(y2));

        auto *mline = dynamic_cast<AnnotCalloutMultiLine *>(line.get());
        if (mline) {
            double x3 = mline->getX3(), y3 = mline->getY3();
            obj1.arrayAdd(Object(x3));
            obj1.arrayAdd(Object(y3));
        }
        calloutLine = std::move(line);
    }

    update("CL", std::move(obj1));
    invalidateAppearance();
}

void AnnotFreeText::setIntent(AnnotFreeTextIntent new_intent)
{
    const char *intentName;

    intent = new_intent;
    if (new_intent == intentFreeText) {
        intentName = "FreeText";
    } else if (new_intent == intentFreeTextCallout) {
        intentName = "FreeTextCallout";
    } else { // intentFreeTextTypeWriter
        intentName = "FreeTextTypeWriter";
    }
    update("IT", Object(objName, intentName));
}

std::unique_ptr<DefaultAppearance> AnnotFreeText::getDefaultAppearance() const
{
    return std::make_unique<DefaultAppearance>(appearanceString.get());
}

static std::unique_ptr<GfxFont> createAnnotDrawFont(XRef *xref, Dict *fontParentDict, const char *resourceName = "AnnotDrawFont", const char *fontname = "Helvetica")
{
    const Ref dummyRef = { .num = -1, .gen = -1 };

    auto fontDict = std::make_unique<Dict>(xref);
    fontDict->add("BaseFont", Object(objName, fontname));
    fontDict->add("Subtype", Object(objName, "Type1"));
    if ((strcmp(fontname, "ZapfDingbats") != 0) && (strcmp(fontname, "Symbol") != 0)) {
        fontDict->add("Encoding", Object(objName, "WinAnsiEncoding"));
    }

    Object fontsDictObj = fontParentDict->lookup("Font");
    if (!fontsDictObj.isDict()) {
        fontsDictObj = Object(std::make_unique<Dict>(xref));
        fontParentDict->add("Font", fontsDictObj.copy()); // This is not a copy it's a ref
    }

    auto font = GfxFont::makeFont(xref, resourceName, dummyRef, *fontDict);
    fontsDictObj.dictSet(resourceName, Object(std::move(fontDict)));
    return font;
}

class HorizontalTextLayouter
{
public:
    HorizontalTextLayouter() = default;

    HorizontalTextLayouter(const GooString *text, const Form *form, const GfxFont *font, std::optional<double> availableWidth, const bool noReencode)
    {
        size_t i = 0;
        double blockWidth;
        bool newFontNeeded = false;
        GooString outputText;
        const bool isUnicode = hasUnicodeByteOrderMark(text->toStr());
        int charCount;

        Annot::layoutText(text, &outputText, &i, *font, &blockWidth, availableWidth ? *availableWidth : 0.0, &charCount, noReencode, !noReencode ? &newFontNeeded : nullptr);
        data.emplace_back(outputText.toStr(), std::string(), blockWidth, charCount);
        if (availableWidth) {
            *availableWidth -= blockWidth;
        }

        while (newFontNeeded && (!availableWidth || *availableWidth > 0 || (isUnicode && i == 2) || (!isUnicode && i == 0))) {
            if (!form) {
                // There's no fonts to look for, so just skip the characters
                i += isUnicode ? 2 : 1;
                error(errSyntaxError, -1, "HorizontalTextLayouter, found character that the font can't represent");
                newFontNeeded = false;
            } else {
                Unicode uChar;
                if (isUnicode) {
                    uChar = (unsigned char)(text->getChar(i)) << 8;
                    uChar += (unsigned char)(text->getChar(i + 1));
                } else {
                    uChar = pdfDocEncoding[text->getChar(i) & 0xff];
                }
                const std::string auxFontName = form->getFallbackFontForChar(uChar, *font);
                if (!auxFontName.empty()) {
                    std::shared_ptr<GfxFont> auxFont = form->getDefaultResources()->lookupFont(auxFontName.c_str());