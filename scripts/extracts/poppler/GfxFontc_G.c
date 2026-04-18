// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 7/20



    if (type == fontType3) {
        return std::nullopt;
    }

    //----- embedded font
    if (embFontID != Ref::INVALID()) {
        embed = true;
        Object refObj(embFontID);
        Object embFontObj = refObj.fetch(xref);
        if (!embFontObj.isStream()) {
            error(errSyntaxError, -1, "Embedded font object is wrong type");
            embed = false;
        }
        if (embed) {
            if (ps) {
                switch (type) {
                case fontType1:
                case fontType1C:
                case fontType1COT:
                    embed = ps->getEmbedType1();
                    break;
                case fontTrueType:
                case fontTrueTypeOT:
                    embed = ps->getEmbedTrueType();
                    break;
                case fontCIDType0C:
                case fontCIDType0COT:
                    embed = ps->getEmbedCIDPostScript();
                    break;
                case fontCIDType2:
                case fontCIDType2OT:
                    embed = ps->getEmbedCIDTrueType();
                    break;
                default:
                    break;
                }
            }
            if (embed) {
                GfxFontLoc fontLoc;
                fontLoc.locType = gfxFontLocEmbedded;
                fontLoc.fontType = type;
                fontLoc.embFontID = embFontID;
                return fontLoc;
            }
        }
    }

    //----- PS passthrough
    if (ps && !isCIDFont() && ps->getFontPassthrough()) {
        GfxFontLoc fontLoc;
        fontLoc.locType = gfxFontLocResident;
        fontLoc.fontType = fontType1;
        fontLoc.path = name.value_or(std::string());
        return fontLoc;
    }

    //----- PS resident Base-14 font
    if (ps && !isCIDFont() && ((Gfx8BitFont *)this)->base14) {
        GfxFontLoc fontLoc;
        fontLoc.locType = gfxFontLocResident;
        fontLoc.fontType = fontType1;
        fontLoc.path = ((Gfx8BitFont *)this)->base14->base14Name;
        return fontLoc;
    }

    //----- external font file (fontFile, fontDir)
    if (name && (path = globalParams->findFontFile(*name))) {
        if (std::optional<GfxFontLoc> fontLoc = getExternalFont(*path, isCIDFont())) {
            return fontLoc;
        }
    }

    //----- external font file for Base-14 font
    if (!ps && !isCIDFont() && ((Gfx8BitFont *)this)->base14) {
        base14Name = new GooString(((Gfx8BitFont *)this)->base14->base14Name);
        if ((path = globalParams->findBase14FontFile(base14Name, *this, substituteFontName))) {
            if (std::optional<GfxFontLoc> fontLoc = getExternalFont(*path, false)) {
                delete base14Name;
                return fontLoc;
            }
        }
        delete base14Name;
    }

    //----- system font
    if ((path = globalParams->findSystemFontFile(*this, &sysFontType, &fontNum, substituteFontName))) {
        if (isCIDFont()) {
            if (sysFontType == sysFontTTF || sysFontType == sysFontTTC) {
                GfxFontLoc fontLoc;
                fontLoc.locType = gfxFontLocExternal;
                fontLoc.fontType = fontCIDType2;
                fontLoc.path = *path;
                fontLoc.fontNum = fontNum;
                return fontLoc;
            }
        } else {
            GfxFontLoc fontLoc;
            fontLoc.path = *path;
            fontLoc.locType = gfxFontLocExternal;
            if (sysFontType == sysFontTTF || sysFontType == sysFontTTC) {
                fontLoc.fontType = fontTrueType;
            } else if (sysFontType == sysFontPFA || sysFontType == sysFontPFB) {
                fontLoc.fontType = fontType1;
                fontLoc.fontNum = fontNum;
            }
            return fontLoc;
        }
    }

    if (!isCIDFont()) {