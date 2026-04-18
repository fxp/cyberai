// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 18/20



std::vector<int> GfxCIDFont::getCodeToGIDMap(FoFiTrueType *ff)
{
#define N_UCS_CANDIDATES 2
    /* space characters */
    static const unsigned long spaces[] = { 0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200A, 0x00A0, 0x200B, 0x2060, 0x3000, 0xFEFF, 0 };
    static const std::array<std::string, 4> adobe_cns1_cmaps = { "UniCNS-UTF32-V", "UniCNS-UCS2-V", "UniCNS-UTF32-H", "UniCNS-UCS2-H" };
    static const std::array<std::string, 4> adobe_gb1_cmaps = { "UniGB-UTF32-V", "UniGB-UCS2-V", "UniGB-UTF32-H", "UniGB-UCS2-H" };
    static const std::array<std::string, 4> adobe_japan1_cmaps = { "UniJIS-UTF32-V", "UniJIS-UCS2-V", "UniJIS-UTF32-H", "UniJIS-UCS2-H" };
    static const std::array<std::string, 4> adobe_japan2_cmaps = { "UniHojo-UTF32-V", "UniHojo-UCS2-V", "UniHojo-UTF32-H", "UniHojo-UCS2-H" };
    static const std::array<std::string, 4> adobe_korea1_cmaps = { "UniKS-UTF32-V", "UniKS-UCS2-V", "UniKS-UTF32-H", "UniKS-UCS2-H" };
    static const struct CMapListEntry
    {
        const char *collection;
        const std::string scriptTag;
        const std::string languageTag;
        const std::string toUnicodeMap;
        const std::array<std::string, 4> *CMaps;
    } CMapList[] = { {
                             .collection = "Adobe-CNS1",
                             .scriptTag = "hani",
                             .languageTag = "CHN ",
                             .toUnicodeMap = "Adobe-CNS1-UCS2",
                             .CMaps = &adobe_cns1_cmaps,
                     },
                     {
                             .collection = "Adobe-GB1",
                             .scriptTag = "hani",
                             .languageTag = "CHN ",
                             .toUnicodeMap = "Adobe-GB1-UCS2",
                             .CMaps = &adobe_gb1_cmaps,
                     },
                     {
                             .collection = "Adobe-Japan1",
                             .scriptTag = "kana",
                             .languageTag = "JAN ",
                             .toUnicodeMap = "Adobe-Japan1-UCS2",
                             .CMaps = &adobe_japan1_cmaps,
                     },
                     {
                             .collection = "Adobe-Japan2",
                             .scriptTag = "kana",
                             .languageTag = "JAN ",
                             .toUnicodeMap = "Adobe-Japan2-UCS2",
                             .CMaps = &adobe_japan2_cmaps,
                     },
                     {
                             .collection = "Adobe-Korea1",
                             .scriptTag = "hang",
                             .languageTag = "KOR ",
                             .toUnicodeMap = "Adobe-Korea1-UCS2",
                             .CMaps = &adobe_korea1_cmaps,
                     },
                     { .collection = nullptr, .scriptTag = {}, .languageTag = {}, .toUnicodeMap = {}, .CMaps = nullptr } };
    Unicode *humap = nullptr;
    Unicode *vumap = nullptr;
    Unicode *tumap = nullptr;
    int i;
    const CMapListEntry *lp;
    int cmap;
    Ref embID;

    if (!ctu || !getCollection()) {
        return {};
    }

    if (getEmbeddedFontID(&embID)) {
        if (getCollection()->compare("Adobe-Identity") == 0) {
            return {};
        }

        /* if this font is embedded font,
         * CIDToGIDMap should be embedded in PDF file
         * and already set. So return it.
         */
        return getCIDToGID();
    }

    /* we use only unicode cmap */
    cmap = -1;
    for (i = 0; i < ff->getNumCmaps(); ++i) {
        const int cmapPlatform = ff->getCmapPlatform(i);
        const int cmapEncoding = ff->getCmapEncoding(i);
        if (cmapPlatform == 3 && cmapEncoding == 10) {
            /* UCS-4 */
            cmap = i;
            /* use UCS-4 cmap */
            break;
        }
        if (cmapPlatform == 3 && cmapEncoding == 1) {
            /* Unicode */
            cmap = i;
        } else if (cmapPlatform == 0 && cmap < 0) {
            cmap = i;
        }
    }
    if (cmap < 0) {
        return {};
    }

    const GfxFont::WritingMode wmode = getWMode();
    for (lp = CMapList; lp->collection != nullptr; lp++) {
        if (strcmp(lp->collection, getCollection()->c_str()) == 0) {
            break;
        }
    }
    const unsigned int n = 65536;
    humap = new Unicode[n * N_UCS_CANDIDATES];
    memset(humap, 0, sizeof(Unicode) * n * N_UCS_CANDIDATES);
    if (lp->collection != nullptr) {
        if (std::unique_ptr<CharCodeToUnicode> tctu = CharCodeToUnicode::parseCMapFromFile(lp->toUnicodeMap, 16)) {
            tumap = new Unicode[n];
            CharCode cid;
            for (cid = 0; cid < n; cid++) {
                int len;
                const Unicode *ucodes;