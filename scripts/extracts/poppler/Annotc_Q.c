// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 49/79



    // parse the default appearance string
    tfPos = tmPos = -1;
    if (da) {
        size_t i = 0;
        size_t j = 0;
        while (i < da->size()) {
            while (i < da->size() && Lexer::isSpace(da->getChar(i))) {
                ++i;
            }
            if (i < da->size()) {
                for (j = i + 1; j < da->size() && !Lexer::isSpace(da->getChar(j)); ++j) {
                    ;
                }
                daToks.push_back(std::make_unique<GooString>(da, i, j - i));
                i = j;
            }
        }
        for (std::size_t k = 2; k < daToks.size(); ++k) {
            if (k >= 2 && !(daToks[k])->compare("Tf")) {
                tfPos = k - 2;
            } else if (k >= 6 && !(daToks[k])->compare("Tm")) {
                tmPos = k - 6;
            }
        }
    }

    // get the font and font size
    font = nullptr;
    fontSize = 0;
    if (tfPos >= 0) {
        tok = daToks[tfPos].get();
        if (!tok->empty() >= 1 && tok->getChar(0) == '/') {
            if (!resources || !(font = resources->lookupFont(tok->c_str() + 1).get())) {
                if (xref != nullptr && resourcesDict != nullptr) {
                    const char *fallback = determineFallbackFont(tok->toStr(), "Helvetica");
                    // The font variable sometimes points to an object that needs to be deleted
                    // and sometimes not, depending on whether the call to lookupFont above fails.
                    // When the code path right here is taken, the destructor of fontToFree
                    // (which is a std::unique_ptr) will delete the font object at the end of this method.
                    fontToFree = createAnnotDrawFont(xref, resourcesDict, tok->c_str() + 1, fallback);
                    font = fontToFree.get();
                } else {
                    error(errSyntaxError, -1, "Unknown font in field's DA string");
                }
            }
        } else {
            error(errSyntaxError, -1, "Invalid font name in 'Tf' operator in field's DA string");
        }
        tok = daToks[tfPos + 1].get();
        fontSize = gatof(tok->c_str());
    } else {
        error(errSyntaxError, -1, "Missing 'Tf' operator in field's DA string");
    }
    if (!font) {
        return false;
    }

    // get the border width
    borderWidth = border ? border->getWidth() : 0;

    // compute font autosize
    if (fontSize == 0) {
        double wMax = 0;
        for (int i = 0; i < fieldChoice->getNumChoices(); ++i) {
            size_t j = 0;
            if (fieldChoice->getChoice(i) == nullptr) {
                error(errSyntaxError, -1, "Invalid annotation listbox");
                return false;
            }
            double w;
            Annot::layoutText(fieldChoice->getChoice(i), &convertedText, &j, *font, &w, 0.0, nullptr, false);
            if (w > wMax) {
                wMax = w;
            }
        }
        fontSize = rect->y2 - rect->y1 - 2 * borderWidth;
        const double fontSize2 = (rect->x2 - rect->x1 - 4 - 2 * borderWidth) / wMax;
        if (fontSize2 < fontSize) {
            fontSize = fontSize2;
        }
        fontSize = floor(fontSize);
        if (tfPos >= 0) {
            tok = daToks[tfPos + 1].get();
            tok->clear();
            tok->appendf("{0:.2f}", fontSize);
        }
    }
    // draw the text
    y = rect->y2 - rect->y1 - 1.1 * fontSize;
    for (int i = fieldChoice->getTopIndex(); i < fieldChoice->getNumChoices(); ++i) {
        // setup
        appearBuf->append("q\n");

        // draw the background if selected
        if (fieldChoice->isSelected(i)) {
            appearBuf->append("0 g f\n");
            appearBuf->appendf("{0:.2f} {1:.2f} {2:.2f} {3:.2f} re f\n", borderWidth, y - 0.2 * fontSize, rect->x2 - rect->x1 - 2 * borderWidth, 1.1 * fontSize);
        }

        // setup
        appearBuf->append("BT\n");

        // compute text width and start position
        size_t j = 0;
        double w;
        Annot::layoutText(fieldChoice->getChoice(i), &convertedText, &j, *font, &w, 0.0, nullptr, false);
        w *= fontSize;
        switch (quadding) {
        case VariableTextQuadding::leftJustified:
        default:
            x = borderWidth + 2;
            break;
        case VariableTextQuadding::centered:
            x = (rect->x2 - rect->x1 - w) / 2;
            break;
        case VariableTextQuadding::rightJustified:
            x = rect->x2 - rect->x1 - borderWidth - 2 - w;
            break;
        }

        // set the font matrix
        if (tmPos >= 0) {
            tok = daToks[tmPos + 4].get();
            tok->clear();
            tok->appendf("{0:.2f}", x);
            tok = daToks[tmPos + 5].get();
            tok->clear();
            tok->appendf("{0:.2f}", y);
        }

        // write the DA string
        for (const std::unique_ptr<GooString> &daTok : daToks) {
            appearBuf->append(daTok->toStr());
            appearBuf->push_back(' ');
        }