// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 57/79



void AnnotStamp::generateStampCustomAppearance()
{
    Ref imgRef = stampImageHelper->getRef();
    const std::string imgStrName = "X" + std::to_string(imgRef.num);

    AnnotAppearanceBuilder appearBuilder;
    appearBuilder.append("q\n");
    appearBuilder.append("/GS0 gs\n");
    appearBuilder.appendf("{0:.3f} 0 0 {1:.3f} 0 0 cm\n", rect->x2 - rect->x1, rect->y2 - rect->y1);
    appearBuilder.append("/");
    appearBuilder.append(imgStrName.c_str());
    appearBuilder.append(" Do\n");
    appearBuilder.append("Q\n");

    std::unique_ptr<Dict> resDict = createResourcesDict(imgStrName.c_str(), Object(imgRef), "GS0", opacity, nullptr);

    const std::array<double, 4> bboxArray = { 0, 0, rect->x2 - rect->x1, rect->y2 - rect->y1 };
    const GooString *appearBuf = appearBuilder.buffer();
    appearance = createForm(appearBuf, bboxArray, false, std::move(resDict));

    if (updatedAppearanceStream == Ref::INVALID()) {
        updatedAppearanceStream = doc->getXRef()->addIndirectObject(appearance);
    } else {
        Object obj1 = appearance.fetch(doc->getXRef());
        doc->getXRef()->setModifiedObject(&obj1, updatedAppearanceStream);
    }

    Object obj1 = Object(std::make_unique<Dict>(doc->getXRef()));
    obj1.dictAdd("N", Object(updatedAppearanceStream));
    update("AP", std::move(obj1));
}

void AnnotStamp::updateAppearanceResDict()
{
    if (appearance.isNull()) {
        if (stampImageHelper != nullptr) {
            generateStampCustomAppearance();
        } else {
            generateStampDefaultAppearance();
        }
    }
}

Object AnnotStamp::getAppearanceResDict()
{
    updateAppearanceResDict();

    return Annot::getAppearanceResDict();
}

void AnnotStamp::generateStampDefaultAppearance()
{
    std::unique_ptr<Dict> extGStateDict;
    AnnotAppearanceBuilder defaultAppearanceBuilder;