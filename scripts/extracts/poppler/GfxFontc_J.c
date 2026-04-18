// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 10/20



    // Encodings start with a base encoding, which can come from
    // (in order of priority):
    //   1. FontDict.Encoding or FontDict.Encoding.BaseEncoding
    //        - MacRoman / MacExpert / WinAnsi / Standard
    //   2. embedded or external font file
    //   3. default:
    //        - builtin --> builtin encoding
    //        - TrueType --> WinAnsiEncoding
    //        - others --> StandardEncoding
    // and then add a list of differences (if any) from
    // FontDict.Encoding.Differences.

    // check FontDict for base encoding
    hasEncoding = false;
    usesMacRomanEnc = false;
    const std::array<const char *, 256> *baseEnc = nullptr;
    baseEncFromFontFile = false;
    obj1 = fontDict.lookup("Encoding");
    bool isZapfDingbats = name && name->ends_with("ZapfDingbats");
    if (isZapfDingbats) {
        baseEnc = &zapfDingbatsEncoding;
        hasEncoding = true;
    } else if (obj1.isDict()) {
        Object obj2 = obj1.dictLookup("BaseEncoding");
        if (obj2.isName("MacRomanEncoding")) {
            hasEncoding = true;
            usesMacRomanEnc = true;
            baseEnc = &macRomanEncoding;
        } else if (obj2.isName("MacExpertEncoding")) {
            hasEncoding = true;
            baseEnc = &macExpertEncoding;
        } else if (obj2.isName("WinAnsiEncoding")) {
            hasEncoding = true;
            baseEnc = &winAnsiEncoding;
        }
    } else if (obj1.isName("MacRomanEncoding")) {
        hasEncoding = true;
        usesMacRomanEnc = true;
        baseEnc = &macRomanEncoding;
    } else if (obj1.isName("MacExpertEncoding")) {
        hasEncoding = true;
        baseEnc = &macExpertEncoding;
    } else if (obj1.isName("WinAnsiEncoding")) {
        hasEncoding = true;
        baseEnc = &winAnsiEncoding;
    }
    // We need to keep ffT1 around until we have read everything out of 'baseEnc'
    // in case we end up using that
    std::unique_ptr<FoFiType1> ffT1;
    std::unique_ptr<FoFiType1C> ffT1C;

    // check embedded font file for base encoding
    // (only for Type 1 fonts - trying to get an encoding out of a
    // TrueType font is a losing proposition)
    if (type == fontType1 && embFontID != Ref::INVALID()) {
        std::optional<std::vector<unsigned char>> buf = readEmbFontFile(xref);
        if (buf) {
            if ((ffT1 = FoFiType1::make(std::move(buf).value()))) {
                const std::string fontName = ffT1->getName();
                if (!fontName.empty()) {
                    embFontName = std::make_unique<GooString>(fontName);
                }
                if (!baseEnc) {
                    baseEnc = ffT1->getEncodingA();
                    baseEncFromFontFile = true;
                }
            }
        }
    } else if (type == fontType1C && embFontID != Ref::INVALID()) {
        std::optional<std::vector<unsigned char>> buf = readEmbFontFile(xref);
        if (buf) {
            if ((ffT1C = FoFiType1C::make(std::move(buf).value()))) {
                if (ffT1C->getName()) {
                    embFontName = std::make_unique<GooString>(ffT1C->getName());
                }
                if (!baseEnc) {
                    baseEnc = ffT1C->getEncodingA();
                    baseEncFromFontFile = true;
                }
            }
        }
    }

    // get default base encoding
    if (!baseEnc) {
        if (builtinFont && embFontID == Ref::INVALID()) {
            baseEnc = builtinFont->defaultBaseEnc;
            hasEncoding = true;
        } else if (type == fontTrueType) {
            baseEnc = &winAnsiEncoding;
        } else {
            baseEnc = &fofiType1StandardEncoding;
        }
    }

    if (baseEncFromFontFile) {
        encodingName = "Builtin";
    } else if (baseEnc == &winAnsiEncoding) {
        encodingName = "WinAnsi";
    } else if (baseEnc == &macRomanEncoding) {
        encodingName = "MacRoman";
    } else if (baseEnc == &macExpertEncoding) {
        encodingName = "MacExpert";
    } else if (baseEnc == &symbolEncoding) {
        encodingName = "Symbol";
    } else if (baseEnc == &zapfDingbatsEncoding) {
        encodingName = "ZapfDingbats";
    } else {
        encodingName = "Standard";
    }

    // copy the base encoding
    for (int i = 0; i < 256; ++i) {
        enc[i] = (char *)(*baseEnc)[i];
        if ((encFree[i] = baseEncFromFontFile) && enc[i]) {
            enc[i] = copyString((*baseEnc)[i]);
        }
    }

    // some Type 1C font files have empty encodings, which can break the
    // T1C->T1 conversion (since the 'seac' operator depends on having
    // the accents in the encoding), so we fill in any gaps from
    // StandardEncoding
    if (type == fontType1C && embFontID != Ref::INVALID() && baseEncFromFontFile) {
        for (int i = 0; i < 256; ++i) {
            if (!enc[i] && fofiType1StandardEncoding[i]) {
                enc[i] = (char *)fofiType1StandardEncoding[i];
                encFree[i] = false;
            }
        }
    }