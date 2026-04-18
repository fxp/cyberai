// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 60/79



        if (type == typeSquare) {
            appearBuilder.appendf("{0:.2f} {1:.2f} {2:.2f} {3:.2f} re\n", borderWidth / 2.0, borderWidth / 2.0, (rect->x2 - rect->x1) - borderWidth, (rect->y2 - rect->y1) - borderWidth);
            if (fill) {
                if (borderWidth > 0) {
                    appearBuilder.append("b\n");
                } else {
                    appearBuilder.append("f\n");
                }
            } else if (borderWidth > 0) {
                appearBuilder.append("S\n");
            }
        } else {
            const double rx { (rect->x2 - rect->x1) / 2. };
            const double ry { (rect->y2 - rect->y1) / 2. };
            const double bwHalf { borderWidth / 2.0 };
            appearBuilder.drawEllipse(rx, ry, rx - bwHalf, ry - bwHalf, fill, borderWidth > 0);
        }
        appearBuilder.append("Q\n");

        const std::array<double, 4> bbox = { 0, 0, rect->x2 - rect->x1, rect->y2 - rect->y1 };
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
    gfx->drawAnnot(&obj, nullptr, color.get(), rect->x1, rect->y1, rect->x2, rect->y2, getRotation());
}

//------------------------------------------------------------------------
// AnnotPolygon
//------------------------------------------------------------------------
AnnotPolygon::AnnotPolygon(PDFDoc *docA, PDFRectangle *rectA, AnnotSubtype subType) : AnnotMarkup(docA, rectA)
{
    switch (subType) {
    case typePolygon:
        annotObj.dictSet("Subtype", Object(objName, "Polygon"));
        break;
    case typePolyLine:
        annotObj.dictSet("Subtype", Object(objName, "PolyLine"));
        break;
    default:
        assert(0 && "Invalid subtype for AnnotGeometry\n");
    }

    // Store dummy path with one null vertex only
    auto a = std::make_unique<Array>(doc->getXRef());
    a->add(Object(0.));
    a->add(Object(0.));
    annotObj.dictSet("Vertices", Object(std::move(a)));

    initialize(annotObj.getDict());
}

AnnotPolygon::AnnotPolygon(PDFDoc *docA, Object &&dictObject, const Object *obj) : AnnotMarkup(docA, std::move(dictObject), obj)
{
    // the real type will be read in initialize()
    type = typePolygon;
    initialize(annotObj.getDict());
}

AnnotPolygon::~AnnotPolygon() = default;

void AnnotPolygon::initialize(Dict *dict)
{
    Object obj1;

    obj1 = dict->lookup("Subtype");
    if (obj1.isName()) {
        if (obj1.isName("Polygon")) {
            type = typePolygon;
        } else if (obj1.isName("PolyLine")) {
            type = typePolyLine;
        }
    }

    obj1 = dict->lookup("Vertices");
    if (obj1.isArray()) {
        vertices = std::make_unique<AnnotPath>(*obj1.getArray());
    } else {
        vertices = std::make_unique<AnnotPath>();
        error(errSyntaxError, -1, "Bad Annot Polygon Vertices");
        ok = false;
    }

    obj1 = dict->lookup("LE");
    if (obj1.isArrayOfLength(2)) {
        Object obj2 = obj1.arrayGet(0);
        if (obj2.isName()) {
            startStyle = parseAnnotLineEndingStyle(obj2);
        } else {
            startStyle = annotLineEndingNone;
        }
        obj2 = obj1.arrayGet(1);
        if (obj2.isName()) {
            endStyle = parseAnnotLineEndingStyle(obj2);
        } else {
            endStyle = annotLineEndingNone;
        }
    } else {
        startStyle = endStyle = annotLineEndingNone;
    }

    obj1 = dict->lookup("IC");
    if (obj1.isArray()) {
        interiorColor = std::make_unique<AnnotColor>(*obj1.getArray());
    }

    obj1 = dict->lookup("BS");
    if (obj1.isDict()) {
        border = std::make_unique<AnnotBorderBS>(obj1.getDict());
    } else if (!border) {
        border = std::make_unique<AnnotBorderBS>();
    }

    obj1 = dict->lookup("BE");
    if (obj1.isDict()) {
        borderEffect = std::make_unique<AnnotBorderEffect>(obj1.getDict());
    }

    obj1 = dict->lookup("IT");
    if (obj1.isName()) {
        const char *intentName = obj1.getName();

        if (!strcmp(intentName, "PolygonCloud")) {
            intent = polygonCloud;
        } else if (!strcmp(intentName, "PolyLineDimension")) {
            intent = polylineDimension;
        } else {
            intent = polygonDimension;
        }
    } else {
        intent = polygonCloud;
    }
}

void AnnotPolygon::setType(AnnotSubtype new_type)
{
    const char *typeName = nullptr; /* squelch bogus compiler warning */