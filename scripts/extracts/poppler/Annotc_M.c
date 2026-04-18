// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 45/79



        if (noReencode) {
            outBuf->push_back(uChar);
        } else {
            const CharCodeToUnicode *ccToUnicode = font.getToUnicode();
            if (!ccToUnicode) {
                // This assumes an identity CMap.
                outBuf->push_back((uChar >> 8) & 0xff);
                outBuf->push_back(uChar & 0xff);
            } else if (ccToUnicode->mapToCharCode(&uChar, &c, 1)) {
                if (font.isCIDFont()) {
                    const auto *cidFont = static_cast<const GfxCIDFont *>(&font);
                    if (c < cidFont->getCIDToGIDLen()) {
                        const int glyph = cidFont->getCIDToGID()[c];
                        if (glyph > 0 || c == 0) {
                            outBuf->push_back((c >> 8) & 0xff);
                            outBuf->push_back(c & 0xff);
                        } else {
                            if (newFontNeeded) {
                                *newFontNeeded = true;
                                *i -= unicode ? 2 : 1;
                                break;
                            }
                            outBuf->push_back((c >> 8) & 0xff);
                            outBuf->push_back(c & 0xff);
                            error(errSyntaxError, -1, "AnnotWidget::layoutText, font doesn't have glyph for charcode U+{0:04uX}", c);
                        }
                    } else {
                        // TODO: This assumes an identity CMap.  It should be extended to
                        // handle the general case.
                        outBuf->push_back((c >> 8) & 0xff);
                        outBuf->push_back(c & 0xff);
                    }
                } else {
                    // 8-bit font
                    outBuf->push_back(c);
                }
            } else {
                if (newFontNeeded) {
                    *newFontNeeded = true;
                    *i -= unicode ? 2 : 1;
                    break;
                }
                error(errSyntaxError, -1, "AnnotWidget::layoutText, cannot convert U+{0:04uX}", uChar);
            }
        }

        // If we see a space, then we have a linebreak opportunity.
        if (uChar == ' ') {
            last_i1 = *i;
            if (!spacePrev) {
                last_o1 = last_o2;
            }
            spacePrev = true;
        } else {
            spacePrev = false;
        }

        // Compute width of character just output
        if (outBuf->size() > last_o2) {
            dx = 0.0;
            font.getNextChar(outBuf->c_str() + last_o2, outBuf->size() - last_o2, &c, &uAux, &uLen, &dx, &dy, &ox, &oy);
            w += dx;
        }

        // Current line over-full now?
        if (widthLimit > 0.0 && w > widthLimit) {
            if (last_o1 > 0) {
                // Back up to the previous word which fit, if there was a previous
                // word.
                *i = last_i1;
                outBuf->erase(last_o1, outBuf->size() - last_o1);
            } else if (last_o2 > 0) {
                // Otherwise, back up to the previous character (in the only word on
                // this line)
                *i = last_i2;
                outBuf->erase(last_o2, outBuf->size() - last_o2);
            } else {
                // Otherwise, we were trying to fit the first character; include it
                // anyway even if it overflows the space--no updates to make.
            }
            break;
        }
    }

    // If splitting input into lines because we reached the width limit, then
    // consume any remaining trailing spaces that would go on this line from the
    // input.  If in doing so we reach a newline, consume that also.  This code
    // won't run if we stopped at a newline above, since in that case w <=
    // widthLimit still.
    if (widthLimit > 0.0 && w > widthLimit) {
        if (unicode) {
            while (*i < text->size() && text->getChar(*i) == '\0' && text->getChar(*i + 1) == ' ') {
                *i += 2;
            }
            if (*i < text->size() && text->getChar(*i) == '\0' && text->getChar(*i + 1) == '\r') {
                *i += 2;
            }
            if (*i < text->size() && text->getChar(*i) == '\0' && text->getChar(*i + 1) == '\n') {
                *i += 2;
            }
        } else {
            while (*i < text->size() && text->getChar(*i) == ' ') {
                *i += 1;
            }
            if (*i < text->size() && text->getChar(*i) == '\r') {
                *i += 1;
            }
            if (*i < text->size() && text->getChar(*i) == '\n') {
                *i += 1;
            }
        }
    }

    // Compute the actual width and character count of the final string, based on
    // breakpoint, if this information is requested by the caller.
    if (width != nullptr || charCount != nullptr) {
        const char *s = outBuf->c_str();
        int len = outBuf->size();