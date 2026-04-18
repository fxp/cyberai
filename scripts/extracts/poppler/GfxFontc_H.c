// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 8/20



        //----- 8-bit font substitution
        if (flags & fontFixedWidth) {
            substIdx = 0;
        } else if (flags & fontSerif) {
            substIdx = 8;
        } else {
            substIdx = 4;
        }
        if (isBold()) {
            substIdx += 2;
        }
        if (isItalic()) {
            substIdx += 1;
        }
        const std::string substName = base14SubstFonts[substIdx];
        if (ps) {
            error(errSyntaxWarning, -1, "Substituting font '{0:s}' for '{1:s}'", base14SubstFonts[substIdx], name ? name->c_str() : "null");
            GfxFontLoc fontLoc;
            fontLoc.locType = gfxFontLocResident;
            fontLoc.fontType = fontType1;
            fontLoc.path = substName;
            fontLoc.substIdx = substIdx;
            return fontLoc;
        }
        path = globalParams->findFontFile(substName);
        if (path) {
            if (std::optional<GfxFontLoc> fontLoc = getExternalFont(*path, false)) {
                error(errSyntaxWarning, -1, "Substituting font '{0:s}' for '{1:s}'", base14SubstFonts[substIdx], name ? name->c_str() : "");
                name = base14SubstFonts[substIdx];
                fontLoc->substIdx = substIdx;
                return fontLoc;
            }
        }

        // failed to find a substitute font
        return std::nullopt;
    }

    // failed to find a substitute font
    return std::nullopt;
}

std::optional<GfxFontLoc> GfxFont::getExternalFont(const std::string &path, bool cid)
{
    FoFiIdentifierType fft;
    GfxFontType fontType;

    fft = FoFiIdentifier::identifyFile(path.c_str());
    switch (fft) {
    case fofiIdType1PFA:
    case fofiIdType1PFB:
        fontType = fontType1;
        break;
    case fofiIdCFF8Bit:
        fontType = fontType1C;
        break;
    case fofiIdCFFCID:
        fontType = fontCIDType0C;
        break;
    case fofiIdTrueType:
    case fofiIdTrueTypeCollection:
        fontType = cid ? fontCIDType2 : fontTrueType;
        break;
    case fofiIdOpenTypeCFF8Bit:
        fontType = fontType1COT;
        break;
    case fofiIdOpenTypeCFFCID:
        fontType = fontCIDType0COT;
        break;
    case fofiIdUnknown:
    case fofiIdError:
    default:
        fontType = fontUnknownType;
        break;
    }
    if (fontType == fontUnknownType || (cid ? (fontType < fontCIDType0) : (fontType >= fontCIDType0))) {
        return std::nullopt;
    }
    GfxFontLoc fontLoc;
    fontLoc.locType = gfxFontLocExternal;
    fontLoc.fontType = fontType;
    fontLoc.path = path;
    return fontLoc;
}

std::optional<std::vector<unsigned char>> GfxFont::readEmbFontFile(XRef *xref)
{
    Stream *str;

    Object obj1(embFontID);
    Object obj2 = obj1.fetch(xref);
    if (!obj2.isStream()) {
        error(errSyntaxError, -1, "Embedded font file is not a stream");
        embFontID = Ref::INVALID();
        return {};
    }
    str = obj2.getStream();

    std::vector<unsigned char> buf = str->toUnsignedChars();
    str->close();

    return buf;
}

struct AlternateNameMap
{
    const char *name;
    const char *alt;
};

static const AlternateNameMap alternateNameMap[] = { { .name = "fi", .alt = "f_i" },    { .name = "fl", .alt = "f_l" },    { .name = "ff", .alt = "f_f" },
                                                     { .name = "ffi", .alt = "f_f_i" }, { .name = "ffl", .alt = "f_f_l" }, { .name = nullptr, .alt = nullptr } };

const char *GfxFont::getAlternateName(const char *name)
{
    const AlternateNameMap *map = alternateNameMap;
    while (map->name) {
        if (strcmp(name, map->name) == 0) {
            return map->alt;
        }
        map++;
    }
    return nullptr;
}

//------------------------------------------------------------------------
// Gfx8BitFont
//------------------------------------------------------------------------

// Parse character names of the form 'Axx', 'xx', 'Ann', 'ABnn', or
// 'nn', where 'A' and 'B' are any letters, 'xx' is two hex digits,
// and 'nn' is decimal digits.
static bool parseNumericName(const char *s, bool hex, unsigned int *u)
{
    char *endptr;

    // Strip leading alpha characters.
    if (hex) {
        int n = 0;

        // Get string length while ignoring junk at end.
        while (isalnum(s[n])) {
            ++n;
        }

        // Only 2 hex characters with optional leading alpha is allowed.
        if (n == 3 && isalpha(*s)) {
            ++s;
        } else if (n != 2) {
            return false;
        }
    } else {
        // Strip up to two alpha characters.
        for (int i = 0; i < 2 && isalpha(*s); ++i) {
            ++s;
        }
    }

    int v = strtol(s, &endptr, hex ? 16 : 10);

    if (endptr == s) {
        return false;
    }

    // Skip trailing junk characters.
    while (*endptr != '\0' && !isalnum(*endptr)) {
        ++endptr;
    }

    if (*endptr == '\0') {
        if (u) {
            *u = v;
        }
        return true;
    }
    return false;
}