// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 15/24



            for (int i = 0; i < numChoices; i++) {
                if (choices[i].exportVal) {
                    if (choices[i].exportVal->compare(obj1.getString()) == 0) {
                        optionFound = true;
                    }
                } else if (choices[i].optionName) {
                    if (choices[i].optionName->compare(obj1.getString()) == 0) {
                        optionFound = true;
                    }
                }

                if (optionFound) {
                    if (fillType == fillDefaultValue) {
                        defaultChoices[i] = true;
                    } else {
                        choices[i].selected = true;
                    }
                    break; // We've determined that this option is selected. No need to keep on scanning
                }
            }

            // Set custom value if /V doesn't refer to any predefined option and the field is user-editable
            if (fillType == fillValue && !optionFound && edit) {
                editedChoice = obj1.takeString();
            }
        } else if (obj1.isArray()) {
            for (int i = 0; i < numChoices; i++) {
                for (int j = 0; j < obj1.arrayGetLength(); j++) {
                    const Object obj2 = obj1.arrayGet(j);
                    if (!obj2.isString()) {
                        error(errSyntaxError, -1, "FormWidgetChoice:: {0:s} array contains a non string object", key);
                        continue;
                    }

                    bool matches = false;

                    if (choices[i].exportVal) {
                        if (choices[i].exportVal->compare(obj2.getString()) == 0) {
                            matches = true;
                        }
                    } else if (choices[i].optionName) {
                        if (choices[i].optionName->compare(obj2.getString()) == 0) {
                            matches = true;
                        }
                    }

                    if (matches) {
                        if (fillType == fillDefaultValue) {
                            defaultChoices[i] = true;
                        } else {
                            choices[i].selected = true;
                        }
                        break; // We've determined that this option is selected. No need to keep on scanning
                    }
                }
            }
        }
    }
}

FormFieldChoice::~FormFieldChoice()
{
    delete[] choices;
    delete[] defaultChoices;
}

void FormFieldChoice::print(int indent)
{
    printf("%*s- (%d %d): [choice] terminal: %s children: %zu\n", indent, "", ref.num, ref.gen, terminal ? "Yes" : "No", terminal ? widgets.size() : children.size());
}

void FormFieldChoice::updateSelection()
{
    Object objV;
    Object objI = Object::null();

    if (edit && editedChoice) {
        // This is an editable combo-box with user-entered text
        objV = Object(editedChoice->copy());
    } else {
        const int numSelected = getNumSelected();

        // Create /I array only if multiple selection is allowed (as per PDF spec)
        if (multiselect) {
            objI = Object(std::make_unique<Array>(xref));
        }

        if (numSelected == 0) {
            // No options are selected
            objV = Object(std::make_unique<GooString>(""));
        } else if (numSelected == 1) {
            // Only one option is selected
            for (int i = 0; i < numChoices; i++) {
                if (choices[i].selected) {
                    if (multiselect) {
                        objI.arrayAdd(Object(i));
                    }

                    if (choices[i].exportVal) {
                        objV = Object(choices[i].exportVal->copy());
                    } else if (choices[i].optionName) {
                        objV = Object(choices[i].optionName->copy());
                    }

                    break; // We've just written the selected option. No need to keep on scanning
                }
            }
        } else {
            // More than one option is selected
            objV = Object(std::make_unique<Array>(xref));
            for (int i = 0; i < numChoices; i++) {
                if (choices[i].selected) {
                    if (multiselect) {
                        objI.arrayAdd(Object(i));
                    }

                    if (choices[i].exportVal) {
                        objV.arrayAdd(Object(choices[i].exportVal->copy()));
                    } else if (choices[i].optionName) {
                        objV.arrayAdd(Object(choices[i].optionName->copy()));
                    }
                }
            }
        }
    }

    obj.getDict()->set("V", std::move(objV));
    obj.getDict()->set("I", std::move(objI));
    xref->setModifiedObject(&obj, ref);
    updateChildrenAppearance();
}

void FormFieldChoice::unselectAll()
{
    for (int i = 0; i < numChoices; i++) {
        choices[i].selected = false;
    }
}