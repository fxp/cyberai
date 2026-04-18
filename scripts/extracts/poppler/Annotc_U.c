// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 53/79



    double yDelta = height - textmargin;
    if (centerVertically) {
        const double outTextHeight = textCommands.nLines * da.getFontPtSize();
        if (outTextHeight < height) {
            yDelta = height - (height - outTextHeight) / 2;
        }
    }
    appendf("BT 1 0 0 1 {0:.2f} {1:.2f} Tm\n", leftMargin + textmargin, yDelta);
    append(textCommands.text.c_str());
    append("ET Q\n");
}

bool AnnotAppearanceBuilder::drawFormFieldChoice(const FormFieldChoice *fieldChoice, const Form *form, const GfxResources *resources, const GooString *da, const AnnotBorder *border, const AnnotAppearanceCharacs *appearCharacs,
                                                 const PDFRectangle *rect, XRef *xref, Dict *resourcesDict)
{
    const GooString *selected;
    VariableTextQuadding quadding;

    if (fieldChoice->hasTextQuadding()) {
        quadding = fieldChoice->getTextQuadding();
    } else if (form) {
        quadding = form->getTextQuadding();
    } else {
        quadding = VariableTextQuadding::leftJustified;
    }

    if (fieldChoice->isCombo()) {
        selected = fieldChoice->getAppearanceSelectedChoice();
        if (selected) {
            return drawText(selected, form, da, resources, border, appearCharacs, rect, quadding, xref, resourcesDict, EmitMarkedContentDrawTextFlag);
            //~ Acrobat draws a popup icon on the right side
        }
        // list box
    } else {
        return drawListBox(fieldChoice, border, rect, da, resources, quadding, xref, resourcesDict);
    }

    return true;
}

// Should we also merge Arrays?
static void recursiveMergeDicts(Dict *primary, const Dict *secondary, RefRecursionChecker *alreadySeenDicts)
{
    for (int i = 0; i < secondary->getLength(); ++i) {
        const char *key = secondary->getKey(i);
        if (!primary->hasKey(key)) {
            primary->add(key, secondary->lookup(key).deepCopy());
        } else {
            Ref primaryRef;
            Object primaryObj = primary->lookup(key, &primaryRef);
            if (primaryObj.isDict()) {
                Ref secondaryRef;
                Object secondaryObj = secondary->lookup(key, &secondaryRef);
                if (secondaryObj.isDict()) {
                    if (!alreadySeenDicts->insert(primaryRef) || !alreadySeenDicts->insert(secondaryRef)) {
                        // bad PDF
                        return;
                    }
                    recursiveMergeDicts(primaryObj.getDict(), secondaryObj.getDict(), alreadySeenDicts);
                }
            }
        }
    }
}

static void recursiveMergeDicts(Dict *primary, const Dict *secondary)
{
    RefRecursionChecker alreadySeenDicts;
    recursiveMergeDicts(primary, secondary, &alreadySeenDicts);
}

void AnnotWidget::generateFieldAppearance(bool *addedDingbatsResource)
{
    const GooString *da;

    AnnotAppearanceBuilder appearBuilder;

    // draw the background
    if (appearCharacs) {
        const AnnotColor *aColor = appearCharacs->getBackColor();
        if (aColor) {
            appearBuilder.setDrawColor(*aColor, true);
            appearBuilder.appendf("0 0 {0:.2f} {1:.2f} re f\n", rect->x2 - rect->x1, rect->y2 - rect->y1);
        }
    }

    // draw the border
    if (appearCharacs && border && border->getWidth() > 0) {
        appearBuilder.drawFieldBorder(field, border.get(), appearCharacs.get(), rect.get());
    }

    da = field->getDefaultAppearance();
    if (!da && form) {
        da = form->getDefaultAppearance();
    }

    auto appearDict = std::make_unique<Dict>(doc->getXRef());