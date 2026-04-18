// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 51/79



bool AnnotAppearanceBuilder::drawFormField(const FormField *field, const Form *form, const GfxResources *resources, const GooString *da, const AnnotBorder *border, const AnnotAppearanceCharacs *appearCharacs, const PDFRectangle *rect,
                                           const GooString *appearState, XRef *xref, Dict *resourcesDict)
{
    // draw the field contents
    switch (field->getType()) {
    case formButton:
        return drawFormFieldButton(static_cast<const FormFieldButton *>(field), form, resources, da, border, appearCharacs, rect, appearState, xref, resourcesDict);
        break;
    case formText:
        return drawFormFieldText(static_cast<const FormFieldText *>(field), form, resources, da, border, appearCharacs, rect, xref, resourcesDict);
    case formChoice:
        return drawFormFieldChoice(static_cast<const FormFieldChoice *>(field), form, resources, da, border, appearCharacs, rect, xref, resourcesDict);
        break;
    case formSignature:
        return drawSignatureFieldText(static_cast<const FormFieldSignature *>(field), form, da, border, rect, xref, resourcesDict);
        break;
    case formUndef:
    default:
        error(errSyntaxError, -1, "Unknown field type");
    }

    return false;
}

bool AnnotAppearanceBuilder::drawFormFieldButton(const FormFieldButton *field, const Form *form, const GfxResources *resources, const GooString *da, const AnnotBorder *border, const AnnotAppearanceCharacs *appearCharacs,
                                                 const PDFRectangle *rect, const GooString *appearState, XRef *xref, Dict *resourcesDict)
{
    const GooString *caption = nullptr;
    if (appearCharacs) {
        caption = appearCharacs->getNormalCaption();
    }

    switch (field->getButtonType()) {
    case formButtonRadio: {
        //~ Acrobat doesn't draw a caption if there is no AP dict (?)
        if (appearState && appearState->compare("Off") != 0 && field->getState(appearState->c_str())) {
            if (caption) {
                return drawText(caption, form, da, resources, border, appearCharacs, rect, VariableTextQuadding::centered, xref, resourcesDict, ForceZapfDingbatsDrawTextFlag);
            }
            if (appearCharacs) {
                const AnnotColor *aColor = appearCharacs->getBorderColor();
                if (aColor) {
                    const double dx = rect->x2 - rect->x1;
                    const double dy = rect->y2 - rect->y1;
                    setDrawColor(*aColor, true);
                    drawCircle(0.5 * dx, 0.5 * dy, 0.2 * (dx < dy ? dx : dy), true);
                }
                return true;
            }
        }
    } break;
    case formButtonPush:
        if (caption) {
            return drawText(caption, form, da, resources, border, appearCharacs, rect, VariableTextQuadding::centered, xref, resourcesDict);
        }
        break;
    case formButtonCheck:
        if (appearState && appearState->compare("Off") != 0) {
            if (!caption) {
                GooString checkMark("3");
                return drawText(&checkMark, form, da, resources, border, appearCharacs, rect, VariableTextQuadding::centered, xref, resourcesDict, ForceZapfDingbatsDrawTextFlag);
            }
            return drawText(caption, form, da, resources, border, appearCharacs, rect, VariableTextQuadding::centered, xref, resourcesDict, ForceZapfDingbatsDrawTextFlag);
        }
        break;
    }

    return true;
}

bool AnnotAppearanceBuilder::drawFormFieldText(const FormFieldText *fieldText, const Form *form, const GfxResources *resources, const GooString *da, const AnnotBorder *border, const AnnotAppearanceCharacs *appearCharacs,
                                               const PDFRectangle *rect, XRef *xref, Dict *resourcesDict)
{
    VariableTextQuadding quadding;
    const GooString *contents;

    contents = fieldText->getAppearanceContent();
    if (contents) {
        if (fieldText->hasTextQuadding()) {
            quadding = fieldText->getTextQuadding();
        } else if (form) {
            quadding = form->getTextQuadding();
        } else {
            quadding = VariableTextQuadding::leftJustified;
        }

        const int nCombs = fieldText->isComb() ? fieldText->getMaxLen() : 0;

        int flags = EmitMarkedContentDrawTextFlag;
        if (fieldText->isMultiline()) {
            flags = flags | MultilineDrawTextFlag;
        }
        if (fieldText->isPassword()) {
            flags = flags | TurnTextToStarsDrawTextFlag;
        }
        return drawText(contents, form, da, resources, border, appearCharacs, rect, quadding, xref, resourcesDict, flags, nCombs);
    }

    return true;
}