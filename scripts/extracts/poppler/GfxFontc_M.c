// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 13/20



// This function is in part a derived work of the Adobe Glyph Mapping
// Convention: http://www.adobe.com/devnet/opentype/archives/glyph.html
// Algorithmic comments are excerpted from that document to aid
// maintainability.
static int parseCharName(char *charName, Unicode *uBuf, int uLen, bool names, bool ligatures, bool numeric, bool hex, bool variants)
{
    if (uLen <= 0) {
        error(errInternal, -1,
              "Zero-length output buffer (recursion overflow?) in "
              "parseCharName, component \"{0:s}\"",
              charName);
        return 0;
    }
    // Step 1: drop all the characters from the glyph name starting with the
    // first occurrence of a period (U+002E FULL STOP), if any.
    if (variants) {
        char *var_part = strchr(charName, '.');
        if (var_part == charName) {
            return 0; // .notdef or similar
        }
        if (var_part != nullptr) {
            // parse names of the form 7.oldstyle, P.swash, s.sc, etc.
            char *main_part = copyString(charName, var_part - charName);
            bool namesRecurse = true, variantsRecurse = false;
            int n = parseCharName(main_part, uBuf, uLen, namesRecurse, ligatures, numeric, hex, variantsRecurse);
            gfree(main_part);
            return n;
        }
    }
    // Step 2: split the remaining string into a sequence of components, using
    // underscore (U+005F LOW LINE) as the delimiter.
    if (ligatures && strchr(charName, '_')) {
        // parse names of the form A_a (e.g. f_i, T_h, l_quotesingle)
        char *lig_part, *lig_end, *lig_copy;
        int n = 0, m;
        lig_part = lig_copy = copyString(charName);
        do {
            if ((lig_end = strchr(lig_part, '_'))) {
                *lig_end = '\0';
            }
            if (lig_part[0] != '\0') {
                bool namesRecurse = true, ligaturesRecurse = false;
                if ((m = parseCharName(lig_part, uBuf + n, uLen - n, namesRecurse, ligaturesRecurse, numeric, hex, variants))) {
                    n += m;
                } else {
                    error(errSyntaxWarning, -1,
                          "Could not parse ligature component \"{0:s}\" of \"{1:s}\" in "
                          "parseCharName",
                          lig_part, charName);
                }
            }
            if (lig_end) {
                lig_part = lig_end + 1;
            }
        } while (lig_end && n < uLen);
        gfree(lig_copy);
        return n;
    }
    // Step 3: map each component to a character string according to the
    // procedure below, and concatenate those strings; the result is the
    // character string to which the glyph name is mapped.
    // 3.1. if the font is Zapf Dingbats (PostScript FontName ZapfDingbats), and
    // the component is in the ZapfDingbats list, then map it to the
    // corresponding character in that list.
    // 3.2. otherwise, if the component is in the Adobe Glyph List, then map it
    // to the corresponding character in that list.
    if (names && (uBuf[0] = globalParams->mapNameToUnicodeText(charName))) {
        return 1;
    }
    unsigned int n = strlen(charName);
    // 3.3. otherwise, if the component is of the form "uni" (U+0075 U+006E
    // U+0069) followed by a sequence of uppercase hexadecimal digits (0 .. 9,
    // A .. F, i.e. U+0030 .. U+0039, U+0041 .. U+0046), the length of that
    // sequence is a multiple of four, and each group of four digits represents
    // a number in the set {0x0000 .. 0xD7FF, 0xE000 .. 0xFFFF}, then interpret
    // each such number as a Unicode scalar value and map the component to the
    // string made of those scalar values. Note that the range and digit length
    // restrictions mean that the "uni" prefix can be used only with Unicode
    // values from the Basic Multilingual Plane (BMP).
    if (n >= 7 && (n % 4) == 3 && !strncmp(charName, "uni", 3)) {
        int i;
        unsigned int m;
        for (i = 0, m = 3; i < uLen && m < n; m += 4) {
            if (isxdigit(charName[m]) && isxdigit(charName[m + 1]) && isxdigit(charName[m + 2]) && isxdigit(charName[m + 3])) {
                unsigned int u;
                sscanf(charName + m, "%4x", &u);
                if (u <= 0xD7FF || (0xE000 <= u && u <= 0xFFFF)) {
                    uBuf[i++] = u;
                }
            }
        }
        return i;
    }
    // 3.4. otherwise, if the component is of the form "u" (U+0075) followed by
    // a sequence of four to six uppercase hexadecimal digits {0 .. 9, A .. F}
    // (U+0030 .. U+0039, U+0041 .. U+0046), and those digits represent a
    // number in {0x0000 .. 0xD7FF, 0xE000 .. 0x10FFFF}, then interpret this
    // number as a Unicode scalar value and map the component to the string
    // made of this scalar value.
    if (n >= 5 && n <= 7 && charName[0] == 'u' && isxdigit(charName[1]) && isxdigit(charName[2]) && isxdigit(charName[3]) && isxdigit(charName[4]) && (n <= 5 || isxdigit(charName[5])