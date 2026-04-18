// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 17/20



    // char metric exceptions for vertical font
    obj1 = desFontDict->lookup("W2");
    if (obj1.isArray()) {
        int i = 0;
        while (i + 1 < obj1.arrayGetLength()) {
            obj2 = obj1.arrayGet(i);
            obj3 = obj1.arrayGet(i + 1);
            if (obj2.isInt() && obj3.isInt() && i + 4 < obj1.arrayGetLength()) {
                if ((obj4 = obj1.arrayGet(i + 2), obj4.isNum()) && (obj5 = obj1.arrayGet(i + 3), obj5.isNum()) && (obj6 = obj1.arrayGet(i + 4), obj6.isNum())) {
                    GfxFontCIDWidthExcepV excepV { .first = static_cast<CID>(obj2.getInt()), .last = static_cast<CID>(obj3.getInt()), .height = obj4.getNum() * 0.001, .vx = obj5.getNum() * 0.001, .vy = obj6.getNum() * 0.001 };
                    widths.excepsV.push_back(excepV);
                } else {
                    error(errSyntaxError, -1, "Bad widths (W2) array in Type 0 font");
                }
                i += 5;
            } else if (obj2.isInt() && obj3.isArray()) {
                int j = obj2.getInt();
                for (int k = 0; k < obj3.arrayGetLength(); k += 3) {
                    if ((obj4 = obj3.arrayGet(k), obj4.isNum()) && (obj5 = obj3.arrayGet(k + 1), obj5.isNum()) && (obj6 = obj3.arrayGet(k + 2), obj6.isNum())) {
                        GfxFontCIDWidthExcepV excepV { .first = static_cast<CID>(j), .last = static_cast<CID>(j), .height = obj4.getNum() * 0.001, .vx = obj5.getNum() * 0.001, .vy = obj6.getNum() * 0.001 };
                        widths.excepsV.push_back(excepV);
                        if (j == std::numeric_limits<int>::max()) {
                            error(errSyntaxError, -1, "Reached limit for CID in W2 array in Type 0 font");
                            break;
                        }
                        ++j;
                    } else {
                        error(errSyntaxError, -1, "Bad widths (W2) array in Type 0 font");
                    }
                }
                i += 2;
            } else {
                error(errSyntaxError, -1, "Bad widths (W2) array in Type 0 font");
                ++i;
            }
        }
        std::ranges::sort(widths.excepsV, cmpWidthExcepVFunctor());
    }

    ok = true;
}

GfxCIDFont::~GfxCIDFont() = default;

int GfxCIDFont::getNextChar(const char *s, int len, CharCode *code, Unicode const **u, int *uLen, double *dx, double *dy, double *ox, double *oy) const
{
    CID cid;
    CharCode dummy;
    double w, h, vx, vy;
    int n, a, b;

    if (!cMap) {
        *code = 0;
        *uLen = 0;
        *dx = *dy = *ox = *oy = 0;
        return 1;
    }

    *code = (CharCode)(cid = cMap->getCID(s, len, &dummy, &n));
    if (ctu) {
        if (hasToUnicode) {
            int i = 0, c = 0;
            while (i < n) {
                c = (c << 8) + (s[i] & 0xff);
                ++i;
            }
            *uLen = ctu->mapToUnicode(c, u);
        } else {
            *uLen = ctu->mapToUnicode(cid, u);
        }
    } else {
        *uLen = 0;
    }

    // horizontal
    if (cMap->getWMode() == GfxFont::WritingMode::Horizontal) {
        w = getWidth(cid);
        h = vx = vy = 0;

        // vertical
    } else {
        w = 0;
        h = widths.defHeight;
        vx = getWidth(cid) / 2;
        vy = widths.defVY;
        if (!widths.excepsV.empty() && cid >= widths.excepsV[0].first) {
            a = 0;
            b = widths.excepsV.size();
            // invariant: widths.excepsV[a].first <= cid < widths.excepsV[b].first
            while (b - a > 1) {
                const int m = (a + b) / 2;
                if (widths.excepsV[m].last <= cid) {
                    a = m;
                } else {
                    b = m;
                }
            }
            if (cid <= widths.excepsV[a].last) {
                h = widths.excepsV[a].height;
                vx = widths.excepsV[a].vx;
                vy = widths.excepsV[a].vy;
            }
        }
    }

    *dx = w;
    *dy = h;
    *ox = vx;
    *oy = vy;

    return n;
}

GfxFont::WritingMode GfxCIDFont::getWMode() const
{
    return cMap ? cMap->getWMode() : GfxFont::WritingMode::Horizontal;
}

const CharCodeToUnicode *GfxCIDFont::getToUnicode() const
{
    return ctu.get();
}

const GooString *GfxCIDFont::getCollection() const
{
    return cMap ? cMap->getCollection() : nullptr;
}

int GfxCIDFont::mapCodeToGID(FoFiTrueType *ff, int cmapi, Unicode unicode, GfxFont::WritingMode wmode)
{
    unsigned short gid = ff->mapCodeToGID(cmapi, unicode);
    if (wmode == GfxFont::WritingMode::Vertical) {
        const unsigned short vgid = ff->mapToVertGID(gid);
        if (vgid != 0) {
            gid = vgid;
        }
    }
    return gid;
}