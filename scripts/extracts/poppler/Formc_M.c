// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 13/24



        // append the unicode marker <FE FF> if needed
        if (!hasUnicodeByteOrderMark(content->toStr())) {
            prependUnicodeByteOrderMark(content->toNonConstStr());
        }
        Form *form = doc->getCatalog()->getForm();
        if (form) {
            DefaultAppearance da(defaultAppearance.get());
            const std::string &fontName = da.getFontName();
            if (!fontName.empty()) {
                // Use the field resource dictionary if it exists
                Object fieldResourcesDictObj = obj.dictLookup("DR");
                if (fieldResourcesDictObj.isDict()) {
                    GfxResources fieldResources(doc->getXRef(), fieldResourcesDictObj.getDict(), form->getDefaultResources());
                    const std::vector<Form::AddFontResult> newFonts = form->ensureFontsForAllCharacters(content.get(), fontName, &fieldResources);
                    // If we added new fonts to the Form object default resuources we also need to add them (we only add the ref so this is cheap)
                    // to the field DR dictionary
                    if (!newFonts.empty()) {
                        for (const Form::AddFontResult &afr : newFonts) {
                            fieldResourcesDictObj.dictLookup("Font").dictAdd(afr.fontName, Object(afr.ref));
                            // This is not fully correct, it changes the entire font to the last added font
                            // but it is much better than not doing anything, because we know that one of
                            // the fonts have characters we need, so there is a bit of hope involved here
                            // It is likely that we only have added one font, and it is likely that it is
                            // a non-subset version of a subset or a reduced type1 font or similar.
                            da.setFontName(afr.fontName);
                        }
                        setDefaultAppearance(da.toAppearanceString());
                    }
                } else {
                    form->ensureFontsForAllCharacters(content.get(), fontName);
                }
            } else {
                // This is wrong, there has to be a Tf in DA
            }
        }
    }

    obj.getDict()->set("V", Object(content ? content->copy() : std::make_unique<GooString>("")));
    xref->setModifiedObject(&obj, ref);
    updateChildrenAppearance();
}

void FormFieldText::setAppearanceContent(std::unique_ptr<GooString> new_content)
{
    internalContent.reset();

    if (new_content) {
        internalContent = std::move(new_content);
    }
    updateChildrenAppearance();
}

FormFieldText::~FormFieldText() = default;

void FormFieldText::reset(const std::vector<std::string> &excludedFields)
{
    if (!isAmongExcludedFields(excludedFields)) {
        setContent(defaultContent ? defaultContent->copy() : nullptr);
        if (defaultContent == nullptr) {
            obj.getDict()->remove("V");
        }
    }

    resetChildren(excludedFields);
}

double FormFieldText::getTextFontSize()
{
    std::vector<std::string> daToks;
    const std::optional<size_t> idx = parseDA(&daToks);
    double fontSize = -1;
    if (idx) {
        char *p = nullptr;
        fontSize = strtod(daToks[*idx].c_str(), &p);
        if (!p || *p) {
            fontSize = -1;
        }
    }
    return fontSize;
}

void FormFieldText::setTextFontSize(int fontSize)
{
    if (fontSize > 0 && obj.isDict()) {
        std::vector<std::string> daToks;
        const std::optional<size_t> idx = parseDA(&daToks);
        if (!idx) {
            error(errSyntaxError, -1, "FormFieldText:: invalid DA object");
            return;
        }
        defaultAppearance = std::make_unique<GooString>();
        for (std::size_t i = 0; i < daToks.size(); ++i) {
            if (i > 0) {
                defaultAppearance->push_back(' ');
            }
            if (i == idx) {
                defaultAppearance->appendf("{0:d}", fontSize);
            } else {
                defaultAppearance->append(daToks[i]);
            }
        }
        obj.dictSet("DA", Object(defaultAppearance->copy()));
        xref->setModifiedObject(&obj, ref);
        updateChildrenAppearance();
    }
}

std::optional<size_t> FormFieldText::tokenizeDA(const std::string &da, std::vector<std::string> *daToks, const char *searchTok)
{
    std::optional<size_t> idx;
    if (daToks) {
        size_t i = 0;
        size_t j = 0;
        while (i < da.size()) {
            while (i < da.size() && Lexer::isSpace(da[i])) {
                ++i;
            }
            if (i < da.size()) {
                for (j = i + 1; j < da.size() && !Lexer::isSpace(da[j]); ++j) { }
                std::string tok(da, i, j - i);
                if (searchTok && tok == searchTok) {
                    idx = daToks->size();
                }
                daToks->emplace_back(std::move(tok));
                i = j;
            }
        }
    }
    return idx;
}