// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 22/24



            for (int code = 0; code <= basicMultilingualMaxCode; ++code) {
                const int glyph = fft->mapCodeToGID(unicodeBMPCMap, code);
                if (FT_Load_Glyph(face, glyph, FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING)) {
                    fontsWidths.addWidth(code, 0);
                } else {
                    fontsWidths.addWidth(code, static_cast<int>(face->glyph->metrics.horiAdvance));
                }
            }
            auto widths = std::make_unique<Array>(xref);
            for (const auto &segment : fontsWidths.takeSegments()) {
                std::visit(
                        [&widths, &xref](auto &&s) {
                            using T = std::decay_t<decltype(s)>;
                            if constexpr (std::is_same_v<T, CIDFontsWidthsBuilder::ListSegment>) {
                                widths->add(Object(s.first));
                                auto widthsInner = std::make_unique<Array>(xref);
                                for (const auto &w : s.widths) {
                                    widthsInner->add(Object(w));
                                }
                                widths->add(Object(std::move(widthsInner)));
                            } else if constexpr (std::is_same_v<T, CIDFontsWidthsBuilder::RangeSegment>) {
                                widths->add(Object(s.first));
                                widths->add(Object(s.last));
                                widths->add(Object(s.width));
                            } else {
                                static_assert(always_false_v<T>, "non-exhaustive visitor");
                            }
                        },
                        segment);
            }
            descendantFont->set("W", Object(std::move(widths)));

            std::vector<char> data;
            data.reserve(2 * basicMultilingualMaxCode);

            for (int code = 0; code <= basicMultilingualMaxCode; ++code) {
                const int glyph = fft->mapCodeToGID(unicodeBMPCMap, code);
                data.push_back((char)(glyph >> 8));
                data.push_back((char)(glyph & 0xff));
            }
            const Ref cidToGidMapStream = xref->addStreamObject(std::make_unique<Dict>(xref), std::move(data), StreamCompression::Compress);
            descendantFont->set("CIDToGIDMap", Object(cidToGidMapStream));
        }

        descendantFonts->add(Object(std::move(descendantFont)));

        fontDict.dictSet("DescendantFonts", Object(std::move(descendantFonts)));
    }

    const Ref fontDictRef = xref->addIndirectObject(fontDict);

    std::string dictFontName = forceName ? fontFamilyAndStyle : kOurDictFontNamePrefix;
    Object *acroForm = doc->getCatalog()->getAcroForm();
    if (resDict.isDict()) {
        Ref fontDictObjRef;
        Object fontDictObj = resDict.getDict()->lookup("Font", &fontDictObjRef);
        assert(fontDictObj.isDict());
        dictFontName = fontDictObj.getDict()->findAvailableKey(dictFontName);
        fontDictObj.dictSet(dictFontName, Object(fontDictRef));

        if (fontDictObjRef != Ref::INVALID()) {
            xref->setModifiedObject(&fontDictObj, fontDictObjRef);
        } else {
            Ref resDictRef;
            acroForm->getDict()->lookup("DR", &resDictRef);
            if (resDictRef != Ref::INVALID()) {
                xref->setModifiedObject(&resDict, resDictRef);
            } else {
                doc->getCatalog()->setAcroFormModified();
            }
        }

        // maybe we can do something to reuse the existing data instead of recreating from scratch?
        delete defaultResources;
        defaultResources = new GfxResources(xref, resDict.getDict(), nullptr);
    } else {
        auto fontsDict = std::make_unique<Dict>(xref);
        fontsDict->set(dictFontName, Object(fontDictRef));

        auto defaultResourcesDict = std::make_unique<Dict>(xref);
        defaultResourcesDict->set("Font", Object(std::move(fontsDict)));

        assert(!defaultResources);
        defaultResources = new GfxResources(xref, defaultResourcesDict.get(), nullptr);
        resDict = Object(std::move(defaultResourcesDict));

        acroForm->dictSet("DR", resDict.copy());
        doc->getCatalog()->setAcroFormModified();
    }

    return { .fontName = dictFontName, .ref = fontDictRef };
}

std::string Form::getFallbackFontForChar(Unicode uChar, const GfxFont &fontToEmulate) const
{
    const UCharFontSearchResult res = globalParams->findSystemFontFileForUChar(uChar, fontToEmulate);

    return findFontInDefaultResources(res.family, res.style);
}