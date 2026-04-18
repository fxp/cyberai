// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 36/79



// if fontName is empty it is assumed it is sent from the outside
// so for text that is in font no Tf is added and for text that is in the aux fonts
// a pair of q/Q is added
static DrawMultiLineTextResult drawMultiLineText(const std::string &text, double availableWidth, const Form *form, const GfxFont &font, const std::string &fontName, double fontSize, VariableTextQuadding quadding, double borderWidth)
{
    DrawMultiLineTextResult result;
    size_t i = 0;
    double xPosPrev = 0;
    const double availableTextWidthInFontPtSize = availableWidth / fontSize;
    while (i < text.size()) {
        GooString lineText(text.substr(i));
        if (!hasUnicodeByteOrderMark(lineText.toStr()) && hasUnicodeByteOrderMark(text)) {
            prependUnicodeByteOrderMark(lineText.toNonConstStr());
        }
        const HorizontalTextLayouter textLayouter(&lineText, form, &font, availableTextWidthInFontPtSize, false);

        const double totalWidth = textLayouter.totalWidth() * fontSize;

        auto calculateX = [quadding, availableWidth, totalWidth, borderWidth] {
            switch (quadding) {
            case VariableTextQuadding::centered:
                return (availableWidth - totalWidth) / 2;
                break;
            case VariableTextQuadding::rightJustified:
                return availableWidth - totalWidth - borderWidth;
                break;
            default: // VariableTextQuadding::lLeftJustified:
                return borderWidth;
                break;
            }
        };
        const double xPos = calculateX();

        AnnotAppearanceBuilder builder;
        bool first = true;
        double prevBlockWidth = 0;
        for (const HorizontalTextLayouter::Data &d : textLayouter.data) {
            const std::string &fName = d.fontName.empty() ? fontName : d.fontName;
            if (!fName.empty()) {
                if (fontName.empty()) {
                    builder.append(" q\n");
                }
                builder.appendf("/{0:s} {1:.2f} Tf\n", fName.c_str(), fontSize);
            }

            const double yDiff = first ? -fontSize : 0;
            const double xDiff = first ? xPos - xPosPrev : prevBlockWidth;

            builder.appendf("{0:.2f} {1:.2f} Td\n", xDiff, yDiff);
            builder.writeString(d.text);
            builder.append(" Tj\n");
            first = false;
            prevBlockWidth = d.width * fontSize;

            if (!fName.empty() && fontName.empty()) {
                builder.append(" Q\n");
            }
        }
        xPosPrev = xPos + totalWidth - prevBlockWidth;

        result.text += builder.buffer()->toStr();
        result.nLines += 1;
        if (i == 0) {
            i += textLayouter.consumedText;
        } else {
            i += textLayouter.consumedText - (hasUnicodeByteOrderMark(text) ? 2 : 0);
        }
    }
    return result;
}

void AnnotFreeText::generateFreeTextAppearance()
{
    double borderWidth, ca = opacity;

    AnnotAppearanceBuilder appearBuilder;
    appearBuilder.append("q\n");

    borderWidth = border->getWidth();
    if (borderWidth > 0) {
        appearBuilder.setLineStyleForBorder(*border);
    }

    // Box size
    const double width = rect->x2 - rect->x1;
    const double height = rect->y2 - rect->y1;

    // Parse some properties from the appearance string
    DefaultAppearance da { appearanceString.get() };

    // Default values
    if (da.getFontName().empty()) {
        da.setFontName("AnnotDrawFont");
    }
    if (da.getFontPtSize() <= 0) {
        da.setFontPtSize(undefinedFontPtSize);
    }
    if (!da.getFontColor()) {
        da.setFontColor(std::make_unique<AnnotColor>(0, 0, 0));
    }
    if (!contents) {
        contents = std::make_unique<GooString>();
    }

    // Draw box
    bool doFill = (color && color->getSpace() != AnnotColor::colorTransparent);
    bool doStroke = (borderWidth != 0);
    if (doFill || doStroke) {
        if (doStroke) {
            appearBuilder.setDrawColor(*da.getFontColor(), false); // Border color: same as font color
        }
        appearBuilder.appendf("{0:.2f} {0:.2f} {1:.2f} {2:.2f} re\n", borderWidth / 2, width - borderWidth, height - borderWidth);
        if (doFill) {
            appearBuilder.setDrawColor(*color, true);
            appearBuilder.append(doStroke ? "B\n" : "f\n");
        } else {
            appearBuilder.append("S\n");
        }
    }

    // Setup text clipping
    const double textmargin = borderWidth * 2;
    const double textwidth = width - 2 * textmargin;
    appearBuilder.appendf("{0:.2f} {0:.2f} {1:.2f} {2:.2f} re W n\n", textmargin, textwidth, height - 2 * textmargin);

    std::unique_ptr<const GfxFont> font = nullptr;

    // look for font name in the default resources
    Form *form = doc->getCatalog()->getForm(); // form is owned by catalog, no need to clean it up