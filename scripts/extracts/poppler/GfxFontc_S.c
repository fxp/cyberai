// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 19/20



                len = tctu->mapToUnicode(cid, &ucodes);
                if (len == 1) {
                    tumap[cid] = ucodes[0];
                } else {
                    /* if not single character, ignore it */
                    tumap[cid] = 0;
                }
            }
        }
        vumap = new Unicode[n];
        memset(vumap, 0, sizeof(Unicode) * n);
        for (const std::string &cname : *lp->CMaps) {
            std::shared_ptr<CMap> cnameCMap;
            if ((cnameCMap = globalParams->getCMap(getCollection()->toStr(), cname)) != nullptr) {
                if (cnameCMap->getWMode() == GfxFont::WritingMode::Vertical) {
                    cnameCMap->setReverseMap(vumap, n, 1);
                } else {
                    cnameCMap->setReverseMap(humap, n, N_UCS_CANDIDATES);
                }
            }
        }
        ff->setupGSUB(lp->scriptTag, lp->languageTag);
    } else {
        if (getCollection()->compare("Adobe-Identity") == 0) {
            error(errSyntaxError, -1, "non-embedded font using identity encoding: {0:s}", name ? name->c_str() : "(null)");
        } else {
            error(errSyntaxError, -1, "Unknown character collection {0:t}", getCollection());
        }
        if (ctu) {
            CharCode cid;
            for (cid = 0; cid < n; cid++) {
                const Unicode *ucode;

                if (ctu->mapToUnicode(cid, &ucode)) {
                    humap[cid * N_UCS_CANDIDATES] = ucode[0];
                } else {
                    humap[cid * N_UCS_CANDIDATES] = 0;
                }
                for (i = 1; i < N_UCS_CANDIDATES; i++) {
                    humap[cid * N_UCS_CANDIDATES + i] = 0;
                }
            }
        }
    }
    // map CID -> Unicode -> GID
    std::vector<int> codeToGID;
    codeToGID.resize(n, 0);
    for (unsigned long code = 0; code < n; ++code) {
        Unicode unicode;
        unsigned long gid;

        unicode = 0;
        gid = 0;
        if (humap != nullptr) {
            for (i = 0; i < N_UCS_CANDIDATES && gid == 0 && (unicode = humap[code * N_UCS_CANDIDATES + i]) != 0; i++) {
                gid = mapCodeToGID(ff, cmap, unicode, GfxFont::WritingMode::Horizontal);
            }
        }
        if (gid == 0 && vumap != nullptr) {
            unicode = vumap[code];
            if (unicode != 0) {
                gid = mapCodeToGID(ff, cmap, unicode, GfxFont::WritingMode::Vertical);
                if (gid == 0 && tumap != nullptr) {
                    if ((unicode = tumap[code]) != 0) {
                        gid = mapCodeToGID(ff, cmap, unicode, GfxFont::WritingMode::Vertical);
                    }
                }
            }
        }
        if (gid == 0 && tumap != nullptr) {
            if ((unicode = tumap[code]) != 0) {
                gid = mapCodeToGID(ff, cmap, unicode, GfxFont::WritingMode::Horizontal);
            }
        }
        if (gid == 0) {
            /* special handling space characters */
            const unsigned long *p;

            if (humap != nullptr) {
                unicode = humap[code];
            }
            if (unicode != 0) {
                /* check if code is space character , so map code to 0x0020 */
                for (p = spaces; *p != 0; p++) {
                    if (*p == unicode) {
                        unicode = 0x20;
                        gid = mapCodeToGID(ff, cmap, unicode, wmode);
                        break;
                    }
                }
            }
        }
        codeToGID[code] = gid;
    }
    delete[] humap;
    delete[] tumap;
    delete[] vumap;
    return codeToGID;
}

double GfxCIDFont::getWidth(CID cid) const
{
    double w;
    int a, b;

    w = widths.defWidth;
    if (!widths.exceps.empty() && cid >= widths.exceps[0].first) {
        a = 0;
        b = widths.exceps.size();
        // invariant: widths.exceps[a].first <= cid < widths.exceps[b].first
        while (b - a > 1) {
            const int m = (a + b) / 2;
            if (widths.exceps[m].first <= cid) {
                a = m;
            } else {
                b = m;
            }
        }
        if (cid <= widths.exceps[a].last) {
            w = widths.exceps[a].width;
        }
    }
    return w;
}

double GfxCIDFont::getWidth(char *s, int len) const
{
    int nUsed;
    CharCode c;

    const CID cid = cMap->getCID(s, len, &c, &nUsed);
    return getWidth(cid);
}

bool GfxFont::isBase14Font(std::string_view family, std::string_view style)
{
    std::string key;
    key.reserve(family.size() + (style.empty() ? 0 : style.size() + 1));
    key.append(family);
    if (!style.empty()) {
        key.push_back('-');
        key.append(style);
    }
    return std::ranges::any_of(base14SubstFonts, [key](auto &&element) { return key == element; });
}

//------------------------------------------------------------------------
// GfxFontDict
//------------------------------------------------------------------------