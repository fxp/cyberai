// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 39/41



//------------------------------------------------------------------------
// type 3 font operators
//------------------------------------------------------------------------

void Gfx::opSetCharWidth(Object args[], int /*numArgs*/)
{
    out->type3D0(state, args[0].getNum(), args[1].getNum());
}

void Gfx::opSetCacheDevice(Object args[], int /*numArgs*/)
{
    if (displayTypes.top() == DisplayType::Type3Font) {
        type3FontIsD1.top() = true;
    }
    out->type3D1(state, args[0].getNum(), args[1].getNum(), args[2].getNum(), args[3].getNum(), args[4].getNum(), args[5].getNum());
}

//------------------------------------------------------------------------
// compatibility operators
//------------------------------------------------------------------------

void Gfx::opBeginIgnoreUndef(Object /*args*/[], int /*numArgs*/)
{
    ++ignoreUndef;
}

void Gfx::opEndIgnoreUndef(Object /*args*/[], int /*numArgs*/)
{
    if (ignoreUndef > 0) {
        --ignoreUndef;
    }
}

//------------------------------------------------------------------------
// marked content operators
//------------------------------------------------------------------------

enum GfxMarkedContentKind
{
    gfxMCOptionalContent,
    gfxMCActualText,
    gfxMCOther
};

struct MarkedContentStack
{
    GfxMarkedContentKind kind;
    bool ocSuppressed; // are we ignoring content based on OptionalContent?
    MarkedContentStack *next; // next object on stack
};

void Gfx::popMarkedContent()
{
    MarkedContentStack *mc = mcStack;
    mcStack = mc->next;
    delete mc;
}

void Gfx::pushMarkedContent()
{
    auto *mc = new MarkedContentStack();
    mc->ocSuppressed = false;
    mc->kind = gfxMCOther;
    mc->next = mcStack;
    mcStack = mc;
}

bool Gfx::contentIsHidden()
{
    MarkedContentStack *mc = mcStack;
    bool hidden = mc && mc->ocSuppressed;
    while (!hidden && mc && mc->next) {
        mc = mc->next;
        hidden = mc->ocSuppressed;
    }
    return hidden;
}

void Gfx::opBeginMarkedContent(Object args[], int numArgs)
{
    // push a new stack entry
    pushMarkedContent();

    const OCGs *contentConfig = catalog->getOptContentConfig();
    const char *name0 = args[0].getName();
    if (strncmp(name0, "OC", 2) == 0 && contentConfig) {
        if (numArgs >= 2) {
            if (args[1].isName()) {
                const char *name1 = args[1].getName();
                MarkedContentStack *mc = mcStack;
                mc->kind = gfxMCOptionalContent;
                Object markedContent = res->lookupMarkedContentNF(name1);
                if (!markedContent.isNull()) {
                    bool visible = contentConfig->optContentIsVisible(&markedContent);
                    mc->ocSuppressed = !visible;
                } else {
                    error(errSyntaxError, getPos(), "DID NOT find {0:s}", name1);
                }
            } else {
                error(errSyntaxError, getPos(), "Unexpected MC Type: {0:d}", args[1].getType());
            }
        } else {
            error(errSyntaxError, getPos(), "insufficient arguments for Marked Content");
        }
    } else if (args[0].isName("Span") && numArgs == 2) {
        Object dictToUse;
        if (args[1].isDict()) {
            dictToUse = args[1].copy();
        } else if (args[1].isName()) {
            dictToUse = res->lookupMarkedContentNF(args[1].getName()).fetch(xref);
        }

        if (dictToUse.isDict()) {
            Object obj = dictToUse.dictLookup("ActualText");
            if (obj.isString()) {
                out->beginActualText(state, obj.getString());
                MarkedContentStack *mc = mcStack;
                mc->kind = gfxMCActualText;
            }
        }
    }

    if (printCommands) {
        printf("  marked content: %s ", args[0].getName());
        if (numArgs == 2) {
            args[1].print(stdout);
        }
        printf("\n");
        fflush(stdout);
    }
    ocState = !contentIsHidden();

    if (numArgs == 2 && args[1].isDict()) {
        out->beginMarkedContent(args[0].getName(), args[1].getDict());
    } else if (numArgs == 1) {
        out->beginMarkedContent(args[0].getName(), nullptr);
    }
}

void Gfx::opEndMarkedContent(Object /*args*/[], int /*numArgs*/)
{
    if (!mcStack) {
        error(errSyntaxWarning, getPos(), "Mismatched EMC operator");
        return;
    }

    MarkedContentStack *mc = mcStack;
    GfxMarkedContentKind mcKind = mc->kind;

    // pop the stack
    popMarkedContent();

    if (mcKind == gfxMCActualText) {
        out->endActualText(state);
    }
    ocState = !contentIsHidden();

    out->endMarkedContent(state);
}

void Gfx::opMarkPoint(Object args[], int numArgs)
{
    if (printCommands) {
        printf("  mark point: %s ", args[0].getName());
        if (numArgs == 2) {
            args[1].print(stdout);
        }
        printf("\n");
        fflush(stdout);
    }