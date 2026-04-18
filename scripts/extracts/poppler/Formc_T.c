// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 20/24



    const Dict *fontDict = fontDictObj.getDict();
    for (int i = 0; i < fontDict->getLength(); ++i) {
        const char *key = fontDict->getKey(i);
        if (std::string_view(key).starts_with(kOurDictFontNamePrefix)) {
            const Object fontObj = fontDict->getVal(i);
            if (fontObj.isDict() && fontObj.dictIs("Font")) {
                const Object fontBaseFontObj = fontObj.dictLookup("BaseFont");
                if (fontBaseFontObj.isName(fontFamilyAndStyle)) {
                    return key;
                }
            }
        }
    }

    return {};
}

Form::AddFontResult Form::addFontToDefaultResources(const std::string &fontFamily, const std::string &fontStyle, bool forceName)
{
    FamilyStyleFontSearchResult findFontRes = globalParams->findSystemFontFileForFamilyAndStyle(fontFamily, fontStyle);
    std::vector<std::string> filesToIgnore;
    while (!findFontRes.filepath.empty()) {
        Form::AddFontResult addFontRes = addFontToDefaultResources(findFontRes.filepath, findFontRes.faceIndex, fontFamily, fontStyle, findFontRes.substituted, forceName);
        if (!addFontRes.fontName.empty()) {
            return addFontRes;
        }
        filesToIgnore.emplace_back(findFontRes.filepath);
        findFontRes = globalParams->findSystemFontFileForFamilyAndStyle(fontFamily, fontStyle, filesToIgnore);
    }
    return {};
}

Form::AddFontResult Form::addFontToDefaultResources(const std::string &filepath, int faceIndex, const std::string &fontFamily, const std::string &fontStyle, bool fontSubstitutedIn, bool forceName)
{
    if (!filepath.ends_with(".ttf") && !filepath.ends_with(".ttc") && !filepath.ends_with(".otf")) {
        error(errIO, -1, "We only support embedding ttf/ttc/otf fonts for now. The font file for {0:s} {1:s} was {2:s}", fontFamily.c_str(), fontStyle.c_str(), filepath.c_str());
        return {};
    }

    const FoFiIdentifierType fontFoFiType = FoFiIdentifier::identifyFile(filepath.c_str());
    if (fontFoFiType != fofiIdTrueType && fontFoFiType != fofiIdTrueTypeCollection && fontFoFiType != fofiIdOpenTypeCFF8Bit && fontFoFiType != fofiIdOpenTypeCFFCID) {
        error(errIO, -1, "We only support embedding ttf/ttc/otf fonts for now. The font file for {0:s} {1:s} was {2:s} of type {3:d}", fontFamily.c_str(), fontStyle.c_str(), filepath.c_str(), fontFoFiType);
        return {};
    }

    const std::string fontFamilyAndStyle = fontStyle.empty() ? fontFamily : fontFamily + " " + fontStyle;

    if (forceName && defaultResources && defaultResources->lookupFont(fontFamilyAndStyle.c_str())) {
        error(errInternal, -1, "Form::addFontToDefaultResources: Asked to forceName but font name exists {0:s}", fontFamilyAndStyle.c_str());
        return {};
    }

    // If caller asked for one of the base14 fonts and got something named differently
    // we are only having something close anyways, so don't embed the font program (and
    // when we don't embed the font program, also don't embed widths and the CIDToGIDMap)
    bool embedActualFont = !(fontSubstitutedIn && GfxFont::isBase14Font(fontFamily, fontStyle));

    XRef *xref = doc->getXRef();
    Object fontDict(std::make_unique<Dict>(xref));
    fontDict.dictSet("Type", Object(objName, "Font"));
    fontDict.dictSet("Subtype", Object(objName, (embedActualFont ? "Type0" : "Type1")));
    fontDict.dictSet("BaseFont", Object(objName, fontFamilyAndStyle.c_str()));

    if (embedActualFont) {
        fontDict.dictSet("Encoding", Object(objName, "Identity-H"));
        std::unique_ptr<Array> descendantFonts = std::make_unique<Array>(xref);

        const bool isTrueType = (fontFoFiType == fofiIdTrueType || fontFoFiType == fofiIdTrueTypeCollection);
        std::unique_ptr<Dict> descendantFont = std::make_unique<Dict>(xref);
        descendantFont->set("Type", Object(objName, "Font"));
        descendantFont->set("Subtype", Object(objName, isTrueType ? "CIDFontType2" : "CIDFontType0"));
        descendantFont->set("BaseFont", Object(objName, fontFamilyAndStyle.c_str()));

        {
            // We only support fonts with identity cmaps for now
            auto cidSystemInfo = std::make_unique<Dict>(xref);
            cidSystemInfo->set("Registry", Object(std::make_unique<GooString>("Adobe")));
            cidSystemInfo->set("Ordering", Object(std::make_unique<GooString>("Identity")));
            cidSystemInfo->set("Supplement", Object(0));
            descendantFont->set("CIDSystemInfo", Object(std::move(cidSystemInfo)));
        }

        FT_Library freetypeLib;
        if (FT_Init_FreeType(&freetypeLib)) {
            error(errIO, -1, "FT_Init_FreeType failed");
            return {};
        }
        const std::unique_ptr<FT_Library, void (*)(FT_Library *)> freetypeLibDeleter(&freetypeLib, [](FT_Library *l) { FT_Done_FreeType(*l); });