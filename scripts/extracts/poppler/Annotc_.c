// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 78/79



        if (!strcmp(typeName, "Text")) {
            annot = std::make_shared<AnnotText>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Link")) {
            annot = std::make_shared<AnnotLink>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "FreeText")) {
            annot = std::make_shared<AnnotFreeText>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Line")) {
            annot = std::make_shared<AnnotLine>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Square")) {
            annot = std::make_shared<AnnotGeometry>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Circle")) {
            annot = std::make_shared<AnnotGeometry>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Polygon")) {
            annot = std::make_shared<AnnotPolygon>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "PolyLine")) {
            annot = std::make_shared<AnnotPolygon>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Highlight")) {
            annot = std::make_shared<AnnotTextMarkup>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Underline")) {
            annot = std::make_shared<AnnotTextMarkup>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Squiggly")) {
            annot = std::make_shared<AnnotTextMarkup>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "StrikeOut")) {
            annot = std::make_shared<AnnotTextMarkup>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Stamp")) {
            annot = std::make_shared<AnnotStamp>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Caret")) {
            annot = std::make_shared<AnnotCaret>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Ink")) {
            annot = std::make_shared<AnnotInk>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "FileAttachment")) {
            annot = std::make_shared<AnnotFileAttachment>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Sound")) {
            annot = std::make_shared<AnnotSound>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Movie")) {
            annot = std::make_shared<AnnotMovie>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Widget")) {
            // Find the annot in forms
            if (obj->isRef()) {
                Form *form = doc->getCatalog()->getForm();
                if (form) {
                    FormWidget *widget = form->findWidgetByRef(obj->getRef());
                    if (widget) {
                        annot = widget->getWidgetAnnotation();
                    }
                }
            }
            if (!annot) {
                annot = std::make_shared<AnnotWidget>(doc, std::move(dictObject), obj);
            }
        } else if (!strcmp(typeName, "Screen")) {
            annot = std::make_shared<AnnotScreen>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "PrinterMark")) {
            annot = std::make_shared<Annot>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "TrapNet")) {
            annot = std::make_shared<Annot>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Watermark")) {
            annot = std::make_shared<Annot>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "3D")) {
            annot = std::make_shared<Annot3D>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "RichMedia")) {
            annot = std::make_shared<AnnotRichMedia>(doc, std::move(dictObject), obj);
        } else if (!strcmp(typeName, "Popup")) {
            /* Popup annots are already handled by markup annots
             * Here we only care about popup annots without a
             * markup annotation associated
             */
            Object obj2 = dictObject.dictLookup("Parent");
            if (obj2.isNull()) {
                annot = std::make_shared<AnnotPopup>(doc, std::move(dictObject), obj);
            } else {
                annot = nullptr;
            }
        } else {
            annot = std::make_shared<Annot>(doc, std::move(dictObject), obj);
        }
    }

    return annot;
}

std::shared_ptr<Annot> Annots::findAnnot(Ref *ref)
{
    for (const auto &annot : annots) {
        if (annot->match(ref)) {
            return annot;
        }
    }
    return nullptr;
}

Annots::~Annots() = default;

//------------------------------------------------------------------------
// AnnotAppearanceBuilder
//------------------------------------------------------------------------

AnnotAppearanceBuilder::AnnotAppearanceBuilder() : appearBuf { std::make_unique<GooString>() }
{
    addedDingbatsResource = false;
}