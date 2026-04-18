// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 4/20



    // get base font name
    Object obj1 = fontDict.lookup("BaseFont");
    if (obj1.isName()) {
        name = obj1.getName();
    }

    // There is no BaseFont in Type 3 fonts, try fontDescriptor.FontName
    if (!name) {
        Object fontDesc = fontDict.lookup("FontDescriptor");
        if (fontDesc.isDict()) {
            Object obj2 = fontDesc.dictLookup("FontName");
            if (obj2.isName()) {
                name = obj2.getName();
            }
        }
    }

    // As a last resort try the Name key
    if (!name) {
        Object obj2 = fontDict.lookup("Name");
        if (obj2.isName()) {
            name = obj2.getName();
        }
    }

    // get embedded font ID and font type
    const GfxFontType typeA = getFontType(xref, fontDict, &embFontIDA);

    // create the font object
    if (typeA < fontCIDType0) {
        return std::make_unique<Gfx8BitFont>(xref, tagA, idA, std::move(name), typeA, embFontIDA, fontDict);
    }
    return std::make_unique<GfxCIDFont>(tagA, idA, std::move(name), typeA, embFontIDA, fontDict);
}

GfxFont::GfxFont(const char *tagA, Ref idA, std::optional<std::string> &&nameA, GfxFontType typeA, Ref embFontIDA) : tag(tagA), id(idA), name(std::move(nameA)), type(typeA)
{
    ok = false;
    embFontID = embFontIDA;
    embFontName = nullptr;
    family = nullptr;
    stretch = StretchNotDefined;
    weight = WeightNotDefined;
    hasToUnicode = false;
}

GfxFont::~GfxFont() = default;

bool GfxFont::isSubset() const
{
    if (name) {
        unsigned int i;
        for (i = 0; i < name->size(); ++i) {
            if ((*name)[i] < 'A' || (*name)[i] > 'Z') {
                break;
            }
        }
        return i == 6 && name->size() > 7 && (*name)[6] == '+';
    }
    return false;
}

std::string GfxFont::getNameWithoutSubsetTag() const
{
    if (!name) {
        return {};
    }

    if (!isSubset()) {
        return *name;
    }

    return name->substr(7);
}

// This function extracts three pieces of information:
// 1. the "expected" font type, i.e., the font type implied by
//    Font.Subtype, DescendantFont.Subtype, and
//    FontDescriptor.FontFile3.Subtype
// 2. the embedded font object ID
// 3. the actual font type - determined by examining the embedded font
//    if there is one, otherwise equal to the expected font type
// If the expected and actual font types don't match, a warning
// message is printed.  The expected font type is not used for
// anything else.
GfxFontType GfxFont::getFontType(XRef *xref, const Dict &fontDict, Ref *embID)
{
    GfxFontType t, expectedType;
    FoFiIdentifierType fft;
    bool isType0, err;

    t = fontUnknownType;
    *embID = Ref::INVALID();
    err = false;

    Object subtype = fontDict.lookup("Subtype");
    expectedType = fontUnknownType;
    isType0 = false;
    if (subtype.isName("Type1") || subtype.isName("MMType1")) {
        expectedType = fontType1;
    } else if (subtype.isName("Type1C")) {
        expectedType = fontType1C;
    } else if (subtype.isName("Type3")) {
        expectedType = fontType3;
    } else if (subtype.isName("TrueType")) {
        expectedType = fontTrueType;
    } else if (subtype.isName("Type0")) {
        isType0 = true;
    } else {
        error(errSyntaxWarning, -1, "Unknown font type: '{0:s}'", subtype.isName() ? subtype.getName() : "???");
    }

    const Dict *fontDict2 = &fontDict;
    Object obj1 = fontDict.lookup("DescendantFonts");
    Object obj2; // Do not move to inside the if
                 // we need it around so that fontDict2 remains valid
    if (obj1.isArray()) {
        if (obj1.arrayGetLength() == 0) {
            error(errSyntaxWarning, -1, "Empty DescendantFonts array in font");
        } else {
            obj2 = obj1.arrayGet(0);
            if (obj2.isDict()) {
                if (!isType0) {
                    error(errSyntaxWarning, -1, "Non-CID font with DescendantFonts array");
                }
                fontDict2 = obj2.getDict();
                subtype = fontDict2->lookup("Subtype");
                if (subtype.isName("CIDFontType0")) {
                    if (isType0) {
                        expectedType = fontCIDType0;
                    }
                } else if (subtype.isName("CIDFontType2")) {
                    if (isType0) {
                        expectedType = fontCIDType2;
                    }
                }
            }
        }
    }