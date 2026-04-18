// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 77/79



AnnotRichMedia::Params *AnnotRichMedia::Instance::getParams() const
{
    return params.get();
}

AnnotRichMedia::Params::Params(Dict *dict)
{
    Object obj1 = dict->lookup("FlashVars");
    if (obj1.isString()) {
        flashVars = obj1.takeString();
    }
}

AnnotRichMedia::Params::~Params() = default;

const GooString *AnnotRichMedia::Params::getFlashVars() const
{
    return flashVars.get();
}

//------------------------------------------------------------------------
// Annots
//------------------------------------------------------------------------

Annots::Annots(PDFDoc *docA, int page, Object *annotsObj)
{
    doc = docA;

    if (annotsObj->isArray()) {
        for (int i = 0; i < annotsObj->arrayGetLength(); ++i) {
            // get the Ref to this annot and pass it to Annot constructor
            // this way, it'll be possible for the annot to retrieve the corresponding
            // form widget
            Object obj1 = annotsObj->arrayGet(i);
            if (obj1.isDict()) {
                const Object &obj2 = annotsObj->arrayGetNF(i);
                std::shared_ptr<Annot> annot = createAnnot(std::move(obj1), &obj2);
                if (annot) {
                    if (annot.use_count() > 100000) {
                        error(errSyntaxError, -1, "Annotations likely malformed. Too many references. Stopping processing annots on page {0:d}", page);
                        break;
                    }
                    if (annot->isOk()) {
                        annot->setPage(page, false); // Don't change /P
                        appendAnnot(annot);
                    }
                }
            }
        }
    }
}

void Annots::appendAnnot(std::shared_ptr<Annot> annot)
{
    if (annot && annot->isOk()) {
        annots.push_back(std::move(annot));
    }
}

bool Annots::removeAnnot(const std::shared_ptr<Annot> &annot)
{
    auto idx = std::ranges::find(annots, annot);

    if (idx == annots.end()) {
        return false;
    }
    annots.erase(idx);
    return true;
}

std::shared_ptr<Annot> Annots::createAnnot(Object &&dictObject, const Object *obj)
{
    std::shared_ptr<Annot> annot = nullptr;
    Object obj1 = dictObject.dictLookup("Subtype");
    if (obj1.isName()) {
        const char *typeName = obj1.getName();