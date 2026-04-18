// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 40/79



        // Adjust bounding box
        matr.transform(tlx, tly - captionheight, &tx, &ty);
        appearBBox->extendTo(tx, ty);
        matr.transform(tlx + captionwidth, tly - captionheight, &tx, &ty);
        appearBBox->extendTo(tx, ty);
        matr.transform(tlx + captionwidth, tly, &tx, &ty);
        appearBBox->extendTo(tx, ty);
        matr.transform(tlx, tly, &tx, &ty);
        appearBBox->extendTo(tx, ty);

        // Setup text state (reusing transformed top-left coord)
        appearBuilder.appendf("0 g BT /AnnotDrawFont {0:.2f} Tf\n", fontsize); // Font color: black
        appearBuilder.appendf("{0:.2f} {1:.2f} {2:.2f} {3:.2f} {4:.2f} {5:.2f} Tm\n", matr.m[0], matr.m[1], matr.m[2], matr.m[3], tx, ty);
        appearBuilder.appendf("0 {0:.2f} Td\n", -fontsize * font->getDescent());
        // Draw text
        size_t i = 0;
        double xposPrev = 0;
        while (i < contents->size()) {
            GooString out;
            double linewidth, xpos;
            layoutText(contents.get(), &out, &i, *font, &linewidth, 0, nullptr, false);
            linewidth *= fontsize;
            xpos = (captionwidth - linewidth) / 2;
            appearBuilder.appendf("{0:.2f} {1:.2f} Td\n", xpos - xposPrev, -fontsize);
            appearBuilder.writeString(out.toStr());
            appearBuilder.append("Tj\n");
            xposPrev = xpos;
        }
        appearBuilder.append("ET\n");
    }

    // Draw leader lines
    double ll_len = fabs(leaderLineLength) + leaderLineExtension;
    double sign = leaderLineLength >= 0 ? 1 : -1;
    if (ll_len != 0) {
        matr.transform(0, 0, &tx, &ty);
        appearBuilder.appendf("{0:.2f} {1:.2f} m\n", tx, ty);
        appearBBox->extendTo(tx, ty);
        matr.transform(0, sign * ll_len, &tx, &ty);
        appearBuilder.appendf("{0:.2f} {1:.2f} l S\n", tx, ty);
        appearBBox->extendTo(tx, ty);

        matr.transform(main_len, 0, &tx, &ty);
        appearBuilder.appendf("{0:.2f} {1:.2f} m\n", tx, ty);
        appearBBox->extendTo(tx, ty);
        matr.transform(main_len, sign * ll_len, &tx, &ty);
        appearBuilder.appendf("{0:.2f} {1:.2f} l S\n", tx, ty);
        appearBBox->extendTo(tx, ty);
    }

    appearBuilder.append("Q\n");

    const std::array<double, 4> bbox = appearBBox->getBBoxRect();
    if (ca == 1) {
        appearance = createForm(appearBuilder.buffer(), bbox, false, std::move(fontResDict));
    } else {
        Object aStream = createForm(appearBuilder.buffer(), bbox, true, std::move(fontResDict));

        GooString appearBuf("/GS0 gs\n/Fm0 Do");
        std::unique_ptr<Dict> resDict = createResourcesDict("Fm0", std::move(aStream), "GS0", ca, nullptr);
        appearance = createForm(&appearBuf, bbox, false, std::move(resDict));
    }
}

void AnnotLine::draw(Gfx *gfx, bool printing)
{
    if (!isVisible(printing)) {
        return;
    }

    annotLocker();
    if (appearance.isNull()) {
        generateLineAppearance();
    }

    // draw the appearance stream
    Object obj = appearance.fetch(gfx->getXRef());
    if (appearBBox) {
        gfx->drawAnnot(&obj, nullptr, color.get(), appearBBox->getPageXMin(), appearBBox->getPageYMin(), appearBBox->getPageXMax(), appearBBox->getPageYMax(), getRotation());
    } else {
        gfx->drawAnnot(&obj, nullptr, color.get(), rect->x1, rect->y1, rect->x2, rect->y2, getRotation());
    }
}

// Before retrieving the res dict, regenerate the appearance stream if needed,
// because AnnotLine::draw may need to store font info in the res dict
Object AnnotLine::getAppearanceResDict()
{
    if (appearance.isNull()) {
        generateLineAppearance();
    }
    return Annot::getAppearanceResDict();
}

//------------------------------------------------------------------------
// AnnotTextMarkup
//------------------------------------------------------------------------
AnnotTextMarkup::AnnotTextMarkup(PDFDoc *docA, PDFRectangle *rectA, AnnotSubtype subType) : AnnotMarkup(docA, rectA)
{
    switch (subType) {
    case typeHighlight:
        annotObj.dictSet("Subtype", Object(objName, "Highlight"));
        break;
    case typeUnderline:
        annotObj.dictSet("Subtype", Object(objName, "Underline"));
        break;
    case typeSquiggly:
        annotObj.dictSet("Subtype", Object(objName, "Squiggly"));
        break;
    case typeStrikeOut:
        annotObj.dictSet("Subtype", Object(objName, "StrikeOut"));
        break;
    default:
        assert(0 && "Invalid subtype for AnnotTextMarkup\n");
    }

    // Store dummy quadrilateral with null coordinates
    auto quadPoints = std::make_unique<Array>(doc->getXRef());
    for (int i = 0; i < 4 * 2; ++i) {
        quadPoints->add(Object(0.));
    }
    annotObj.dictSet("QuadPoints", Object(std::move(quadPoints)));

    initialize(annotObj.getDict());
}