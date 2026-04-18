// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 9/20



// Returns true if the font has character names like xx or Axx which
// should be parsed for hex or decimal values.
static bool testForNumericNames(const Dict &fontDict, bool hex)
{
    bool numeric = true;

    Object enc = fontDict.lookup("Encoding");
    if (!enc.isDict()) {
        return false;
    }

    Object diff = enc.dictLookup("Differences");
    if (!diff.isArray()) {
        return false;
    }

    for (int i = 0; i < diff.arrayGetLength() && numeric; ++i) {
        Object obj = diff.arrayGet(i);
        if (obj.isInt()) {
            // All sequences must start between character codes 0 and 5.
            if (obj.getInt() > 5) {
                numeric = false;
            }
        } else if (obj.isName()) {
            // All character names must successfully parse.
            if (!parseNumericName(obj.getName(), hex, nullptr)) {
                numeric = false;
            }
        } else {
            numeric = false;
        }
    }

    return numeric;
}

Gfx8BitFont::Gfx8BitFont(XRef *xref, const char *tagA, Ref idA, std::optional<std::string> &&nameA, GfxFontType typeA, Ref embFontIDA, const Dict &fontDict) : GfxFont(tagA, idA, std::move(nameA), typeA, embFontIDA)
{
    const BuiltinFont *builtinFont;
    bool baseEncFromFontFile;
    int len;
    char *charName;
    bool missing, hex;
    Unicode toUnicode[256];
    Unicode uBuf[8];
    int firstChar, lastChar;
    unsigned short w;
    Object obj1;
    int n, a, b;

    // do font name substitution for various aliases of the Base 14 font
    // names
    base14 = nullptr;
    if (name) {
        std::string name2 = *name;
        size_t i = 0;
        while (i < name2.size()) {
            if (name2[i] == ' ') {
                name2.erase(i, 1);
            } else {
                ++i;
            }
        }
        a = 0;
        b = sizeof(base14FontMap) / sizeof(Base14FontMapEntry);
        // invariant: base14FontMap[a].altName <= name2 < base14FontMap[b].altName
        while (b - a > 1) {
            const int m = (a + b) / 2;
            if (name2.compare(base14FontMap[m].altName) >= 0) {
                a = m;
            } else {
                b = m;
            }
        }
        if (name2 == base14FontMap[a].altName) {
            base14 = &base14FontMap[a];
        }
    }

    // is it a built-in font?
    builtinFont = nullptr;
    if (base14) {
        for (const BuiltinFont &bf : builtinFonts) {
            if (!strcmp(base14->base14Name, bf.name)) {
                builtinFont = &bf;
                break;
            }
        }
    }

    // default ascent/descent values
    if (builtinFont) {
        ascent = 0.001 * builtinFont->ascent;
        descent = 0.001 * builtinFont->descent;
        fontBBox[0] = 0.001 * builtinFont->bbox[0];
        fontBBox[1] = 0.001 * builtinFont->bbox[1];
        fontBBox[2] = 0.001 * builtinFont->bbox[2];
        fontBBox[3] = 0.001 * builtinFont->bbox[3];
    } else {
        ascent = 0.95;
        descent = -0.35;
        fontBBox[0] = fontBBox[1] = fontBBox[2] = fontBBox[3] = 0;
    }

    // get info from font descriptor
    readFontDescriptor(fontDict);

    // for non-embedded fonts, don't trust the ascent/descent/bbox
    // values from the font descriptor
    if (builtinFont && embFontID == Ref::INVALID()) {
        ascent = 0.001 * builtinFont->ascent;
        descent = 0.001 * builtinFont->descent;
        fontBBox[0] = 0.001 * builtinFont->bbox[0];
        fontBBox[1] = 0.001 * builtinFont->bbox[1];
        fontBBox[2] = 0.001 * builtinFont->bbox[2];
        fontBBox[3] = 0.001 * builtinFont->bbox[3];
    }

    // get font matrix
    fontMat[0] = fontMat[3] = 1;
    fontMat[1] = fontMat[2] = fontMat[4] = fontMat[5] = 0;
    obj1 = fontDict.lookup("FontMatrix");
    if (obj1.isArray()) {
        for (int i = 0; i < 6 && i < obj1.arrayGetLength(); ++i) {
            Object obj2 = obj1.arrayGet(i);
            if (obj2.isNum()) {
                fontMat[i] = obj2.getNum();
            }
        }
    }

    // get Type 3 bounding box, font definition, and resources
    if (type == fontType3) {
        obj1 = fontDict.lookup("FontBBox");
        if (obj1.isArray()) {
            for (int i = 0; i < 4 && i < obj1.arrayGetLength(); ++i) {
                Object obj2 = obj1.arrayGet(i);
                if (obj2.isNum()) {
                    fontBBox[i] = obj2.getNum();
                }
            }
        }
        charProcs = fontDict.lookup("CharProcs");
        if (!charProcs.isDict()) {
            error(errSyntaxError, -1, "Missing or invalid CharProcs dictionary in Type 3 font");
            charProcs.setToNull();
        }
        resources = fontDict.lookup("Resources");
        if (!resources.isDict()) {
            resources.setToNull();
        }
    }

    //----- build the font encoding -----