// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 20/20



GfxFontDict::GfxFontDict(XRef *xref, const Ref fontDictRef, const Dict &fontDict)
{
    Ref r;

    fonts.resize(fontDict.getLength());
    for (std::size_t i = 0; i < fonts.size(); ++i) {
        const Object &obj1 = fontDict.getValNF(i);
        Object obj2 = obj1.fetch(xref);
        if (obj2.isDict()) {
            if (obj1.isRef()) {
                r = obj1.getRef();
            } else if (fontDictRef != Ref::INVALID()) {
                // legal generation numbers are five digits, so we use a
                // 6-digit number here
                r.gen = 100000 + fontDictRef.num;
                r.num = i;
            } else {
                // no indirect reference for this font, or for the containing
                // font dict, so hash the font and use that
                r.gen = 100000;
                r.num = hashFontObject(&obj2);
            }
            fonts[i] = GfxFont::makeFont(xref, fontDict.getKey(i), r, *obj2.getDict());
            if (fonts[i] && !fonts[i]->isOk()) {
                // XXX: it may be meaningful to distinguish between
                // NULL and !isOk() so that when we do lookups
                // we can tell the difference between a missing font
                // and a font that is just !isOk()
                fonts[i].reset();
            }
        } else {
            error(errSyntaxError, -1, "font resource is not a dictionary");
            fonts[i] = nullptr;
        }
    }
}

std::shared_ptr<GfxFont> GfxFontDict::lookup(const char *tag) const
{
    for (const auto &font : fonts) {
        if (font && font->matches(tag)) {
            return font;
        }
    }
    return nullptr;
}

// FNV-1a hash
class FNVHash
{
public:
    FNVHash() { h = 2166136261U; }

    void hash(char c)
    {
        h ^= c & 0xff;
        h *= 16777619;
    }

    void hash(const char *p, int n)
    {
        int i;
        for (i = 0; i < n; ++i) {
            hash(p[i]);
        }
    }

    int get31() const { return (h ^ (h >> 31)) & 0x7fffffff; }

private:
    unsigned int h;
};

int GfxFontDict::hashFontObject(Object *obj)
{
    FNVHash h;

    hashFontObject1(obj, &h);
    return h.get31();
}

void GfxFontDict::hashFontObject1(const Object *obj, FNVHash *h)
{
    const char *p;
    double r;
    int n, i;

    switch (obj->getType()) {
    case objBool:
        h->hash('b');
        h->hash(obj->getBool() ? 1 : 0);
        break;
    case objInt:
        h->hash('i');
        n = obj->getInt();
        h->hash((char *)&n, sizeof(int));
        break;
    case objReal:
        h->hash('r');
        r = obj->getReal();
        h->hash((char *)&r, sizeof(double));
        break;
    case objString: {
        h->hash('s');
        const auto &s = obj->getString();
        h->hash(s.c_str(), s.size());
    } break;
    case objName:
        h->hash('n');
        p = obj->getName();
        h->hash(p, (int)strlen(p));
        break;
    case objNull:
        h->hash('z');
        break;
    case objArray:
        h->hash('a');
        n = obj->arrayGetLength();
        h->hash((char *)&n, sizeof(int));
        for (i = 0; i < n; ++i) {
            const Object &obj2 = obj->arrayGetNF(i);
            hashFontObject1(&obj2, h);
        }
        break;
    case objDict:
        h->hash('d');
        n = obj->dictGetLength();
        h->hash((char *)&n, sizeof(int));
        for (i = 0; i < n; ++i) {
            p = obj->dictGetKey(i);
            h->hash(p, (int)strlen(p));
            const Object &obj2 = obj->dictGetValNF(i);
            hashFontObject1(&obj2, h);
        }
        break;
    case objStream:
        // this should never happen - streams must be indirect refs
        break;
    case objRef:
        h->hash('f');
        n = obj->getRefNum();
        h->hash((char *)&n, sizeof(int));
        n = obj->getRefGen();
        h->hash((char *)&n, sizeof(int));
        break;
    default:
        h->hash('u');
        break;
    }
}
