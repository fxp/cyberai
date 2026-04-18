// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 35/79



                    // Here we just layout one char, we don't know if the one afterwards can be layouted with the original font
                    GooString auxContents = GooString(text->toStr().substr(i, isUnicode ? 2 : 1));
                    if (isUnicode) {
                        prependUnicodeByteOrderMark(auxContents.toNonConstStr());
                    }
                    size_t auxI = 0;
                    Annot::layoutText(&auxContents, &outputText, &auxI, *auxFont, &blockWidth, availableWidth ? *availableWidth : 0.0, &charCount, false, &newFontNeeded);
                    assert(!newFontNeeded);
                    if (availableWidth) {
                        *availableWidth -= blockWidth;
                    }
                    // layoutText will always at least layout one character even if it doesn't fit in
                    // the given space which makes sense (except in the case of switching fonts, so we control if we ran out of space here manually)
                    // we also need to allow the character if we have not layouted anything yet because otherwise we will end up in an infinite loop
                    // because it is assumed we at least layout one character
                    if (!availableWidth || *availableWidth > 0 || (isUnicode && i == 2) || (!isUnicode && i == 0)) {
                        i += isUnicode ? 2 : 1;
                        data.emplace_back(outputText.toStr(), auxFontName, blockWidth, charCount);
                    }
                } else {
                    error(errSyntaxError, -1, "HorizontalTextLayouter, couldn't find a font for character U+{0:04uX}", uChar);
                    newFontNeeded = false;
                    i += isUnicode ? 2 : 1;
                }
            }
            // Now layout the rest of the text with the original font
            if (!availableWidth || *availableWidth > 0) {
                Annot::layoutText(text, &outputText, &i, *font, &blockWidth, availableWidth ? *availableWidth : 0.0, &charCount, false, &newFontNeeded);
                if (availableWidth) {
                    *availableWidth -= blockWidth;
                }
                // layoutText will always at least layout one character even if it doesn't fit in
                // the given space which makes sense (except in the case of switching fonts, so we control if we ran out of space here manually)
                if (!availableWidth || *availableWidth > 0) {
                    data.emplace_back(outputText.toStr(), std::string(), blockWidth, charCount);
                } else {
                    i -= isUnicode ? 2 : 1;
                }
            }
        }
        consumedText = i;
    }

    HorizontalTextLayouter(const HorizontalTextLayouter &) = delete;
    HorizontalTextLayouter &operator=(const HorizontalTextLayouter &) = delete;

    double totalWidth() const
    {
        double totalWidth = 0;
        for (const Data &d : data) {
            totalWidth += d.width;
        }
        return totalWidth;
    }

    int totalCharCount() const
    {
        int total = 0;
        for (const Data &d : data) {
            total += d.charCount;
        }
        return total;
    }

    struct Data
    {
        Data(std::string t, std::string fName, double w, int cc) : text(std::move(t)), fontName(std::move(fName)), width(w), charCount(cc) { }

        const std::string text;
        const std::string fontName;
        const double width;
        const int charCount;
    };

    std::vector<Data> data;
    int consumedText;
};

double Annot::calculateFontSize(const Form *form, const GfxFont *font, const GooString *text, double wMax, double hMax, const bool forceZapfDingbats)
{
    const bool isUnicode = hasUnicodeByteOrderMark(text->toStr());
    double fontSize;

    for (fontSize = 20; fontSize > 1; --fontSize) {
        const double availableWidthInFontSize = wMax / fontSize;
        double y = hMax - 3;
        size_t i = 0;
        while (i < text->size()) {
            GooString lineText(text->toStr().substr(i));
            if (!hasUnicodeByteOrderMark(lineText.toStr()) && isUnicode) {
                prependUnicodeByteOrderMark(lineText.toNonConstStr());
            }
            const HorizontalTextLayouter textLayouter(&lineText, form, font, availableWidthInFontSize, forceZapfDingbats);
            y -= fontSize;
            if (i == 0) {
                i += textLayouter.consumedText;
            } else {
                i += textLayouter.consumedText - (isUnicode ? 2 : 0);
            }
        }
        // approximate the descender for the last line
        if (y >= 0.33 * fontSize) {
            break;
        }
    }
    return fontSize;
}

struct DrawMultiLineTextResult
{
    std::string text;
    int nLines = 0;
};