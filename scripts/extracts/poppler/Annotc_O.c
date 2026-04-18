// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 47/79



        textToFree = std::make_unique<GooString>();
        for (int i = 0; i < len; ++i) {
            textToFree->push_back('*');
        }
        text = textToFree.get();
    }

    // setup
    if (flags & EmitMarkedContentDrawTextFlag) {
        appearBuf->append("/Tx BMC\n");
    }
    appearBuf->append("q\n");
    auto calculateDxDy = [this, appearCharacs, rect]() -> std::tuple<double, double> {
        const int rot = appearCharacs ? appearCharacs->getRotation() : 0;
        switch (rot) {
        case 90:
            appearBuf->appendf("0 1 -1 0 {0:.2f} 0 cm\n", rect->x2 - rect->x1);
            return { rect->y2 - rect->y1, rect->x2 - rect->x1 };

        case 180:
            appearBuf->appendf("-1 0 0 -1 {0:.2f} {1:.2f} cm\n", rect->x2 - rect->x1, rect->y2 - rect->y1);
            return { rect->x2 - rect->y2, rect->y2 - rect->y1 };

        case 270:
            appearBuf->appendf("0 -1 1 0 0 {0:.2f} cm\n", rect->y2 - rect->y1);
            return { rect->y2 - rect->y1, rect->x2 - rect->x1 };

        default: // assume rot == 0
            return { rect->x2 - rect->x1, rect->y2 - rect->y1 };
        }
    };
    const auto dxdy = calculateDxDy();
    const double dx = std::get<0>(dxdy);
    const double dy = std::get<1>(dxdy);
    appearBuf->append("BT\n");
    // multi-line text
    if (flags & MultilineDrawTextFlag) {
        // note: comb is ignored in multiline mode as mentioned in the spec

        const double wMax = dx - 2 * borderWidth - 4;

        // compute font autosize
        if (fontSize == 0) {
            fontSize = Annot::calculateFontSize(form, font, text, wMax, dy, forceZapfDingbats);
            daToks[tfPos + 1] = GooString().appendf("{0:.2f}", fontSize)->toStr();
        }

        // starting y coordinate
        // (note: each line of text starts with a Td operator that moves
        // down a line)
        const double y = dy - 3;

        // set the font matrix
        daToks[tmPos + 4] = "0";
        daToks[tmPos + 5] = GooString().appendf("{0:.2f}", y)->toStr();

        // write the DA string
        for (const std::string &daTok : daToks) {
            appearBuf->append(daTok);
            appearBuf->push_back(' ');
        }

        const DrawMultiLineTextResult textCommands = drawMultiLineText(text->toStr(), dx, form, *font, std::string(), fontSize, quadding, borderWidth + 2);
        appearBuf->append(textCommands.text);

        // single-line text
    } else {
        //~ replace newlines with spaces? - what does Acrobat do?

        // comb formatting
        if (nCombs > 0) {
            // compute comb spacing
            const double w = (dx - 2 * borderWidth) / nCombs;

            // compute font autosize
            if (fontSize == 0) {
                fontSize = dy - 2 * borderWidth;
                if (w < fontSize) {
                    fontSize = w;
                }
                fontSize = floor(fontSize);
                daToks[tfPos + 1] = GooString().appendf("{0:.2f}", fontSize)->toStr();
            }

            const HorizontalTextLayouter textLayouter(text, form, font, {}, forceZapfDingbats);

            const int charCount = std::min(textLayouter.totalCharCount(), nCombs);

            // compute starting text cell
            auto calculateX = [quadding, borderWidth, nCombs, charCount, w] {
                switch (quadding) {
                case VariableTextQuadding::leftJustified:
                default:
                    return borderWidth;
                case VariableTextQuadding::centered:
                    return borderWidth + (nCombs - charCount) / 2.0 * w;
                case VariableTextQuadding::rightJustified:
                    return borderWidth + (nCombs - charCount) * w;
                }
            };
            const double x = calculateX();
            const double y = 0.5 * dy - 0.4 * fontSize;

            // set the font matrix
            daToks[tmPos + 4] = GooString().appendf("{0:.2f}", x)->toStr();
            daToks[tmPos + 5] = GooString().appendf("{0:.2f}", y)->toStr();

            // write the DA string
            for (const std::string &daTok : daToks) {
                appearBuf->append(daTok);
                appearBuf->push_back(' ');
            }

            // write the text string
            int i = 0;
            double xPrev = w; // so that first character is placed properly
            for (const HorizontalTextLayouter::Data &d : textLayouter.data) {
                const char *s = d.text.c_str();
                int len = d.text.size();
                while (i < nCombs && len > 0) {
                    CharCode code;
                    const Unicode *uAux;
                    int uLen, n;
                    double char_dx, char_dy, ox, oy;