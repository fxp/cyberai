// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 48/79



                    const GfxFont *currentFont = font;
                    if (!d.fontName.empty()) {
                        appearBuf->append(" q\n");
                        appearBuf->appendf("/{0:s} {1:.2f} Tf\n", d.fontName.c_str(), fontSize);
                        currentFont = form->getDefaultResources()->lookupFont(d.fontName.c_str()).get();
                    }

                    char_dx = 0.0;
                    n = currentFont->getNextChar(s, len, &code, &uAux, &uLen, &char_dx, &char_dy, &ox, &oy);
                    char_dx *= fontSize;

                    // center each character within its cell, by advancing the text
                    // position the appropriate amount relative to the start of the
                    // previous character
                    const double combX = 0.5 * (w - char_dx);
                    appearBuf->appendf("{0:.2f} 0 Td\n", combX - xPrev + w);

                    GooString charBuf(s, n);
                    writeString(charBuf.toStr());
                    appearBuf->append(" Tj\n");

                    if (!d.fontName.empty()) {
                        appearBuf->append(" Q\n");
                    }

                    i++;
                    s += n;
                    len -= n;
                    xPrev = combX;
                }
            }

            // regular (non-comb) formatting
        } else {
            const HorizontalTextLayouter textLayouter(text, form, font, {}, forceZapfDingbats);

            const double usedWidthUnscaled = textLayouter.totalWidth();

            // compute font autosize
            if (fontSize == 0) {
                fontSize = dy - 2 * borderWidth;
                if (usedWidthUnscaled > 0) {
                    const double fontSize2 = (dx - 4 - 2 * borderWidth) / usedWidthUnscaled;
                    if (fontSize2 < fontSize) {
                        fontSize = fontSize2;
                    }
                }
                fontSize = floor(fontSize);
                daToks[tfPos + 1] = GooString().appendf("{0:.2f}", fontSize)->toStr();
            }

            // compute text start position
            const double usedWidth = usedWidthUnscaled * fontSize;
            auto calculateX = [quadding, borderWidth, dx, usedWidth] {
                switch (quadding) {
                case VariableTextQuadding::leftJustified:
                default:
                    return borderWidth + 2;
                case VariableTextQuadding::centered:
                    return (dx - usedWidth) / 2;
                case VariableTextQuadding::rightJustified:
                    return dx - borderWidth - 2 - usedWidth;
                }
            };
            const double x = calculateX();
            const double y = 0.5 * dy - 0.4 * fontSize;

            // set the font matrix
            daToks[tmPos + 4] = GooString().appendf("{0:.2f}", x)->toStr();
            daToks[tmPos + 5] = GooString().appendf("{0:.2f}", y)->toStr();

            // write the DA string
            for (const std::string &daTok : daToks) {
                appearBuf->append(daTok);
                appearBuf->push_back(' ');
            }
            // This newline is not neeed at all but it makes for easier reading
            // and our auto tests "wrongly" assume it will be there, so add it anyway
            appearBuf->append("\n");

            // write the text strings
            for (const HorizontalTextLayouter::Data &d : textLayouter.data) {
                if (!d.fontName.empty()) {
                    appearBuf->append(" q\n");
                    appearBuf->appendf("/{0:s} {1:.2f} Tf\n", d.fontName.c_str(), fontSize);
                }
                writeString(d.text);
                appearBuf->append(" Tj\n");
                if (!d.fontName.empty()) {
                    appearBuf->append(" Q\n");
                }
            }
        }
    }
    // cleanup
    appearBuf->append("ET\n");
    appearBuf->append("Q\n");
    if (flags & EmitMarkedContentDrawTextFlag) {
        appearBuf->append("EMC\n");
    }

    return true;
}

// Draw the variable text or caption for a field.
bool AnnotAppearanceBuilder::drawListBox(const FormFieldChoice *fieldChoice, const AnnotBorder *border, const PDFRectangle *rect, const GooString *da, const GfxResources *resources, VariableTextQuadding quadding, XRef *xref,
                                         Dict *resourcesDict)
{
    std::vector<std::unique_ptr<GooString>> daToks;
    GooString *tok;
    GooString convertedText;
    const GfxFont *font;
    double fontSize, borderWidth, x, y;
    int tfPos, tmPos;
    std::unique_ptr<const GfxFont> fontToFree;

    //~ if there is no MK entry, this should use the existing content stream,
    //~ and only replace the marked content portion of it
    //~ (this is only relevant for Tx fields)