// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 32/79



        appearBuilder.append("q\n");
        if (color) {
            appearBuilder.setDrawColor(*color, true);
        } else {
            appearBuilder.append("1 1 1 rg\n");
        }
        if (icon == "Note") {
            appearBuilder.append(ANNOT_TEXT_AP_NOTE);
        } else if (icon == "Comment") {
            appearBuilder.append(ANNOT_TEXT_AP_COMMENT);
        } else if (icon == "Key") {
            appearBuilder.append(ANNOT_TEXT_AP_KEY);
        } else if (icon == "Help") {
            appearBuilder.append(ANNOT_TEXT_AP_HELP);
        } else if (icon == "NewParagraph") {
            appearBuilder.append(ANNOT_TEXT_AP_NEW_PARAGRAPH);
        } else if (icon == "Paragraph") {
            appearBuilder.append(ANNOT_TEXT_AP_PARAGRAPH);
        } else if (icon == "Insert") {
            appearBuilder.append(ANNOT_TEXT_AP_INSERT);
        } else if (icon == "Cross") {
            appearBuilder.append(ANNOT_TEXT_AP_CROSS);
        } else if (icon == "Circle") {
            appearBuilder.append(ANNOT_TEXT_AP_CIRCLE);
        } else if (icon == "Check") {
            appearBuilder.append(ANNOT_TEXT_AP_CHECK);
        } else if (icon == "Star") {
            appearBuilder.append(ANNOT_TEXT_AP_STAR);
        } else if (icon == "RightArrow") {
            appearBuilder.append(ANNOT_TEXT_AP_RIGHT_ARROW);
        } else if (icon == "RightPointer") {
            appearBuilder.append(ANNOT_TEXT_AP_RIGHT_POINTER);
        } else if (icon == "UpArrow") {
            appearBuilder.append(ANNOT_TEXT_AP_UP_ARROW);
        } else if (icon == "UpLeftArrow") {
            appearBuilder.append(ANNOT_TEXT_AP_UP_LEFT_ARROW);
        } else if (icon == "CrossHairs") {
            appearBuilder.append(ANNOT_TEXT_AP_CROSS_HAIRS);
        }
        appearBuilder.append("Q\n");

        // Force 24x24 rectangle
        PDFRectangle fixedRect(rect->x1, rect->y2 - 24, rect->x1 + 24, rect->y2);
        appearBBox = std::make_unique<AnnotAppearanceBBox>(&fixedRect);
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
// AnnotLink
//------------------------------------------------------------------------
AnnotLink::AnnotLink(PDFDoc *docA, PDFRectangle *rectA) : Annot(docA, rectA)
{
    type = typeLink;
    annotObj.dictSet("Subtype", Object(objName, "Link"));
    initialize(annotObj.getDict());
}

AnnotLink::AnnotLink(PDFDoc *docA, Object &&dictObject, const Object *obj) : Annot(docA, std::move(dictObject), obj)
{

    type = typeLink;
    initialize(annotObj.getDict());
}

AnnotLink::~AnnotLink() = default;

void AnnotLink::initialize(Dict *dict)
{
    Object obj1;

    // look for destination
    obj1 = dict->lookup("Dest");
    if (!obj1.isNull()) {
        action = LinkAction::parseDest(&obj1);
        // look for action
    } else {
        obj1 = dict->lookup("A");
        if (obj1.isDict()) {
            action = LinkAction::parseAction(&obj1, doc->getCatalog()->getBaseURI());
        }
    }

    obj1 = dict->lookup("H");
    if (obj1.isName()) {
        const char *effect = obj1.getName();

        if (!strcmp(effect, "N")) {
            linkEffect = effectNone;
        } else if (!strcmp(effect, "I")) {
            linkEffect = effectInvert;
        } else if (!strcmp(effect, "O")) {
            linkEffect = effectOutline;
        } else if (!strcmp(effect, "P")) {
            linkEffect = effectPush;
        } else {
            linkEffect = effectInvert;
        }
    } else {
        linkEffect = effectInvert;
    }
    /*
    obj1 = dict->lookup("PA");
    if (obj1.isDict()) {
      uriAction = NULL;
    } else {
      uriAction = NULL;
    }
    obj1.free();
    */
    obj1 = dict->lookup("QuadPoints");
    if (obj1.isArray()) {
        quadrilaterals = std::make_unique<AnnotQuadrilaterals>(*obj1.getArray());
    }

    obj1 = dict->lookup("BS");
    if (obj1.isDict()) {
        border = std::make_unique<AnnotBorderBS>(obj1.getDict());
    } else if (!border) {
        border = std::make_unique<AnnotBorderBS>();
    }
}