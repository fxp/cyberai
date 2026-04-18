// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 15/20



        // map the char codes through the cmap, possibly with an offset of
        // 0xf000
    } else {
        for (int i = 0; i < 256; ++i) {
            if (!(map[i] = ff->mapCodeToGID(cmap, i))) {
                map[i] = ff->mapCodeToGID(cmap, 0xf000 + i);
            }
        }
    }

    // try the TrueType 'post' table to handle any unmapped characters
    for (int i = 0; i < 256; ++i) {
        if (map[i] <= 0 && (charName = enc[i])) {
            map[i] = ff->mapNameToGID(charName);
        }
    }

    return map;
}

Dict *Gfx8BitFont::getCharProcs()
{
    return charProcs.isDict() ? charProcs.getDict() : nullptr;
}

Object Gfx8BitFont::getCharProc(int code)
{
    if (enc[code] && charProcs.isDict()) {
        return charProcs.dictLookup(enc[code]);
    }
    return Object::null();
}

Object Gfx8BitFont::getCharProcNF(int code)
{
    if (enc[code] && charProcs.isDict()) {
        return charProcs.dictLookupNF(enc[code]).copy();
    }
    return Object::null();
}

Dict *Gfx8BitFont::getResources()
{
    return resources.isDict() ? resources.getDict() : nullptr;
}

//------------------------------------------------------------------------
// GfxCIDFont
//------------------------------------------------------------------------

struct cmpWidthExcepFunctor
{
    bool operator()(const GfxFontCIDWidthExcep w1, const GfxFontCIDWidthExcep w2) { return w1.first < w2.first; }
};

struct cmpWidthExcepVFunctor
{
    bool operator()(const GfxFontCIDWidthExcepV &w1, const GfxFontCIDWidthExcepV &w2) { return w1.first < w2.first; }
};

GfxCIDFont::GfxCIDFont(const char *tagA, Ref idA, std::optional<std::string> &&nameA, GfxFontType typeA, Ref embFontIDA, const Dict &fontDict) : GfxFont(tagA, idA, std::move(nameA), typeA, embFontIDA)
{
    Dict *desFontDict;
    Object desFontDictObj;
    Object obj1, obj2, obj3, obj4, obj5, obj6;
    int c1, c2;

    ascent = 0.95;
    descent = -0.35;
    fontBBox[0] = fontBBox[1] = fontBBox[2] = fontBBox[3] = 0;
    collection = nullptr;
    ctuUsesCharCode = true;
    widths.defWidth = 1.0;
    widths.defHeight = -1.0;
    widths.defVY = 0.880;

    // get the descendant font
    obj1 = fontDict.lookup("DescendantFonts");
    if (!obj1.isArrayOfLengthAtLeast(1)) {
        error(errSyntaxError, -1, "Missing or empty DescendantFonts entry in Type 0 font");
        return;
    }
    desFontDictObj = obj1.arrayGet(0);
    if (!desFontDictObj.isDict()) {
        error(errSyntaxError, -1, "Bad descendant font in Type 0 font");
        return;
    }
    desFontDict = desFontDictObj.getDict();

    // get info from font descriptor
    readFontDescriptor(*desFontDict);

    //----- encoding info -----

    // char collection
    obj1 = desFontDict->lookup("CIDSystemInfo");
    if (obj1.isDict()) {
        obj2 = obj1.dictLookup("Registry");
        obj3 = obj1.dictLookup("Ordering");
        if (!obj2.isString() || !obj3.isString()) {
            error(errSyntaxError, -1, "Invalid CIDSystemInfo dictionary in Type 0 descendant font");
            error(errSyntaxError, -1, "Assuming Adobe-Identity for character collection");
            obj2 = Object(std::make_unique<GooString>("Adobe"));
            obj3 = Object(std::make_unique<GooString>("Identity"));
        }
        collection = obj2.takeString();
        collection->push_back('-');
        collection->append(obj3.getString());
    } else {
        error(errSyntaxError, -1, "Missing CIDSystemInfo dictionary in Type 0 descendant font");
        error(errSyntaxError, -1, "Assuming Adobe-Identity for character collection");
        collection = std::make_unique<GooString>("Adobe-Identity");
    }

    // look for a ToUnicode CMap
    if (!(ctu = readToUnicodeCMap(fontDict, 16, nullptr))) {
        ctuUsesCharCode = false;