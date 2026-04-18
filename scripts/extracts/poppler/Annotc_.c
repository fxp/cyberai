// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 79/79



AnnotAppearanceBuilder::~AnnotAppearanceBuilder() = default;

void AnnotAppearanceBuilder::append(const char *text)
{
    appearBuf->append(text);
}

void AnnotAppearanceBuilder::appendf(const char *fmt, ...) GOOSTRING_FORMAT
{
    va_list argList;

    va_start(argList, fmt);
    appearBuf->appendfv(fmt, argList);
    va_end(argList);
}

const GooString *AnnotAppearanceBuilder::buffer() const
{
    return appearBuf.get();
}
