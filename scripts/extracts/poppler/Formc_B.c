// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 2/24



FormWidget::FormWidget(PDFDoc *docA, Object *aobj, unsigned num, Ref aref, FormField *fieldA)
{
    ref = aref;
    ID = 0;
    childNum = num;
    doc = docA;
    xref = doc->getXRef();
    obj = aobj->copy();
    type = formUndef;
    field = fieldA;
    widget = nullptr;
}

FormWidget::~FormWidget() = default;

void FormWidget::print(int indent) const
{
    printf("%*s+ (%d %d): [widget]\n", indent, "", ref.num, ref.gen);
}

void FormWidget::createWidgetAnnotation()
{
    if (widget) {
        return;
    }

    Object obj1(ref);
    widget = std::make_shared<AnnotWidget>(doc, &obj, &obj1, field);
}

bool FormWidget::inRect(double x, double y) const
{
    return widget ? widget->inRect(x, y) : false;
}

void FormWidget::getRect(double *x1, double *y1, double *x2, double *y2) const
{
    if (widget) {
        widget->getRect(x1, y1, x2, y2);
    }
}

bool FormWidget::isReadOnly() const
{
    return field->isReadOnly();
}

void FormWidget::setReadOnly(bool value)
{
    field->setReadOnly(value);
}

int FormWidget::encodeID(unsigned pageNum, unsigned fieldNum)
{
    return (pageNum << 4 * sizeof(unsigned)) + fieldNum;
}

void FormWidget::decodeID(unsigned id, unsigned *pageNum, unsigned *fieldNum)
{
    *pageNum = id >> 4 * sizeof(unsigned);
    *fieldNum = (id << 4 * sizeof(unsigned)) >> 4 * sizeof(unsigned);
}

const GooString *FormWidget::getPartialName() const
{
    return field->getPartialName();
}

void FormWidget::setPartialName(const GooString &name)
{
    field->setPartialName(name);
}

const GooString *FormWidget::getAlternateUiName() const
{
    return field->getAlternateUiName();
}

const GooString *FormWidget::getMappingName() const
{
    return field->getMappingName();
}

const GooString *FormWidget::getFullyQualifiedName()
{
    return field->getFullyQualifiedName();
}

LinkAction *FormWidget::getActivationAction()
{
    return widget ? widget->getAction() : nullptr;
}

std::unique_ptr<LinkAction> FormWidget::getAdditionalAction(Annot::FormAdditionalActionsType t)
{
    return widget ? widget->getFormAdditionalAction(t) : nullptr;
}

bool FormWidget::setAdditionalAction(Annot::FormAdditionalActionsType t, const std::string &js)
{
    if (!widget) {
        return false;
    }

    return widget->setFormAdditionalAction(t, js);
}

FormWidgetButton::FormWidgetButton(PDFDoc *docA, Object *dictObj, unsigned num, Ref refA, FormField *p) : FormWidget(docA, dictObj, num, refA, p)
{
    type = formButton;

    // Find the name of the ON state in the AP dictionary
    // The reference say the Off state, if it exists, _must_ be stored in the AP dict under the name /Off
    // The "on" state may be stored under any other name
    Object obj1 = obj.dictLookup("AP");
    if (obj1.isDict()) {
        Object obj2 = obj1.dictLookup("N");
        if (obj2.isDict()) {
            for (int i = 0; i < obj2.dictGetLength(); i++) {
                const char *key = obj2.dictGetKey(i);
                if (strcmp(key, "Off") != 0) {
                    onStr = std::make_unique<GooString>(key);
                    break;
                }
            }
        }
    }
}

const char *FormWidgetButton::getOnStr() const
{
    if (onStr) {
        return onStr->c_str();
    }

    // 12.7.4.2.3 Check Boxes
    //  Yes should be used as the name for the on state
    return parent()->getButtonType() == formButtonCheck ? "Yes" : nullptr;
}

FormWidgetButton::~FormWidgetButton() = default;

FormButtonType FormWidgetButton::getButtonType() const
{
    return parent()->getButtonType();
}

void FormWidgetButton::setAppearanceState(const char *state)
{
    if (!widget) {
        return;
    }
    widget->setAppearanceState(state);
}

void FormWidgetButton::updateWidgetAppearance()
{
    // The appearance stream must NOT be regenerated for this widget type
}

void FormWidgetButton::setState(bool astate)
{
    // pushButtons don't have state
    if (parent()->getButtonType() == formButtonPush) {
        return;
    }

    // Silently return if can't set ON state
    if (astate && !getOnStr()) {
        return;
    }

    parent()->setState(astate ? getOnStr() : (char *)"Off");
    // Parent will call setAppearanceState()

    // Now handle standAlone fields which are related to this one by having the same
    // fully qualified name. This is *partially* by spec, as seen in "Field names"
    // section inside "8.6.2 Field Dictionaries" in 1.7 PDF spec. Issue #1034

    if (!astate) { // We're only interested when this field is being set to ON,
        return; // to check if it has related fields and then set them OFF
    }

    unsigned this_page_num, this_field_num;
    decodeID(getID(), &this_page_num, &this_field_num);
    Page *this_page = doc->getCatalog()->getPage(this_page_num);
    const FormField *this_field = getField();
    if (!this_page->hasStandaloneFields() || this_field == nullptr) {
        return;
    }