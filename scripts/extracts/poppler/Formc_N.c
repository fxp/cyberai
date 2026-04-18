// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 14/24



std::optional<size_t> FormFieldText::parseDA(std::vector<std::string> *daToks) const
{
    if (obj.isDict()) {
        Object objDA(obj.dictLookup("DA"));
        if (objDA.isString()) {
            const std::string &da = objDA.getString();
            const std::optional<size_t> tfIndex = tokenizeDA(da, daToks, "Tf");
            if (tfIndex) {
                return tfIndex.value() - 1;
            }
        }
    }
    return {};
}

//------------------------------------------------------------------------
// FormFieldChoice
//------------------------------------------------------------------------
FormFieldChoice::FormFieldChoice(PDFDoc *docA, Object &&aobj, const Ref refA, FormField *parentA, std::set<int> *usedParents) : FormField(docA, std::move(aobj), refA, parentA, usedParents, formChoice)
{
    numChoices = 0;
    choices = nullptr;
    defaultChoices = nullptr;
    editedChoice = nullptr;
    appearanceSelectedChoice = nullptr;
    topIdx = 0;

    Dict *dict = obj.getDict();
    Object obj1;

    combo = edit = multiselect = doNotSpellCheck = doCommitOnSelChange = false;

    obj1 = Form::fieldLookup(dict, "Ff");
    if (obj1.isInt()) {
        int flags = obj1.getInt();
        if (flags & 0x20000) { // 18 -> Combo
            combo = true;
        }
        if (flags & 0x40000) { // 19 -> Edit
            edit = true;
        }
        if (flags & 0x200000) { // 22 -> MultiSelect
            multiselect = true;
        }
        if (flags & 0x400000) { // 23 -> DoNotSpellCheck
            doNotSpellCheck = true;
        }
        if (flags & 0x4000000) { // 27 -> CommitOnSelChange
            doCommitOnSelChange = true;
        }
    }

    obj1 = dict->lookup("TI");
    if (obj1.isInt()) {
        topIdx = obj1.getInt();
        if (topIdx < 0) {
            error(errSyntaxError, -1, "FormFieldChoice:: invalid topIdx entry");
            topIdx = 0;
        }
    }

    obj1 = Form::fieldLookup(dict, "Opt");
    if (obj1.isArray()) {
        numChoices = obj1.arrayGetLength();
        choices = new ChoiceOpt[numChoices];

        for (int i = 0; i < numChoices; i++) {
            Object obj2 = obj1.arrayGet(i);
            if (obj2.isString()) {
                choices[i].optionName = obj2.takeString();
            } else if (obj2.isArray()) { // [Export_value, Displayed_text]
                if (obj2.arrayGetLength() < 2) {
                    error(errSyntaxError, -1, "FormWidgetChoice:: invalid Opt entry -- array's length < 2");
                    continue;
                }
                Object obj3 = obj2.arrayGet(0);
                if (obj3.isString()) {
                    choices[i].exportVal = obj3.takeString();
                } else {
                    error(errSyntaxError, -1, "FormWidgetChoice:: invalid Opt entry -- exported value not a string");
                }

                obj3 = obj2.arrayGet(1);
                if (obj3.isString()) {
                    choices[i].optionName = obj3.takeString();
                } else {
                    error(errSyntaxError, -1, "FormWidgetChoice:: invalid Opt entry -- choice name not a string");
                }
            } else {
                error(errSyntaxError, -1, "FormWidgetChoice:: invalid {0:d} Opt entry", i);
            }
        }
    } else {
        // empty choice
    }

    // Find selected items
    // Note: PDF specs say that /V has precedence over /I, but acroread seems to
    // do the opposite. We do the same.
    obj1 = Form::fieldLookup(dict, "I");
    if (obj1.isArray()) {
        for (int i = 0; i < obj1.arrayGetLength(); i++) {
            Object obj2 = obj1.arrayGet(i);
            if (obj2.isInt() && obj2.getInt() >= 0 && obj2.getInt() < numChoices) {
                choices[obj2.getInt()].selected = true;
            }
        }
    } else {
        // Note: According to PDF specs, /V should *never* contain the exportVal.
        // However, if /Opt is an array of (exportVal,optionName) pairs, acroread
        // seems to expect the exportVal instead of the optionName and so we do too.
        fillChoices(fillValue);
    }

    fillChoices(fillDefaultValue);
}

void FormFieldChoice::fillChoices(FillValueType fillType)
{
    const char *key = fillType == fillDefaultValue ? "DV" : "V";
    Dict *dict = obj.getDict();
    Object obj1;

    obj1 = Form::fieldLookup(dict, key);
    if (obj1.isString() || obj1.isArray()) {
        if (fillType == fillDefaultValue) {
            defaultChoices = new bool[numChoices];
            memset(defaultChoices, 0, sizeof(bool) * numChoices);
        }

        if (obj1.isString()) {
            bool optionFound = false;