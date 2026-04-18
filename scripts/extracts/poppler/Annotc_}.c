// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 61/79



    switch (new_type) {
    case typePolygon:
        typeName = "Polygon";
        break;
    case typePolyLine:
        typeName = "PolyLine";
        break;
    default:
        assert(!"Invalid subtype");
    }

    type = new_type;
    update("Subtype", Object(objName, typeName));
    invalidateAppearance();
}

void AnnotPolygon::setVertices(const AnnotPath &path)
{
    auto a = std::make_unique<Array>(doc->getXRef());
    for (int i = 0; i < path.getCoordsLength(); i++) {
        a->add(Object(path.getX(i)));
        a->add(Object(path.getY(i)));
    }

    vertices = std::make_unique<AnnotPath>(*a);

    update("Vertices", Object(std::move(a)));
    invalidateAppearance();
}

void AnnotPolygon::setStartEndStyle(AnnotLineEndingStyle start, AnnotLineEndingStyle end)
{
    startStyle = start;
    endStyle = end;

    auto a = std::make_unique<Array>(doc->getXRef());
    a->add(Object(objName, convertAnnotLineEndingStyle(startStyle)));
    a->add(Object(objName, convertAnnotLineEndingStyle(endStyle)));

    update("LE", Object(std::move(a)));
    invalidateAppearance();
}

void AnnotPolygon::setInteriorColor(std::unique_ptr<AnnotColor> &&new_color)
{
    if (new_color) {
        Object obj1 = new_color->writeToObject(doc->getXRef());
        update("IC", std::move(obj1));
        interiorColor = std::move(new_color);
    } else {
        interiorColor = nullptr;
        update("IC", Object::null());
    }
    invalidateAppearance();
}

void AnnotPolygon::setIntent(AnnotPolygonIntent new_intent)
{
    const char *intentName;

    intent = new_intent;
    if (new_intent == polygonCloud) {
        intentName = "PolygonCloud";
    } else if (new_intent == polylineDimension) {
        intentName = "PolyLineDimension";
    } else { // polygonDimension
        intentName = "PolygonDimension";
    }
    update("IT", Object(objName, intentName));
}

void AnnotPolygon::generatePolyLineAppearance(AnnotAppearanceBuilder *appearBuilder)
{
    const bool fill = (bool)interiorColor;
    const double x1 = vertices->getX(0);
    const double y1 = vertices->getY(0);
    const double x2 = vertices->getX(1);
    const double y2 = vertices->getY(1);
    const double x3 = vertices->getX(vertices->getCoordsLength() - 2);
    const double y3 = vertices->getY(vertices->getCoordsLength() - 2);
    const double x4 = vertices->getX(vertices->getCoordsLength() - 1);
    const double y4 = vertices->getY(vertices->getCoordsLength() - 1);

    const double len_1 = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    // length of last segment
    const double len_2 = sqrt((x4 - x3) * (x4 - x3) + (y4 - y3) * (y4 - y3));

    // segments become positive x direction, coord1 becomes (0,0).
    Matrix matr1, matr2;
    const double angle1 = atan2(y2 - y1, x2 - x1);
    const double angle2 = atan2(y4 - y3, x4 - x3);

    matr1.m[0] = matr1.m[3] = cos(angle1);
    matr1.m[1] = sin(angle1);
    matr1.m[2] = -matr1.m[1];
    matr1.m[4] = x1 - rect->x1;
    matr1.m[5] = y1 - rect->y1;

    matr2.m[0] = matr2.m[3] = cos(angle2);
    matr2.m[1] = sin(angle2);
    matr2.m[2] = -matr2.m[1];
    matr2.m[4] = x3 - rect->x1;
    matr2.m[5] = y3 - rect->y1;

    const double lineEndingSize1 { std::min(6. * border->getWidth(), len_1 / 2) };
    const double lineEndingSize2 { std::min(6. * border->getWidth(), len_2 / 2) };

    if (vertices->getCoordsLength() != 0) {
        double tx, ty;
        matr1.transform(AnnotAppearanceBuilder::lineEndingXShorten(startStyle, lineEndingSize1), 0, &tx, &ty);
        appearBuilder->appendf("{0:.2f} {1:.2f} m\n", tx, ty);
        appearBBox->extendTo(tx, ty);

        for (int i = 1; i < vertices->getCoordsLength() - 1; ++i) {
            appearBuilder->appendf("{0:.2f} {1:.2f} l\n", vertices->getX(i) - rect->x1, vertices->getY(i) - rect->y1);
            appearBBox->extendTo(vertices->getX(i) - rect->x1, vertices->getY(i) - rect->y1);
        }

        if (vertices->getCoordsLength() > 1) {
            matr2.transform(len_2 - AnnotAppearanceBuilder::lineEndingXShorten(endStyle, lineEndingSize2), 0, &tx, &ty);
            appearBuilder->appendf("{0:.2f} {1:.2f} l S\n", tx, ty);
            appearBBox->extendTo(tx, ty);
        }
    }

    if (startStyle != annotLineEndingNone) {
        const double extendX { -AnnotAppearanceBuilder::lineEndingXExtendBBox(startStyle, lineEndingSize1) };
        double tx, ty;
        appearBuilder->drawLineEnding(startStyle, 0, 0, -lineEndingSize1, fill, matr1);
        matr1.transform(extendX, lineEndingSize1 / 2., &tx, &ty);
        appearBBox->extendTo(tx, ty);
        matr1.transform(extendX, -lineEndingSize1 / 2., &tx, &ty);
        appearBBox->extendTo(tx, ty);
    }