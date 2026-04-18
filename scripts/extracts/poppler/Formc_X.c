// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 24/24



                if (field.ends_with(" R") && sscanf(field.c_str(), "%d %d R", &fieldRef.num, &fieldRef.gen) == 2) {
                    foundField = findFieldByRef(fieldRef);
                } else {
                    foundField = findFieldByFullyQualifiedName(field);
                }

                if (foundField) {
                    foundField->reset(std::vector<std::string>());
                }
            }
        } else {
            for (auto &rootField : rootFields) {
                rootField->reset(fields);
            }
        }
    }
}

std::string Form::findPdfFontNameToUseForSigning()
{
    static constexpr std::array<const char *, 2> fontsToUseToSign = { "Helvetica", "Arial" };
    for (const char *fontToUseToSign : fontsToUseToSign) {
        std::string pdfFontName = findFontInDefaultResources(fontToUseToSign, "");
        if (!pdfFontName.empty()) {
            return pdfFontName;
        }

        pdfFontName = addFontToDefaultResources(fontToUseToSign, "").fontName;
        if (!pdfFontName.empty()) {
            return pdfFontName;
        }
    }

    error(errInternal, -1, "Form::findPdfFontNameToUseForSigning: No suitable font found'");

    return {};
}

//------------------------------------------------------------------------
// FormPageWidgets
//------------------------------------------------------------------------

FormPageWidgets::FormPageWidgets(Annots *annots, unsigned int page, Form *form)
{
    if (annots && !annots->getAnnots().empty() && form) {

        /* For each entry in the page 'Annots' dict, try to find
           a matching form field */
        for (const std::shared_ptr<Annot> &annot : annots->getAnnots()) {

            if (annot->getType() != Annot::typeWidget) {
                continue;
            }

            if (!annot->getHasRef()) {
                /* Since all entry in a form field's kid dict needs to be
                   indirect references, if this annot isn't indirect, it isn't
                   related to a form field */
                continue;
            }

            Ref r = annot->getRef();

            /* Try to find a form field which either has this Annot in its Kids entry
                or  is merged with this Annot */
            FormWidget *tmp = form->findWidgetByRef(r);
            if (tmp) {
                // We've found a corresponding form field, link it
                tmp->setID(FormWidget::encodeID(page, widgets.size()));
                widgets.push_back(tmp);
            }
        }
    }
}

void FormPageWidgets::addWidgets(const std::vector<std::unique_ptr<FormField>> &addedWidgets, unsigned int page)
{
    if (addedWidgets.empty()) {
        return;
    }

    for (const auto &frmField : addedWidgets) {
        FormWidget *frmWidget = frmField->getWidget(0);
        frmWidget->setID(FormWidget::encodeID(page, widgets.size()));
        widgets.push_back(frmWidget);
    }
}

FormPageWidgets::~FormPageWidgets() = default;
