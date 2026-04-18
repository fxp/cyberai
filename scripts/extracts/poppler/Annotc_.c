// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 64/79



    appearBuilder.append("Q\n");

    const std::array<double, 4> bbox = appearBBox->getBBoxRect();

    if (opacity == 1 && !drawBelow) {
        newAppearance = createForm(appearBuilder.buffer(), bbox, false, Object {});
    } else {
        std::unique_ptr<Dict> resDict = createResourcesDict(nullptr, Object::null(), "GS0", opacity, drawBelow ? "Multiply" : nullptr);
        newAppearance = createForm(appearBuilder.buffer(), bbox, false, std::move(resDict));
    }

    /* If the annotation is drawn below (highlighting), we must save the
    appearance stream so the multiply blend mode can be saved */
    if (drawBelow) {
        setNewAppearance(std::move(newAppearance));
    } else {
        appearance = std::move(newAppearance);
    }
}

void AnnotInk::draw(Gfx *gfx, bool printing)
{
    if (!isVisible(printing)) {
        return;
    }

    annotLocker();
    if (appearance.isNull()) {
        generateInkAppearance();
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
// AnnotFileAttachment
//------------------------------------------------------------------------
AnnotFileAttachment::AnnotFileAttachment(PDFDoc *docA, PDFRectangle *rectA, GooString *filename) : AnnotMarkup(docA, rectA)
{
    type = typeFileAttachment;

    annotObj.dictSet("Subtype", Object(objName, "FileAttachment"));
    annotObj.dictSet("FS", Object(filename->copy()));

    initialize(annotObj.getDict());
}

AnnotFileAttachment::AnnotFileAttachment(PDFDoc *docA, Object &&dictObject, const Object *obj) : AnnotMarkup(docA, std::move(dictObject), obj)
{
    type = typeFileAttachment;
    initialize(annotObj.getDict());
}

AnnotFileAttachment::~AnnotFileAttachment() = default;

void AnnotFileAttachment::initialize(Dict *dict)
{
    Object objFS = dict->lookup("FS");
    if (objFS.isDict() || objFS.isString()) {
        file = std::move(objFS);
    } else {
        error(errSyntaxError, -1, "Bad Annot File Attachment");
        ok = false;
    }

    Object objName = dict->lookup("Name");
    if (objName.isName()) {
        iconName = std::make_unique<GooString>(objName.getNameString());
    } else {
        iconName = std::make_unique<GooString>("PushPin");
    }
}