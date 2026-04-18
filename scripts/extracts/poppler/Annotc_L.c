// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 44/79



// Grand unified handler for preparing text strings to be drawn into form
// fields.  Takes as input a text string (in PDFDocEncoding or UTF-16).
// Converts some or all of this string to the appropriate encoding for the
// specified font, and computes the width of the text.  Can optionally stop
// converting when a specified width has been reached, to perform line-breaking
// for multi-line fields.
//
// Parameters:
//   text: input text string to convert
//   outBuf: buffer for writing re-encoded string
//   i: index at which to start converting; will be updated to point just after
//      last character processed
//   font: the font which will be used for display
//   width: computed width (unscaled by font size) will be stored here
//   widthLimit: if non-zero, stop converting to keep width under this value
//      (should be scaled down by font size)
//   charCount: count of number of characters will be stored here
//   noReencode: if set, do not try to translate the character encoding
//      (useful for Zapf Dingbats or other unusual encodings)
//      can only be used with simple fonts, not CID-keyed fonts
//
// TODO: Handle surrogate pairs in UTF-16.
//       Should be able to generate output for any CID-keyed font.
//       Doesn't handle vertical fonts--should it?
void Annot::layoutText(const GooString *text, GooString *outBuf, size_t *i, const GfxFont &font, double *width, double widthLimit, int *charCount, bool noReencode, bool *newFontNeeded)
{
    CharCode c;
    Unicode uChar;
    const Unicode *uAux;
    double w = 0.0;
    int uLen, n;
    double dx, dy, ox, oy;

    if (newFontNeeded) {
        *newFontNeeded = false;
    }
    if (width != nullptr) {
        *width = 0.0;
    }
    if (charCount != nullptr) {
        *charCount = 0;
    }

    if (!text) {
        return;
    }
    bool unicode = hasUnicodeByteOrderMark(text->toStr());
    bool spacePrev; // previous character was a space

    // State for backtracking when more text has been processed than fits within
    // widthLimit.  We track for both input (text) and output (outBuf) the offset
    // to the first character to discard.
    //
    // We keep track of several points:
    //   1 - end of previous completed word which fits
    //   2 - previous character which fit
    size_t last_i1, last_i2, last_o1, last_o2;

    if (unicode && text->size() % 2 != 0) {
        error(errSyntaxError, -1, "AnnotWidget::layoutText, bad unicode string");
        *i += 1;
        return;
    }

    // skip Unicode marker on string if needed
    if (unicode && *i == 0) {
        *i = 2;
    }

    // Start decoding and copying characters, until either:
    //   we reach the end of the string
    //   we reach the maximum width
    //   we reach a newline character
    // As we copy characters, keep track of the last full word to fit, so that we
    // can backtrack if we exceed the maximum width.
    last_i1 = last_i2 = *i;
    last_o1 = last_o2 = 0;
    spacePrev = false;
    outBuf->clear();

    while (*i < text->size()) {
        last_i2 = *i;
        last_o2 = outBuf->size();

        if (unicode) {
            uChar = (unsigned char)(text->getChar(*i)) << 8;
            uChar += (unsigned char)(text->getChar(*i + 1));
            *i += 2;
        } else {
            if (noReencode) {
                uChar = text->getChar(*i) & 0xff;
            } else {
                uChar = pdfDocEncoding[text->getChar(*i) & 0xff];
            }
            *i += 1;
        }

        // Explicit line break?
        if (uChar == '\r' || uChar == '\n') {
            // Treat a <CR><LF> sequence as a single line break
            if (uChar == '\r' && *i < text->size()) {
                if (unicode && text->getChar(*i) == '\0' && text->getChar(*i + 1) == '\n') {
                    *i += 2;
                } else if (!unicode && text->getChar(*i) == '\n') {
                    *i += 1;
                }
            }

            break;
        }