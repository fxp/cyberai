// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 3/24



    auto this_page_widgets = this_page->getFormWidgets();
    const FormButtonType this_button_type = getButtonType();

    const int tot = this_page_widgets->getNumWidgets();
    for (int i = 0; i < tot; i++) {
        bool found_related = false;
        FormWidget *wid = this_page_widgets->getWidget(i);
        const bool same_fqn = wid->getFullyQualifiedName()->compare(getFullyQualifiedName()->toStr()) == 0;
        const bool same_button_type = wid->getType() == formButton && static_cast<const FormWidgetButton *>(wid)->getButtonType() == this_button_type;

        if (same_fqn && same_button_type) {
            if (this_field->isStandAlone()) {
                //'this_field' is standAlone, so we need to search in both standAlone fields and normal fields
                if (this_field != wid->getField()) { // so take care to not choose our same field
                    found_related = true;
                }
            } else {
                //'this_field' is not standAlone, so we just need to search in standAlone fields
                if (wid->getField()->isStandAlone()) {
                    found_related = true;
                }
            }
        }

        if (found_related) {
            auto *ffb = static_cast<FormFieldButton *>(wid->getField());
            if (ffb == nullptr) {
                error(errInternal, -1, "FormWidgetButton::setState : FormFieldButton expected");
                continue;
            }
            ffb->setState((char *)"Off", true);
        }
    }
}

bool FormWidgetButton::getState() const
{
    return getOnStr() ? parent()->getState(getOnStr()) : false;
}

FormFieldButton *FormWidgetButton::parent() const
{
    return static_cast<FormFieldButton *>(field);
}

FormWidgetText::FormWidgetText(PDFDoc *docA, Object *dictObj, unsigned num, Ref refA, FormField *p) : FormWidget(docA, dictObj, num, refA, p)
{
    type = formText;
}

const GooString *FormWidgetText::getContent() const
{
    return parent()->getContent();
}

void FormWidgetText::updateWidgetAppearance()
{
    if (widget) {
        widget->updateAppearanceStream();
    }
}

bool FormWidgetText::isMultiline() const
{
    return parent()->isMultiline();
}

bool FormWidgetText::isPassword() const
{
    return parent()->isPassword();
}

bool FormWidgetText::isFileSelect() const
{
    return parent()->isFileSelect();
}

bool FormWidgetText::noSpellCheck() const
{
    return parent()->noSpellCheck();
}

bool FormWidgetText::noScroll() const
{
    return parent()->noScroll();
}

bool FormWidgetText::isComb() const
{
    return parent()->isComb();
}

bool FormWidgetText::isRichText() const
{
    return parent()->isRichText();
}

int FormWidgetText::getMaxLen() const
{
    return parent()->getMaxLen();
}

double FormWidgetText::getTextFontSize()
{
    return parent()->getTextFontSize();
}

void FormWidgetText::setTextFontSize(int fontSize)
{
    parent()->setTextFontSize(fontSize);
}

void FormWidgetText::setContent(std::unique_ptr<GooString> new_content)
{
    parent()->setContent(std::move(new_content));
}

void FormWidgetText::setAppearanceContent(std::unique_ptr<GooString> new_content)
{
    parent()->setAppearanceContent(std::move(new_content));
}

FormFieldText *FormWidgetText::parent() const
{
    return static_cast<FormFieldText *>(field);
}

FormWidgetChoice::FormWidgetChoice(PDFDoc *docA, Object *dictObj, unsigned num, Ref refA, FormField *p) : FormWidget(docA, dictObj, num, refA, p)
{
    type = formChoice;
}

FormWidgetChoice::~FormWidgetChoice() = default;

bool FormWidgetChoice::_checkRange(int i) const
{
    if (i < 0 || i >= parent()->getNumChoices()) {
        error(errInternal, -1, "FormWidgetChoice::_checkRange i out of range : {0:d}", i);
        return false;
    }
    return true;
}

void FormWidgetChoice::select(int i)
{
    if (!_checkRange(i)) {
        return;
    }
    parent()->select(i);
}

void FormWidgetChoice::toggle(int i)
{
    if (!_checkRange(i)) {
        return;
    }
    parent()->toggle(i);
}

void FormWidgetChoice::deselectAll()
{
    parent()->deselectAll();
}

const GooString *FormWidgetChoice::getEditChoice() const
{
    if (!hasEdit()) {
        error(errInternal, -1, "FormFieldChoice::getEditChoice called on a non-editable choice");
        return nullptr;
    }
    return parent()->getEditChoice();
}

void FormWidgetChoice::updateWidgetAppearance()
{
    if (widget) {
        widget->updateAppearanceStream();
    }
}

bool FormWidgetChoice::isSelected(int i) const
{
    if (!_checkRange(i)) {
        return false;
    }
    return parent()->isSelected(i);
}

void FormWidgetChoice::setEditChoice(std::unique_ptr<GooString> new_content)
{
    if (!hasEdit()) {
        error(errInternal, -1, "FormFieldChoice::setEditChoice : trying to edit an non-editable choice");
        return;
    }

    parent()->setEditChoice(std::move(new_content));
}