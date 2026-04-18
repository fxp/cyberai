// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 62/79



    if (endStyle != annotLineEndingNone) {
        const double extendX { AnnotAppearanceBuilder::lineEndingXExtendBBox(endStyle, lineEndingSize2) };
        double tx, ty;
        appearBuilder->drawLineEnding(endStyle, len_2, 0, lineEndingSize2, fill, matr2);
        matr2.transform(len_2 + extendX, lineEndingSize2 / 2., &tx, &ty);
        appearBBox->extendTo(tx, ty);
        matr2.transform(len_2 + extendX, -lineEndingSize2 / 2., &tx, &ty);
        appearBBox->extendTo(tx, ty);
    }
}

void AnnotPolygon::draw(Gfx *gfx, bool printing)
{
    double ca = 1;

    if (!isVisible(printing)) {
        return;
    }

    annotLocker();
    if (appearance.isNull()) {
        appearBBox = std::make_unique<AnnotAppearanceBBox>(rect.get());
        ca = opacity;

        AnnotAppearanceBuilder appearBuilder;
        appearBuilder.append("q\n");

        if (color) {
            appearBuilder.setDrawColor(*color, false);
        }

        appearBuilder.setLineStyleForBorder(*border);
        appearBBox->setBorderWidth(std::max(1., border->getWidth()));

        if (interiorColor) {
            appearBuilder.setDrawColor(*interiorColor, true);
        }

        if (type == typePolyLine) {
            generatePolyLineAppearance(&appearBuilder);
        } else {
            if (vertices->getCoordsLength() != 0) {
                appearBuilder.appendf("{0:.2f} {1:.2f} m\n", vertices->getX(0) - rect->x1, vertices->getY(0) - rect->y1);
                appearBBox->extendTo(vertices->getX(0) - rect->x1, vertices->getY(0) - rect->y1);

                for (int i = 1; i < vertices->getCoordsLength(); ++i) {
                    appearBuilder.appendf("{0:.2f} {1:.2f} l\n", vertices->getX(i) - rect->x1, vertices->getY(i) - rect->y1);
                    appearBBox->extendTo(vertices->getX(i) - rect->x1, vertices->getY(i) - rect->y1);
                }

                const double borderWidth = border->getWidth();
                if (interiorColor && interiorColor->getSpace() != AnnotColor::colorTransparent) {
                    if (borderWidth > 0) {
                        appearBuilder.append("b\n");
                    } else {
                        appearBuilder.append("f\n");
                    }
                } else if (borderWidth > 0) {
                    appearBuilder.append("s\n");
                }
            }
        }
        appearBuilder.append("Q\n");

        const std::array<double, 4> bbox = appearBBox->getBBoxRect();
        if (ca == 1) {
            appearance = createForm(appearBuilder.buffer(), bbox, false, Object {});
        } else {
            Object aStream = createForm(appearBuilder.buffer(), bbox, true, Object {});

            GooString appearBuf("/GS0 gs\n/Fm0 Do");
            std::unique_ptr<Dict> resDict = createResourcesDict("Fm0", std::move(aStream), "GS0", ca, nullptr);
            appearance = createForm(&appearBuf, bbox, false, std::move(resDict));
        }
    }

    // draw the appearance stream
    Object obj = appearance.fetch(gfx->getXRef());
    if (appearBBox) {
        gfx->drawAnnot(&obj, nullptr, color.get(), appearBBox->getPageXMin(), appearBBox->getPageYMin(), appearBBox->getPageXMax(), appearBBox->getPageYMax(), getRotation());
    } else {
        gfx->drawAnnot(&obj, nullptr, color.get(), rect->x1, rect->y1, rect->x2, rect->y2, getRotation());
    }
}

//------------------------------------------------------------------------
// AnnotCaret
//------------------------------------------------------------------------
AnnotCaret::AnnotCaret(PDFDoc *docA, PDFRectangle *rectA) : AnnotMarkup(docA, rectA)
{
    type = typeCaret;

    annotObj.dictSet("Subtype", Object(objName, "Caret"));
    initialize(annotObj.getDict());
}

AnnotCaret::AnnotCaret(PDFDoc *docA, Object &&dictObject, const Object *obj) : AnnotMarkup(docA, std::move(dictObject), obj)
{
    type = typeCaret;
    initialize(annotObj.getDict());
}

AnnotCaret::~AnnotCaret() = default;

void AnnotCaret::initialize(Dict *dict)
{
    Object obj1;

    symbol = symbolNone;
    obj1 = dict->lookup("Sy");
    if (obj1.isName()) {
        if (obj1.isName("P")) {
            symbol = symbolP;
        } else if (obj1.isName("None")) {
            symbol = symbolNone;
        }
    }

    obj1 = dict->lookup("RD");
    if (obj1.isArray()) {
        caretRect = parseDiffRectangle(obj1.getArray(), rect.get());
    }
}

void AnnotCaret::setSymbol(AnnotCaretSymbol new_symbol)
{
    symbol = new_symbol;
    update("Sy", Object(objName, new_symbol == symbolP ? "P" : "None"));
    invalidateAppearance();
}

//------------------------------------------------------------------------
// AnnotInk
//------------------------------------------------------------------------
AnnotInk::AnnotInk(PDFDoc *docA, PDFRectangle *rectA) : AnnotMarkup(docA, rectA)
{
    type = typeInk;

    annotObj.dictSet("Subtype", Object(objName, "Ink"));