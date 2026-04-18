// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 6/20



        // get name
        obj2 = obj1.dictLookup("FontName");
        if (obj2.isName()) {
            embFontName = std::make_unique<GooString>(obj2.getName());
        }
        if (embFontName == nullptr) {
            // get name with typo
            obj2 = obj1.dictLookup("Fontname");
            if (obj2.isName()) {
                embFontName = std::make_unique<GooString>(obj2.getName());
                error(errSyntaxWarning, -1, "The file uses Fontname instead of FontName please notify the creator that the file is broken");
            }
        }

        // get family
        obj2 = obj1.dictLookup("FontFamily");
        if (obj2.isString()) {
            family = obj2.takeString();
        }

        // get stretch
        obj2 = obj1.dictLookup("FontStretch");
        if (obj2.isName()) {
            if (strcmp(obj2.getName(), "UltraCondensed") == 0) {
                stretch = UltraCondensed;
            } else if (strcmp(obj2.getName(), "ExtraCondensed") == 0) {
                stretch = ExtraCondensed;
            } else if (strcmp(obj2.getName(), "Condensed") == 0) {
                stretch = Condensed;
            } else if (strcmp(obj2.getName(), "SemiCondensed") == 0) {
                stretch = SemiCondensed;
            } else if (strcmp(obj2.getName(), "Normal") == 0) {
                stretch = Normal;
            } else if (strcmp(obj2.getName(), "SemiExpanded") == 0) {
                stretch = SemiExpanded;
            } else if (strcmp(obj2.getName(), "Expanded") == 0) {
                stretch = Expanded;
            } else if (strcmp(obj2.getName(), "ExtraExpanded") == 0) {
                stretch = ExtraExpanded;
            } else if (strcmp(obj2.getName(), "UltraExpanded") == 0) {
                stretch = UltraExpanded;
            } else {
                error(errSyntaxWarning, -1, "Invalid Font Stretch");
            }
        }

        // get weight
        obj2 = obj1.dictLookup("FontWeight");
        if (obj2.isNum()) {
            if (obj2.getNum() == 100) {
                weight = W100;
            } else if (obj2.getNum() == 200) {
                weight = W200;
            } else if (obj2.getNum() == 300) {
                weight = W300;
            } else if (obj2.getNum() == 400) {
                weight = W400;
            } else if (obj2.getNum() == 500) {
                weight = W500;
            } else if (obj2.getNum() == 600) {
                weight = W600;
            } else if (obj2.getNum() == 700) {
                weight = W700;
            } else if (obj2.getNum() == 800) {
                weight = W800;
            } else if (obj2.getNum() == 900) {
                weight = W900;
            } else {
                error(errSyntaxWarning, -1, "Invalid Font Weight");
            }
        }

        // look for MissingWidth
        obj2 = obj1.dictLookup("MissingWidth");
        if (obj2.isNum()) {
            missingWidth = obj2.getNum();
        }

        // get Ascent and Descent
        obj2 = obj1.dictLookup("Ascent");
        if (obj2.isNum()) {
            t = 0.001 * obj2.getNum();
            // some broken font descriptors specify a negative ascent
            if (t < 0) {
                t = -t;
            }
            // some broken font descriptors set ascent and descent to 0;
            // others set it to ridiculous values (e.g., 32768)
            if (t != 0 && t < 3) {
                ascent = t;
            }
        }
        obj2 = obj1.dictLookup("Descent");
        if (obj2.isNum()) {
            t = 0.001 * obj2.getNum();
            // some broken font descriptors specify a positive descent
            if (t > 0) {
                t = -t;
            }
            // some broken font descriptors set ascent and descent to 0
            if (t != 0 && t > -3) {
                descent = t;
            }
        }

        // font FontBBox
        obj2 = obj1.dictLookup("FontBBox");
        if (obj2.isArray()) {
            for (int i = 0; i < 4 && i < obj2.arrayGetLength(); ++i) {
                Object obj3 = obj2.arrayGet(i);
                if (obj3.isNum()) {
                    fontBBox[i] = 0.001 * obj3.getNum();
                }
            }
        }
    }
}

std::unique_ptr<CharCodeToUnicode> GfxFont::readToUnicodeCMap(const Dict &fontDict, int nBits, std::unique_ptr<CharCodeToUnicode> ctu)
{

    Object obj1 = fontDict.lookup("ToUnicode");
    if (!obj1.isStream()) {
        return ctu;
    }
    std::string buf;
    obj1.getStream()->fillString(buf);
    obj1.streamClose();
    if (ctu) {
        ctu->mergeCMap(buf, nBits);
    } else {
        ctu = CharCodeToUnicode::parseCMap(buf, nBits);
    }
    hasToUnicode = true;
    return ctu;
}

std::optional<GfxFontLoc> GfxFont::locateFont(XRef *xref, PSOutputDev *ps, GooString *substituteFontName)
{
    SysFontType sysFontType;
    std::optional<std::string> path;
    GooString *base14Name;
    int substIdx, fontNum;
    bool embed;