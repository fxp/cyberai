// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 12/20



    // use widths from font dict, if present
    obj1 = fontDict.lookup("FirstChar");
    firstChar = obj1.isInt() ? obj1.getInt() : 0;
    if (firstChar < 0 || firstChar > 255) {
        firstChar = 0;
    }
    obj1 = fontDict.lookup("LastChar");
    lastChar = obj1.isInt() ? obj1.getInt() : 255;
    if (lastChar < 0 || lastChar > 255) {
        lastChar = 255;
    }
    const double mul = (type == fontType3) ? fontMat[0] : 0.001;
    obj1 = fontDict.lookup("Widths");
    if (obj1.isArray()) {
        flags |= fontFixedWidth;
        if (obj1.arrayGetLength() < lastChar - firstChar + 1) {
            lastChar = firstChar + obj1.arrayGetLength() - 1;
        }
        double firstNonZeroWidth = 0;
        for (int code = firstChar; code <= lastChar; ++code) {
            Object obj2 = obj1.arrayGet(code - firstChar);
            if (obj2.isNum()) {
                widths[code] = obj2.getNum() * mul;

                // Check if the font is fixed width
                if (firstNonZeroWidth == 0) {
                    firstNonZeroWidth = widths[code];
                }
                if (firstNonZeroWidth != 0 && widths[code] != 0 && fabs(widths[code] - firstNonZeroWidth) > 0.00001) {
                    flags &= ~fontFixedWidth;
                }
            }
        }

        // use widths from built-in font
    } else if (builtinFont) {
        // this is a kludge for broken PDF files that encode char 32
        // as .notdef
        if (builtinFont->getWidth("space", &w)) {
            widths[32] = 0.001 * w;
        }
        for (int code = 0; code < 256; ++code) {
            if (enc[code] && builtinFont->getWidth(enc[code], &w)) {
                widths[code] = 0.001 * w;
            }
        }

        // couldn't find widths -- use defaults
    } else {
        // this is technically an error -- the Widths entry is required
        // for all but the Base-14 fonts -- but certain PDF generators
        // apparently don't include widths for Arial and TimesNewRoman
        int i;
        if (isFixedWidth()) {
            i = 0;
        } else if (isSerif()) {
            i = 8;
        } else {
            i = 4;
        }
        if (isBold()) {
            i += 2;
        }
        if (isItalic()) {
            i += 1;
        }
        builtinFont = builtinFontSubst[i];
        // this is a kludge for broken PDF files that encode char 32
        // as .notdef
        if (builtinFont->getWidth("space", &w)) {
            widths[32] = 0.001 * w;
        }
        for (int code = 0; code < 256; ++code) {
            if (enc[code] && builtinFont->getWidth(enc[code], &w)) {
                widths[code] = 0.001 * w;
            }
        }
    }

    ok = true;
}

Gfx8BitFont::~Gfx8BitFont()
{
    int i;

    for (i = 0; i < 256; ++i) {
        if (encFree[i] && enc[i]) {
            gfree(enc[i]);
        }
    }
}