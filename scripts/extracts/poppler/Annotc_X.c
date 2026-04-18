// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 56/79



            auto formDict = std::make_unique<Dict>(gfx->getXRef());
            formDict->set("Length", Object(int(appearBuf->size())));
            formDict->set("Subtype", Object(objName, "Form"));
            formDict->set("Name", Object(objName, "FRM"));
            auto bboxArray = std::make_unique<Array>(gfx->getXRef());
            bboxArray->add(Object(0));
            bboxArray->add(Object(0));
            bboxArray->add(Object(width));
            bboxArray->add(Object(height));
            formDict->set("BBox", Object(std::move(bboxArray)));
            auto matrix = std::make_unique<Array>(gfx->getXRef());
            matrix->add(Object(1));
            matrix->add(Object(0));
            matrix->add(Object(0));
            matrix->add(Object(1));
            matrix->add(Object(-width / 2));
            matrix->add(Object(-height / 2));
            formDict->set("Matrix", Object(std::move(matrix)));
            formDict->set("Resources", Object(std::move(resDict)));

            std::vector<char> data { appearBuf->c_str(), appearBuf->c_str() + appearBuf->size() };
            auto mStream = std::make_unique<AutoFreeMemStream>(std::move(data), Object(std::move(formDict)));

            auto dict = std::make_unique<Dict>(gfx->getXRef());
            dict->set("FRM", Object(std::move(mStream)));

            auto resDict2 = std::make_unique<Dict>(gfx->getXRef());
            resDict2->set("XObject", Object(std::move(dict)));

            appearBuf = std::make_unique<GooString>();
            appearBuf->append("q\n");
            appearBuf->appendf("0 0 {0:d} {1:d} re W n\n", width, height);
            appearBuf->append("q\n");
            appearBuf->appendf("0 0 {0:d} {1:d} re W n\n", width, height);
            appearBuf->appendf("1 0 0 1 {0:d} {1:d} cm\n", width / 2, height / 2);
            appearBuf->append("/FRM Do\n");
            appearBuf->append("Q\n");
            appearBuf->append("Q\n");

            const std::array<double, 4> bbox = { 0, 0, static_cast<double>(width), static_cast<double>(height) };
            appearance = createForm(appearBuf.get(), bbox, false, std::move(resDict2));
        }
    }

    // draw the appearance stream
    Object obj = appearance.fetch(gfx->getXRef());
    gfx->drawAnnot(&obj, nullptr, color.get(), rect->x1, rect->y1, rect->x2, rect->y2, getRotation());
}

//------------------------------------------------------------------------
// AnnotScreen
//------------------------------------------------------------------------
AnnotScreen::AnnotScreen(PDFDoc *docA, PDFRectangle *rectA) : Annot(docA, rectA)
{
    type = typeScreen;

    annotObj.dictSet("Subtype", Object(objName, "Screen"));
    initialize(annotObj.getDict());
}

AnnotScreen::AnnotScreen(PDFDoc *docA, Object &&dictObject, const Object *obj) : Annot(docA, std::move(dictObject), obj)
{
    type = typeScreen;
    initialize(annotObj.getDict());
}

AnnotScreen::~AnnotScreen() = default;

void AnnotScreen::initialize(Dict *dict)
{
    Object obj1;

    obj1 = dict->lookup("T");
    if (obj1.isString()) {
        title = std::make_unique<GooString>(obj1.getString());
    }

    obj1 = dict->lookup("A");
    if (obj1.isDict()) {
        action = LinkAction::parseAction(&obj1, doc->getCatalog()->getBaseURI());
        if (action && action->getKind() == actionRendition && page == 0) {
            error(errSyntaxError, -1, "Invalid Rendition action: associated screen annotation without P");
            action = nullptr;
            ok = false;
        }
    }

    additionalActions = dict->lookupNF("AA").copy();

    obj1 = dict->lookup("MK");
    if (obj1.isDict()) {
        appearCharacs = std::make_unique<AnnotAppearanceCharacs>(obj1.getDict());
    }
}

std::unique_ptr<LinkAction> AnnotScreen::getAdditionalAction(AdditionalActionsType additionalActionType)
{
    if (additionalActionType == actionFocusIn || additionalActionType == actionFocusOut) { // not defined for screen annotation
        return nullptr;
    }

    return ::getAdditionalAction(additionalActionType, &additionalActions, doc);
}

//------------------------------------------------------------------------
// AnnotStamp
//------------------------------------------------------------------------
AnnotStamp::AnnotStamp(PDFDoc *docA, PDFRectangle *rectA) : AnnotMarkup(docA, rectA)
{
    type = typeStamp;
    annotObj.dictSet("Subtype", Object(objName, "Stamp"));
    initialize(annotObj.getDict());
}

AnnotStamp::AnnotStamp(PDFDoc *docA, Object &&dictObject, const Object *obj) : AnnotMarkup(docA, std::move(dictObject), obj)
{
    type = typeStamp;
    initialize(annotObj.getDict());
}

AnnotStamp::~AnnotStamp() = default;

void AnnotStamp::initialize(Dict *dict)
{
    Object obj1 = dict->lookup("Name");
    if (obj1.isName()) {
        icon = obj1.getNameString();
    } else {
        icon = std::string { "Draft" };
    }

    updatedAppearanceStream = Ref::INVALID();
}