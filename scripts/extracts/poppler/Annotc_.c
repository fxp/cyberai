// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 74/79

                                                             \
    "0.729412 0.741176 0.713725 RG 12 21 m 12 21 l 13.656 21 15 19.656 15 18 c\n"                                                                                                                                                              \
    "15 14 l 15 12.344 13.656 11 12 11 c 12 11 l 10.344 11 9 12.344 9 14 c\n"                                                                                                                                                                  \
    "9 18 l 9 19.656 10.344 21 12 21 c h\n"                                                                                                                                                                                                    \
    "12 21 m S\n"                                                                                                                                                                                                                              \
    "1 w\n"                                                                                                                                                                                                                                    \
    "17.5 15.5 m 17.5 12.973 l 17.5 9.941 15.047 7.5 12 7.5 c 8.953 7.5 6.5\n"                                                                                                                                                                 \
    "9.941 6.5 12.973 c 6.5 15.5 l S\n"                                                                                                                                                                                                        \
    "2 w\n"                                                                                                                                                                                                                                    \
    "0 J\n"                                                                                                                                                                                                                                    \
    "12 7.52 m 12 4 l S\n"                                                                                                                                                                                                                     \
    "1 J\n"                                                                                                                                                                                                                                    \
    "8 4 m 16 4 l S\n"

void AnnotSound::draw(Gfx *gfx, bool printing)
{
    Object obj;
    double ca = 1;

    if (!isVisible(printing)) {
        return;
    }

    annotLocker();
    if (appearance.isNull()) {
        ca = opacity;

        AnnotAppearanceBuilder appearBuilder;

        appearBuilder.append("q\n");
        if (color) {
            appearBuilder.setDrawColor(*color, true);
        } else {
            appearBuilder.append("1 1 1 rg\n");
        }
        if (!iconName->compare("Speaker")) {
            appearBuilder.append(ANNOT_SOUND_AP_SPEAKER);
        } else if (!iconName->compare("Mic")) {
            appearBuilder.append(ANNOT_SOUND_AP_MIC);
        }
        appearBuilder.append("Q\n");

        static constexpr std::array<double, 4> bbox = { 0, 0, 24, 24 };
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
    obj = appearance.fetch(gfx->getXRef());
    gfx->drawAnnot(&obj, nullptr, color.get(), rect->x1, rect->y1, rect->x2, rect->y2, getRotation());
}

//------------------------------------------------------------------------
// Annot3D
//------------------------------------------------------------------------
Annot3D::Annot3D(PDFDoc *docA, PDFRectangle *rectA) : Annot(docA, rectA)
{
    type = type3D;

    annotObj.dictSet("Subtype", Object(objName, "3D"));

    initialize(annotObj.getDict());
}

Annot3D::Annot3D(PDFDoc *docA, Object &&dictObject, const Object *obj) : Annot(docA, std::move(dictObject), obj)
{
    type = type3D;
    initialize(annotObj.getDict());
}

Annot3D::~Annot3D() = default;

void Annot3D::initialize(Dict *dict)
{
    Object obj1 = dict->lookup("3DA");
    if (obj1.isDict()) {
        activation = std::make_unique<Activation>(obj1.getDict());
    }
}

Annot3D::Activation::Activation(Dict *dict)
{
    Object obj1;