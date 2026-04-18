// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 3/20

          { .altName = "SymbolMT", .base14Name = "Symbol" },
                                                    { .altName = "SymbolMT,Bold", .base14Name = "Symbol" },
                                                    { .altName = "SymbolMT,BoldItalic", .base14Name = "Symbol" },
                                                    { .altName = "SymbolMT,Italic", .base14Name = "Symbol" },
                                                    { .altName = "Times-Bold", .base14Name = "Times-Bold" },
                                                    { .altName = "Times-BoldItalic", .base14Name = "Times-BoldItalic" },
                                                    { .altName = "Times-Italic", .base14Name = "Times-Italic" },
                                                    { .altName = "Times-Roman", .base14Name = "Times-Roman" },
                                                    { .altName = "TimesNewRoman", .base14Name = "Times-Roman" },
                                                    { .altName = "TimesNewRoman,Bold", .base14Name = "Times-Bold" },
                                                    { .altName = "TimesNewRoman,BoldItalic", .base14Name = "Times-BoldItalic" },
                                                    { .altName = "TimesNewRoman,Italic", .base14Name = "Times-Italic" },
                                                    { .altName = "TimesNewRoman-Bold", .base14Name = "Times-Bold" },
                                                    { .altName = "TimesNewRoman-BoldItalic", .base14Name = "Times-BoldItalic" },
                                                    { .altName = "TimesNewRoman-Italic", .base14Name = "Times-Italic" },
                                                    { .altName = "TimesNewRomanPS", .base14Name = "Times-Roman" },
                                                    { .altName = "TimesNewRomanPS-Bold", .base14Name = "Times-Bold" },
                                                    { .altName = "TimesNewRomanPS-BoldItalic", .base14Name = "Times-BoldItalic" },
                                                    { .altName = "TimesNewRomanPS-BoldItalicMT", .base14Name = "Times-BoldItalic" },
                                                    { .altName = "TimesNewRomanPS-BoldMT", .base14Name = "Times-Bold" },
                                                    { .altName = "TimesNewRomanPS-Italic", .base14Name = "Times-Italic" },
                                                    { .altName = "TimesNewRomanPS-ItalicMT", .base14Name = "Times-Italic" },
                                                    { .altName = "TimesNewRomanPSMT", .base14Name = "Times-Roman" },
                                                    { .altName = "TimesNewRomanPSMT,Bold", .base14Name = "Times-Bold" },
                                                    { .altName = "TimesNewRomanPSMT,BoldItalic", .base14Name = "Times-BoldItalic" },
                                                    { .altName = "TimesNewRomanPSMT,Italic", .base14Name = "Times-Italic" },
                                                    { .altName = "ZapfDingbats", .base14Name = "ZapfDingbats" } };

//------------------------------------------------------------------------

// index: {fixed:0, sans-serif:4, serif:8} + bold*2 + italic
// NB: must be in same order as psSubstFonts in PSOutputDev.cc
static const char *base14SubstFonts[14] = { "Courier", "Courier-Oblique", "Courier-Bold", "Courier-BoldOblique", "Helvetica", "Helvetica-Oblique", "Helvetica-Bold", "Helvetica-BoldOblique", "Times-Roman", "Times-Italic", "Times-Bold",
                                            "Times-BoldItalic",
                                            // the last two are never used for substitution
                                            "Symbol", "ZapfDingbats" };

//------------------------------------------------------------------------

static int parseCharName(char *charName, Unicode *uBuf, int uLen, bool names, bool ligatures, bool numeric, bool hex, bool variants);

//------------------------------------------------------------------------

static int readFromStream(void *data)
{
    return ((Stream *)data)->getChar();
}

//------------------------------------------------------------------------
// GfxFontLoc
//------------------------------------------------------------------------

GfxFontLoc::GfxFontLoc()
{
    fontNum = 0;
    substIdx = -1;
}

GfxFontLoc::~GfxFontLoc() = default;

GfxFontLoc::GfxFontLoc(GfxFontLoc &&other) noexcept = default;

GfxFontLoc &GfxFontLoc::operator=(GfxFontLoc &&other) noexcept = default;

//------------------------------------------------------------------------
// GfxFont
//------------------------------------------------------------------------

std::unique_ptr<GfxFont> GfxFont::makeFont(XRef *xref, const char *tagA, Ref idA, const Dict &fontDict)
{
    std::optional<std::string> name;
    Ref embFontIDA;