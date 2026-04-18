// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 23/24



std::vector<Form::AddFontResult> Form::ensureFontsForAllCharacters(const GooString *unicodeText, const std::string &pdfFontNameToEmulate, GfxResources *fieldResources)
{
    GfxResources *resources = fieldResources ? fieldResources : defaultResources;
    std::shared_ptr<GfxFont> f;
    if (!resources) {
        // There's no resources, so create one with the needed font name
        addFontToDefaultResources(pdfFontNameToEmulate, "", /*forceName*/ true);
        resources = defaultResources;
    }
    f = resources->lookupFont(pdfFontNameToEmulate.c_str());
    const CharCodeToUnicode *ccToUnicode = f ? f->getToUnicode() : nullptr;
    if (!ccToUnicode) {
        error(errInternal, -1, "Form::ensureFontsForAllCharacters: No ccToUnicode, this should not happen");
        return {}; // will never happen with current code
    }

    std::vector<AddFontResult> newFonts;

    // If the text has some characters that are not available in the font, try adding a font for those
    std::unordered_set<Unicode> seen;
    for (size_t i = 2; i < unicodeText->size(); i += 2) {
        Unicode uChar = (unsigned char)(unicodeText->getChar(i)) << 8;
        uChar += (unsigned char)(unicodeText->getChar(i + 1));

        if (uChar < 128 && !std::isprint(static_cast<unsigned char>(uChar))) {
            continue;
        }
        const auto [_, inserted] = seen.insert(uChar);
        if (!inserted) {
            continue;
        }

        CharCode c;
        bool addFont = false;
        if (ccToUnicode->mapToCharCode(&uChar, &c, 1)) {
            if (f->isCIDFont()) {
                const auto *cidFont = static_cast<const GfxCIDFont *>(f.get());
                if (c < cidFont->getCIDToGIDLen() && c != 0 && c != '\r' && c != '\n') {
                    const int glyph = cidFont->getCIDToGID()[c];
                    if (glyph == 0) {
                        addFont = true;
                    }
                }
            }
        } else {
            addFont = true;
        }

        if (addFont) {
            Form::AddFontResult res = doGetAddFontToDefaultResources(uChar, *f);
            if (res.ref != Ref::INVALID()) {
                newFonts.emplace_back(res);
            }
        }
    }

    return newFonts;
}

Form::AddFontResult Form::doGetAddFontToDefaultResources(Unicode uChar, const GfxFont &fontToEmulate)
{
    const UCharFontSearchResult res = globalParams->findSystemFontFileForUChar(uChar, fontToEmulate);

    std::string pdfFontName = findFontInDefaultResources(res.family, res.style);
    if (pdfFontName.empty()) {
        return addFontToDefaultResources(res.filepath, res.faceIndex, res.family, res.style,
                                         true /*This is called when a string contains a uChar that is not present in the font for that string so we find a another font to display it, thus it's always a 'substitute' font*/,
                                         false /*forceName*/);
    }
    return { .fontName = pdfFontName, .ref = Ref::INVALID() };
}

void Form::postWidgetsLoad()
{
    // We create the widget annotations associated to
    // every form widget here, because the AnnotWidget constructor
    // needs the form object that gets from the catalog. When constructing
    // a FormWidget the Catalog is still creating the form object
    for (auto &rootField : rootFields) {
        rootField->fillChildrenSiblingsID();
        rootField->createWidgetAnnotations();
    }
}

FormWidget *Form::findWidgetByRef(Ref aref)
{
    for (auto &rootField : rootFields) {
        FormWidget *result = rootField->findWidgetByRef(aref);
        if (result) {
            return result;
        }
    }
    return nullptr;
}

FormField *Form::findFieldByRef(Ref aref) const
{
    for (const auto &rootField : rootFields) {
        FormField *result = rootField->findFieldByRef(aref);
        if (result) {
            return result;
        }
    }
    return nullptr;
}

FormField *Form::findFieldByFullyQualifiedName(const std::string &name) const
{
    for (const auto &rootField : rootFields) {
        FormField *result = rootField->findFieldByFullyQualifiedName(name);
        if (result) {
            return result;
        }
    }
    return nullptr;
}

FormField *Form::findFieldByFullyQualifiedNameOrRef(const std::string &field) const
{
    Ref fieldRef;

    if (field.ends_with(" R") && sscanf(field.c_str(), "%d %d R", &fieldRef.num, &fieldRef.gen) == 2) {
        return findFieldByRef(fieldRef);
    }
    return findFieldByFullyQualifiedName(field);
}

void Form::reset(const std::vector<std::string> &fields, bool excludeFields)
{
    const bool resetAllFields = fields.empty();

    if (resetAllFields) {
        for (auto &rootField : rootFields) {
            rootField->reset(std::vector<std::string>());
        }
    } else {
        if (!excludeFields) {
            FormField *foundField;
            for (const std::string &field : fields) {
                Ref fieldRef;