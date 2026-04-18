// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 70/79

                                                                                                                          \
    "0 j\n"                                                                                                                                                                                                                                    \
    "6.93 16.141 m 8 21 14.27 21.5 16 21.5 c 18.094 21.504 19.5 21 19.5 19 c\n"                                                                                                                                                                \
    "19.5 17.699 20.91 17.418 22.5 17.5 c S\n"

void AnnotFileAttachment::draw(Gfx *gfx, bool printing)
{
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
        if (!iconName->compare("PushPin")) {
            appearBuilder.append(ANNOT_FILE_ATTACHMENT_AP_PUSHPIN);
        } else if (!iconName->compare("Paperclip")) {
            appearBuilder.append(ANNOT_FILE_ATTACHMENT_AP_PAPERCLIP);
        } else if (!iconName->compare("Graph")) {
            appearBuilder.append(ANNOT_FILE_ATTACHMENT_AP_GRAPH);
        } else if (!iconName->compare("Tag")) {
            appearBuilder.append(ANNOT_FILE_ATTACHMENT_AP_TAG);
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
    Object obj = appearance.fetch(gfx->getXRef());
    gfx->drawAnnot(&obj, nullptr, color.get(), rect->x1, rect->y1, rect->x2, rect->y2, getRotation());
}

//------------------------------------------------------------------------
// AnnotSound
//------------------------------------------------------------------------
AnnotSound::AnnotSound(PDFDoc *docA, PDFRectangle *rectA, Sound *soundA) : AnnotMarkup(docA, rectA)
{
    type = typeSound;

    annotObj.dictSet("Subtype", Object(objName, "Sound"));
    annotObj.dictSet("Sound", soundA->getObject().copy());

    initialize(annotObj.getDict());
}

AnnotSound::AnnotSound(PDFDoc *docA, Object &&dictObject, const Object *obj) : AnnotMarkup(docA, std::move(dictObject), obj)
{
    type = typeSound;
    initialize(annotObj.getDict());
}

AnnotSound::~AnnotSound() = default;

void AnnotSound::initialize(Dict *dict)
{
    Object obj1 = dict->lookup("Sound");

    sound = Sound::parseSound(obj1);
    if (!sound) {
        error(errSyntaxError, -1, "Bad Annot Sound");
        ok = false;
    }

    obj1 = dict->lookup("Name");
    if (obj1.isName()) {
        iconName = std::make_unique<GooString>(obj1.getName());
    } else {
        iconName = std::make_unique<GooString>("Speaker");
    }
}