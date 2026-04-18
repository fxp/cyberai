// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 14/20

) && (n <= 6 || isxdigit(charName[6]))) {
        unsigned int u;
        sscanf(charName + 1, "%x", &u);
        if (u <= 0xD7FF || (0xE000 <= u && u <= 0x10FFFF)) {
            uBuf[0] = u;
            return 1;
        }
    }
    // Not in Adobe Glyph Mapping convention: look for names like xx
    // or Axx and parse for hex or decimal values.
    if (numeric && parseNumericName(charName, hex, uBuf)) {
        return 1;
    }
    // 3.5. otherwise, map the component to the empty string
    return 0;
}

int Gfx8BitFont::getNextChar(const char *s, int /*len*/, CharCode *code, Unicode const **u, int *uLen, double *dx, double *dy, double *ox, double *oy) const
{
    CharCode c;

    *code = c = (CharCode)(*s & 0xff);
    *uLen = ctu->mapToUnicode(c, u);
    *dx = widths[c];
    *dy = *ox = *oy = 0;
    return 1;
}

const CharCodeToUnicode *Gfx8BitFont::getToUnicode() const
{
    return ctu.get();
}

std::vector<int> Gfx8BitFont::getCodeToGIDMap(FoFiTrueType *ff)
{
    std::vector<int> map;
    int unicodeCmap, macRomanCmap, msSymbolCmap, cmap;
    bool useMacRoman, useUnicode;
    char *charName;
    Unicode u;

    map.resize(256, 0);

    // To match up with the Adobe-defined behaviour, we choose a cmap
    // like this:
    // 1. If the PDF font has an encoding:
    //    1a. If the TrueType font has a Microsoft Unicode
    //        cmap or a non-Microsoft Unicode cmap, use it, and use the
    //        Unicode indexes, not the char codes.
    //    1b. If the PDF font specified MacRomanEncoding and the
    //        TrueType font has a Macintosh Roman cmap, use it, and
    //        reverse map the char names through MacRomanEncoding to
    //        get char codes.
    //    1c. If the PDF font is symbolic and the TrueType font has a
    //        Microsoft Symbol cmap, use it, and use char codes
    //        directly (possibly with an offset of 0xf000).
    //    1d. If the TrueType font has a Macintosh Roman cmap, use it,
    //        as in case 1a.
    // 2. If the PDF font does not have an encoding or the PDF font is
    //    symbolic:
    //    2a. If the TrueType font has a Macintosh Roman cmap, use it,
    //        and use char codes directly (possibly with an offset of
    //        0xf000).
    //    2b. If the TrueType font has a Microsoft Symbol cmap, use it,
    //        and use char codes directly (possible with an offset of
    //        0xf000).
    // 3. If none of these rules apply, use the first cmap and hope for
    //    the best (this shouldn't happen).
    unicodeCmap = macRomanCmap = msSymbolCmap = -1;
    for (int i = 0; i < ff->getNumCmaps(); ++i) {
        const int cmapPlatform = ff->getCmapPlatform(i);
        const int cmapEncoding = ff->getCmapEncoding(i);
        if ((cmapPlatform == 3 && cmapEncoding == 1) || cmapPlatform == 0) {
            unicodeCmap = i;
        } else if (cmapPlatform == 1 && cmapEncoding == 0) {
            macRomanCmap = i;
        } else if (cmapPlatform == 3 && cmapEncoding == 0) {
            msSymbolCmap = i;
        }
    }
    cmap = 0;
    useMacRoman = false;
    useUnicode = false;
    if (hasEncoding || type == fontType1) {
        if (unicodeCmap >= 0) {
            cmap = unicodeCmap;
            useUnicode = true;
        } else if (usesMacRomanEnc && macRomanCmap >= 0) {
            cmap = macRomanCmap;
            useMacRoman = true;
        } else if ((flags & fontSymbolic) && msSymbolCmap >= 0) {
            cmap = msSymbolCmap;
        } else if ((flags & fontSymbolic) && macRomanCmap >= 0) {
            cmap = macRomanCmap;
        } else if (macRomanCmap >= 0) {
            cmap = macRomanCmap;
            useMacRoman = true;
        }
    } else {
        if (msSymbolCmap >= 0) {
            cmap = msSymbolCmap;
        } else if (macRomanCmap >= 0) {
            cmap = macRomanCmap;
        }
    }

    // reverse map the char names through MacRomanEncoding, then map the
    // char codes through the cmap
    if (useMacRoman) {
        for (int i = 0; i < 256; ++i) {
            if ((charName = enc[i])) {
                const CharCode code = globalParams->getMacRomanCharCode(charName);
                if (code > 0) {
                    map[i] = ff->mapCodeToGID(cmap, code);
                }
            } else {
                map[i] = -1;
            }
        }

        // map Unicode through the cmap
    } else if (useUnicode) {
        const Unicode *uAux;
        for (int i = 0; i < 256; ++i) {
            if (((charName = enc[i]) && (u = globalParams->mapNameToUnicodeAll(charName)))) {
                map[i] = ff->mapCodeToGID(cmap, u);
            } else {
                const int n = ctu->mapToUnicode((CharCode)i, &uAux);
                if (n > 0) {
                    map[i] = ff->mapCodeToGID(cmap, uAux[0]);
                } else {
                    map[i] = -1;
                }
            }
        }