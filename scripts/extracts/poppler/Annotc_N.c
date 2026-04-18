// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 46/79



        while (len > 0) {
            dx = 0.0;
            n = font.getNextChar(s, len, &c, &uAux, &uLen, &dx, &dy, &ox, &oy);

            if (n == 0) {
                break;
            }

            if (width != nullptr) {
                *width += dx;
            }
            if (charCount != nullptr) {
                *charCount += 1;
            }

            s += n;
            len -= n;
        }
    }
}

// Copy the given string to appearBuf, adding parentheses around it and
// escaping characters as appropriate.
void AnnotAppearanceBuilder::writeString(const std::string &str)
{
    appearBuf->push_back('(');

    for (const char c : str) {
        if (c == '(' || c == ')' || c == '\\') {
            appearBuf->push_back('\\');
            appearBuf->push_back(c);
        } else if (c < 0x20) {
            appearBuf->appendf("\\{0:03o}", (unsigned char)c);
        } else {
            appearBuf->push_back(c);
        }
    }

    appearBuf->push_back(')');
}

// Draw the variable text or caption for a field.
bool AnnotAppearanceBuilder::drawText(const GooString *text, const Form *form, const GooString *da, const GfxResources *resources, const AnnotBorder *border, const AnnotAppearanceCharacs *appearCharacs, const PDFRectangle *rect,
                                      const VariableTextQuadding quadding, XRef *xref, Dict *resourcesDict, const int flags, const int nCombs)
{
    const bool forceZapfDingbats = flags & ForceZapfDingbatsDrawTextFlag;

    std::vector<std::string> daToks;
    const GfxFont *font;
    double fontSize;
    int tfPos, tmPos;
    std::unique_ptr<GooString> textToFree;
    std::unique_ptr<const GfxFont> fontToFree = nullptr;

    //~ if there is no MK entry, this should use the existing content stream,
    //~ and only replace the marked content portion of it
    //~ (this is only relevant for Tx fields)

    // Checkbox fields may come without a DA entry, spec requires it
    // for all fields containing variable text but it seems Checkbox
    // fields are de-facto not considered as such - Issue #1055
    GooString daStackString;
    if (!da && forceZapfDingbats) {
        daStackString = GooString("/ZaDb 0 Tf 0 g");
        da = &daStackString;
    }

    // parse the default appearance string
    tfPos = tmPos = -1;
    if (da) {
        FormFieldText::tokenizeDA(da->toStr(), &daToks, nullptr /*searchTok*/);
        for (size_t i = 2; i < daToks.size(); ++i) {
            if (i >= 2 && daToks[i] == "Tf") {
                tfPos = i - 2;
            } else if (i >= 6 && daToks[i] == "Tm") {
                tmPos = i - 6;
            }
        }
    }

    // get the font and font size
    font = nullptr;
    fontSize = 0;
    if (tfPos >= 0) {
        std::string &tok = daToks[tfPos];
        if (forceZapfDingbats) {
            assert(xref != nullptr);
            if (tok != "/ZaDb") {
                tok = "/ZaDb";
            }
        }
        if (!tok.empty() && tok[0] == '/') {
            if (!resources || !(font = resources->lookupFont(tok.c_str() + 1).get())) {
                if (xref != nullptr && resourcesDict != nullptr) {
                    const char *fallback = determineFallbackFont(tok, forceZapfDingbats ? "ZapfDingbats" : "Helvetica");
                    // The font variable sometimes points to an object that needs to be deleted
                    // and sometimes not, depending on whether the call to lookupFont above fails.
                    // When the code path right here is taken, the destructor of fontToFree
                    // (which is a std::unique_ptr) will delete the font object at the end of this method.
                    fontToFree = createAnnotDrawFont(xref, resourcesDict, tok.c_str() + 1, fallback);
                    font = fontToFree.get();
                    if (font && forceZapfDingbats) {
                        addedDingbatsResource = true;
                    }
                } else {
                    error(errSyntaxError, -1, "Unknown font in field's DA string");
                }
            }
        } else {
            error(errSyntaxError, -1, "Invalid font name in 'Tf' operator in field's DA string");
        }
        fontSize = gatof(daToks[tfPos + 1].c_str());
    } else {
        error(errSyntaxError, -1, "Missing 'Tf' operator in field's DA string");
    }
    if (!font) {
        return false;
    }

    if (tmPos < 0) {
        // Add fake Tm to the DA tokens
        tmPos = daToks.size();
        daToks.insert(daToks.end(), { "1", "0", "0", "1", "0", "0", "Tm" });
    }

    // get the border width
    const double borderWidth = border ? border->getWidth() : 0;

    // for a password field, replace all characters with asterisks
    if (flags & TurnTextToStarsDrawTextFlag) {
        int len;
        if (hasUnicodeByteOrderMark(text->toStr())) {
            len = (text->size() - 2) / 2;
        } else {
            len = text->size();
        }