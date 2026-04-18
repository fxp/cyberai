// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 39/79



    appearBBox = std::make_unique<AnnotAppearanceBBox>(rect.get());
    AnnotAppearanceBuilder appearBuilder;
    appearBuilder.append("q\n");
    if (color) {
        appearBuilder.setDrawColor(*color, false);
    }
    if (interiorColor) {
        appearBuilder.setDrawColor(*interiorColor, true);
        fill = true;
    }
    appearBuilder.setLineStyleForBorder(*border);
    borderWidth = border->getWidth();
    appearBBox->setBorderWidth(std::max(1., borderWidth));

    const double x1 = coord1->getX();
    const double y1 = coord1->getY();
    const double x2 = coord2->getX();
    const double y2 = coord2->getY();

    // Main segment length
    const double main_len = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));

    // Main segment becomes positive x direction, coord1 becomes (0,0)
    Matrix matr;
    const double angle = atan2(y2 - y1, x2 - x1);
    matr.m[0] = matr.m[3] = cos(angle);
    matr.m[1] = sin(angle);
    matr.m[2] = -matr.m[1];
    matr.m[4] = x1 - rect->x1;
    matr.m[5] = y1 - rect->y1;

    double tx, ty, captionwidth = 0, captionheight = 0;
    AnnotLineCaptionPos actualCaptionPos = captionPos;
    const double fontsize = 9;
    const double captionhmargin = 2; // Left and right margin (inline caption only)
    const double captionmaxwidth = main_len - 2 * captionhmargin;
    const double lineendingSize = std::min(6. * borderWidth, main_len / 2);

    std::unique_ptr<Dict> fontResDict;
    std::unique_ptr<const GfxFont> font;

    // Calculate caption width and height
    if (caption) {
        fontResDict = std::make_unique<Dict>(doc->getXRef());
        font = createAnnotDrawFont(doc->getXRef(), fontResDict.get());
        int lines = 0;
        size_t i = 0;
        while (i < contents->size()) {
            GooString out;
            double linewidth;
            layoutText(contents.get(), &out, &i, *font, &linewidth, 0, nullptr, false);
            linewidth *= fontsize;
            if (linewidth > captionwidth) {
                captionwidth = linewidth;
            }
            ++lines;
        }
        captionheight = lines * fontsize;
        // If text is longer than available space, turn into captionPosTop
        if (captionwidth > captionmaxwidth) {
            actualCaptionPos = captionPosTop;
        }
    } else {
        fontResDict = nullptr;
        font = nullptr;
    }

    // Draw main segment
    matr.transform(AnnotAppearanceBuilder::lineEndingXShorten(startStyle, lineendingSize), leaderLineLength, &tx, &ty);
    appearBuilder.appendf("{0:.2f} {1:.2f} m\n", tx, ty);
    appearBBox->extendTo(tx, ty);

    if (captionwidth != 0 && actualCaptionPos == captionPosInline) { // Break in the middle
        matr.transform((main_len - captionwidth) / 2 - captionhmargin, leaderLineLength, &tx, &ty);
        appearBuilder.appendf("{0:.2f} {1:.2f} l S\n", tx, ty);

        matr.transform((main_len + captionwidth) / 2 + captionhmargin, leaderLineLength, &tx, &ty);
        appearBuilder.appendf("{0:.2f} {1:.2f} m\n", tx, ty);
    }

    matr.transform(main_len - AnnotAppearanceBuilder::lineEndingXShorten(endStyle, lineendingSize), leaderLineLength, &tx, &ty);
    appearBuilder.appendf("{0:.2f} {1:.2f} l S\n", tx, ty);
    appearBBox->extendTo(tx, ty);

    if (startStyle != annotLineEndingNone) {
        const double extendX { -AnnotAppearanceBuilder::lineEndingXExtendBBox(startStyle, lineendingSize) };
        appearBuilder.drawLineEnding(startStyle, 0, leaderLineLength, -lineendingSize, fill, matr);
        matr.transform(extendX, leaderLineLength + lineendingSize / 2., &tx, &ty);
        appearBBox->extendTo(tx, ty);
        matr.transform(extendX, leaderLineLength - lineendingSize / 2., &tx, &ty);
        appearBBox->extendTo(tx, ty);
    }

    if (endStyle != annotLineEndingNone) {
        const double extendX { AnnotAppearanceBuilder::lineEndingXExtendBBox(endStyle, lineendingSize) };
        appearBuilder.drawLineEnding(endStyle, main_len, leaderLineLength, lineendingSize, fill, matr);
        matr.transform(main_len + extendX, leaderLineLength + lineendingSize / 2., &tx, &ty);
        appearBBox->extendTo(tx, ty);
        matr.transform(main_len + extendX, leaderLineLength - lineendingSize / 2., &tx, &ty);
        appearBBox->extendTo(tx, ty);
    }

    // Draw caption text
    if (caption) {
        double tlx = (main_len - captionwidth) / 2, tly; // Top-left coords
        if (actualCaptionPos == captionPosInline) {
            tly = leaderLineLength + captionheight / 2;
        } else {
            tly = leaderLineLength + captionheight + 2 * borderWidth;
        }

        tlx += captionTextHorizontal;
        tly += captionTextVertical;