// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 54/79



    // Let's init resourcesDictObj and resources.
    // In PDF 1.2, an additional entry in the field dictionary, DR, was defined.
    // Beginning with PDF 1.5, this entry is obsolete.
    // And yet Acrobat Reader seems to be taking a field's DR into account.
    Object resourcesDictObj;
    const GfxResources *resources = nullptr;
    std::unique_ptr<GfxResources> resourcesToFree = nullptr;
    if (field->getObj() && field->getObj()->isDict()) {
        // Let's use a field's resource dictionary.
        resourcesDictObj = field->getObj()->dictLookup("DR");
        if (resourcesDictObj.isDict()) {
            if (form && form->getDefaultResourcesObj()->isDict()) {
                resourcesDictObj = resourcesDictObj.deepCopy();
                recursiveMergeDicts(resourcesDictObj.getDict(), form->getDefaultResourcesObj()->getDict());
            }
            resourcesToFree = std::make_unique<GfxResources>(doc->getXRef(), resourcesDictObj.getDict(), nullptr);
            resources = resourcesToFree.get();
        }
    }
    if (!resourcesDictObj.isDict()) {
        // No luck with a field's resource dictionary. Let's use an AcroForm's resource dictionary.
        if (form && form->getDefaultResourcesObj()->isDict()) {
            resourcesDictObj = form->getDefaultResourcesObj()->deepCopy();
            resources = form->getDefaultResources();
        }
    }
    if (!resourcesDictObj.isDict()) {
        resourcesDictObj = Object(std::make_unique<Dict>(doc->getXRef()));
    }

    const bool success = appearBuilder.drawFormField(field, form, resources, da, border.get(), appearCharacs.get(), rect.get(), appearState.get(), doc->getXRef(), resourcesDictObj.getDict());
    if (!success && form && da != form->getDefaultAppearance()) {
        da = form->getDefaultAppearance();
        appearBuilder.drawFormField(field, form, resources, da, border.get(), appearCharacs.get(), rect.get(), appearState.get(), doc->getXRef(), resourcesDictObj.getDict());
    }

    if (addedDingbatsResource) {
        *addedDingbatsResource = appearBuilder.getAddedDingbatsResource();
    }

    const GooString *appearBuf = appearBuilder.buffer();
    // fill the appearance stream dictionary
    appearDict->add("Length", Object(int(appearBuf->size())));
    appearDict->add("Subtype", Object(objName, "Form"));
    auto bbox = std::make_unique<Array>(doc->getXRef());
    bbox->add(Object(0));
    bbox->add(Object(0));
    bbox->add(Object(rect->x2 - rect->x1));
    bbox->add(Object(rect->y2 - rect->y1));
    appearDict->add("BBox", Object(std::move(bbox)));

    // set the resource dictionary
    if (resourcesDictObj.getDict()->getLength() > 0) {
        appearDict->set("Resources", std::move(resourcesDictObj));
    }

    // build the appearance stream
    std::vector<char> data { appearBuf->c_str(), appearBuf->c_str() + appearBuf->size() };
    auto appearStream = std::make_unique<AutoFreeMemStream>(std::move(data), Object(std::move(appearDict)));
    if (hasBeenUpdated) {
        // We should technically do this for all annots but AnnotFreeText
        // forms are particularly special since we're potentially embeddeing a font so we really need
        // to set the AP and not let other renderers guess it from the contents
        // In addition, we keep the appearState of checkboxes to prevent them from being deselected
        bool keepAppearState = field->getType() == formButton && static_cast<const FormFieldButton *>(field)->getButtonType() == formButtonCheck;
        setNewAppearance(Object(std::move(appearStream)), keepAppearState);
    } else {
        appearance = Object(std::move(appearStream));
    }
}

void AnnotWidget::updateAppearanceStream()
{
    // If this the first time updateAppearanceStream() is called on this widget,
    // destroy the AP dictionary because we are going to create a new one.
    if (updatedAppearanceStream == Ref::INVALID()) {
        invalidateAppearance(); // Delete AP dictionary and all referenced streams
    }

    // There's no need to create a new appearance stream if NeedAppearances is
    // set, because it will be ignored next time anyway.
    // except if signature type; most readers can't figure out how to create an
    // appearance for those and thus renders nothing.
    if (form && form->getNeedAppearances()) {
        if (field->getType() != FormFieldType::formSignature) {
            return;
        }
    }

    // Create the new appearance
    generateFieldAppearance();

    // Fetch the appearance stream we've just created
    Object obj1 = appearance.fetch(doc->getXRef());

    // If this the first time updateAppearanceStream() is called on this widget,
    // create a new AP dictionary containing the new appearance stream.
    // Otherwise, just update the stream we had created previously.
    if (updatedAppearanceStream == Ref::INVALID()) {
        // Write the appearance stream
        updatedAppearanceStream = doc->getXRef()->addIndirectObject(obj1);