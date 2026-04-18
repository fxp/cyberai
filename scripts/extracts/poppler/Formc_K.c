// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 11/24



    bool isChildRadiobutton = btype == formButtonRadio && terminal && parent && parent->getType() == formButton;
    // Ignore "V" for child radiobuttons, so FormFieldButton::getState() does not use it and instead uses the
    // "V" of the parent, which is the real value indicating the active field in the radio group. Issue #159
    if (btype != formButtonPush && !isChildRadiobutton) {
        // Even though V is inheritable we are interested in the value of this
        // field, if not present it's probably because it's a button in a set.
        appearanceState = dict->lookup("V");
        defaultAppearanceState = Form::fieldLookup(dict, "DV");
    }
}

static const char *_getButtonType(FormButtonType type)
{
    switch (type) {
    case formButtonPush:
        return "push";
    case formButtonCheck:
        return "check";
    case formButtonRadio:
        return "radio";
    default:
        break;
    }
    return "unknown";
}

void FormFieldButton::print(int indent)
{
    printf("%*s- (%d %d): [%s] terminal: %s children: %zu\n", indent, "", ref.num, ref.gen, _getButtonType(btype), terminal ? "Yes" : "No", terminal ? widgets.size() : children.size());
}

void FormFieldButton::setNumSiblings(int num)
{
    siblings.resize(num, nullptr);
}

void FormFieldButton::fillChildrenSiblingsID()
{
    if (!terminal) {
        int numChildren = int(children.size());
        for (int i = 0; i < numChildren; i++) {
            auto *child = dynamic_cast<FormFieldButton *>(children[i].get());
            if (child != nullptr) {
                // Fill the siblings of this node childs
                child->setNumSiblings(numChildren - 1);
                for (int j = 0, counter = 0; j < numChildren; j++) {
                    auto *otherChild = dynamic_cast<FormFieldButton *>(children[j].get());
                    if (i == j) {
                        continue;
                    }
                    if (child == otherChild) {
                        continue;
                    }
                    child->setSibling(counter, otherChild);
                    counter++;
                }

                // now call ourselves on the child
                // to fill its children data
                child->fillChildrenSiblingsID();
            }
        }
    }
}

bool FormFieldButton::setState(const char *state, bool ignoreToggleOff)
{
    // A check button could behave as a radio button
    // when it's in a set of more than 1 buttons
    if (btype != formButtonRadio && btype != formButtonCheck) {
        return false;
    }

    if (terminal && parent && parent->getType() == formButton && appearanceState.isNull()) {
        // It's button in a set, set state on parent
        return static_cast<FormFieldButton *>(parent)->setState(state);
    }

    bool isOn = strcmp(state, "Off") != 0;

    if (!isOn && noAllOff && !ignoreToggleOff) {
        return false; // Don't allow to set all radio to off
    }

    const char *current = getAppearanceState();
    bool currentFound = false, newFound = false;

    for (size_t i = 0; i < (terminal ? widgets.size() : children.size()); i++) {
        FormWidgetButton *widget;

        // If radio button is a terminal field we want the widget at i, but
        // if it's not terminal, the child widget is a composed dict, so
        // we want the ony child widget of the children at i
        if (terminal) {
            widget = static_cast<FormWidgetButton *>(widgets[i].get());
        } else {
            widget = static_cast<FormWidgetButton *>(children[i]->getWidget(0));
        }

        if (!widget->getOnStr()) {
            continue;
        }

        const char *onStr = widget->getOnStr();
        if (current && strcmp(current, onStr) == 0) {
            widget->setAppearanceState("Off");
            if (!isOn) {
                break;
            }
            currentFound = true;
        }

        if (isOn && strcmp(state, onStr) == 0) {
            widget->setAppearanceState(state);
            newFound = true;
        }

        if (currentFound && newFound) {
            break;
        }
    }

    updateState(state);

    return true;
}

bool FormFieldButton::getState(const char *state) const
{
    if (appearanceState.isName(state)) {
        return true;
    }

    return (parent && parent->getType() == formButton) ? static_cast<FormFieldButton *>(parent)->getState(state) : false;
}

void FormFieldButton::updateState(const char *state)
{
    appearanceState = Object(objName, state);
    obj.getDict()->set("V", appearanceState.copy());
    xref->setModifiedObject(&obj, ref);
}

FormFieldButton::~FormFieldButton() = default;

void FormFieldButton::reset(const std::vector<std::string> &excludedFields)
{
    if (!isAmongExcludedFields(excludedFields)) {
        if (getDefaultAppearanceState()) {
            setState(getDefaultAppearanceState());
        } else {
            obj.getDict()->remove("V");