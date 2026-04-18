// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 52/79



static void setChildDictEntryValue(Dict *parentDict, const char *childDictName, const char *childDictEntryName, const Ref childDictEntryValue, XRef *xref)
{
    Object childDictionaryObj = parentDict->lookup(childDictName);
    if (!childDictionaryObj.isDict()) {
        childDictionaryObj = Object(std::make_unique<Dict>(xref));
        parentDict->set(childDictName, childDictionaryObj.copy());
    }
    childDictionaryObj.dictSet(childDictEntryName, Object(childDictEntryValue));
}

bool AnnotAppearanceBuilder::drawSignatureFieldText(const FormFieldSignature *field, const Form *form, const GooString *_da, const AnnotBorder *border, const PDFRectangle *rect, XRef *xref, Dict *resourcesDict)
{
    const GooString &contents = field->getCustomAppearanceContent();
    if (contents.toStr().empty()) {
        return false;
    }

    if (field->getImageResource() != Ref::INVALID()) {
        const double width = rect->x2 - rect->x1;
        const double height = rect->y2 - rect->y1;
        static const char *imageResourceId = "SigImg";
        setChildDictEntryValue(resourcesDict, "XObject", imageResourceId, field->getImageResource(), xref);
        Matrix matrix = { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
        matrix.scale(width, height);
        const std::string imgBuffer = GooString::format("\nq {0:.1g} {1:.1g} {2:.1g} {3:.1g} {4:.1g} {5:.1g} cm /{6:s} Do Q\n", matrix.m[0], matrix.m[1], matrix.m[2], matrix.m[3], matrix.m[4], matrix.m[5], imageResourceId);
        append(imgBuffer.c_str());
    }

    const GooString &leftText = field->getCustomAppearanceLeftContent();
    if (leftText.toStr().empty()) {
        drawSignatureFieldText(contents.toStr(), form, DefaultAppearance(_da), border, rect, xref, resourcesDict, 0, false /* don't center vertically */, false /* don't center horizontally */);
    } else {
        const double halfWidth = (rect->x2 - rect->x1) / 2;

        double borderWidth = 0;

        if (border) {
            borderWidth = border->getWidth();
        }

        const double wMax = (rect->x2 - rect->x1) - 2 * borderWidth - 4;
        const double hMax = (rect->y2 - rect->y1) - 2 * borderWidth;

        DefaultAppearance daLeft(_da);

        double leftFontSize = field->getCustomAppearanceLeftFontSize();
        if (leftFontSize == 0) {
            std::shared_ptr<GfxFont> font = form->getDefaultResources()->lookupFont(daLeft.getFontName().c_str());
            leftFontSize = Annot::calculateFontSize(form, font.get(), &leftText, wMax / 2.0, hMax);
        }
        daLeft.setFontPtSize(leftFontSize);

        PDFRectangle rectLeft(rect->x1, rect->y1, rect->x1 + halfWidth, rect->y2);
        drawSignatureFieldText(leftText.toStr(), form, daLeft, border, &rectLeft, xref, resourcesDict, 0, true /* center vertically */, true /* center horizontally */);

        DefaultAppearance daRight(_da);

        double fontSize = daRight.getFontPtSize();
        if (fontSize == 0) {
            std::shared_ptr<GfxFont> font = form->getDefaultResources()->lookupFont(daLeft.getFontName().c_str());
            fontSize = Annot::calculateFontSize(form, font.get(), &contents, wMax / 2.0, hMax);
        }
        daRight.setFontPtSize(fontSize);

        PDFRectangle rectRight(rectLeft.x2, rect->y1, rect->x2, rect->y2);
        drawSignatureFieldText(contents.toStr(), form, daRight, border, &rectRight, xref, resourcesDict, halfWidth, true /* center vertically */, false /* don't center horizontally */);
    }

    return true;
}

void AnnotAppearanceBuilder::drawSignatureFieldText(const std::string &text, const Form *form, const DefaultAppearance &da, const AnnotBorder *border, const PDFRectangle *rect, XRef *xref, Dict *resourcesDict, double leftMargin,
                                                    bool centerVertically, bool centerHorizontally)
{
    double borderWidth = 0;
    append("q\n");

    if (border) {
        borderWidth = border->getWidth();
        if (borderWidth > 0) {
            setLineStyleForBorder(*border);
        }
    }

    // Box size
    const double width = rect->x2 - rect->x1;
    const double height = rect->y2 - rect->y1;
    const double textmargin = borderWidth * 2;
    const double textwidth = width - 2 * textmargin;

    // create a Helvetica fake font
    std::shared_ptr<const GfxFont> font = form ? form->getDefaultResources()->lookupFont(da.getFontName().c_str()) : nullptr;
    if (!font) {
        font = createAnnotDrawFont(xref, resourcesDict, da.getFontName().c_str());
    }

    // Setup text clipping
    appendf("{0:.2f} {1:.2f} {2:.2f} {3:.2f} re W n\n", leftMargin + textmargin, textmargin, textwidth, height - 2 * textmargin);
    setDrawColor(*da.getFontColor(), true);
    const DrawMultiLineTextResult textCommands =
            drawMultiLineText(text, textwidth, form, *font, da.getFontName(), da.getFontPtSize(), centerHorizontally ? VariableTextQuadding::centered : VariableTextQuadding::leftJustified, 0 /*borderWidth*/);