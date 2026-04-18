// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 16/20



        // use an identity mapping for the "Adobe-Identity" and
        // "Adobe-UCS" collections
        if (!collection->compare("Adobe-Identity") || !collection->compare("Adobe-UCS")) {
            ctu = CharCodeToUnicode::makeIdentityMapping();
        } else {
            // look for a user-supplied .cidToUnicode file
            if (!(ctu = globalParams->getCIDToUnicode(collection->toStr()))) {
                // I'm not completely sure that this is the best thing to do
                // but it seems to produce better results when the .cidToUnicode
                // files from the poppler-data package are missing. At least
                // we know that assuming the Identity mapping is definitely wrong.
                //   -- jrmuizel
                static const char *knownCollections[] = {
                    "Adobe-CNS1", "Adobe-GB1", "Adobe-Japan1", "Adobe-Japan2", "Adobe-Korea1",
                };
                for (const char *knownCollection : knownCollections) {
                    if (collection->compare(knownCollection) == 0) {
                        error(errSyntaxError, -1, "Missing language pack for '{0:t}' mapping", collection.get());
                        return;
                    }
                }
                error(errSyntaxError, -1, "Unknown character collection '{0:t}'", collection.get());
                // fall-through, assuming the Identity mapping -- this appears
                // to match Adobe's behavior
            }
        }
    }

    // encoding (i.e., CMap)
    obj1 = fontDict.lookup("Encoding");
    if (obj1.isNull()) {
        error(errSyntaxError, -1, "Missing Encoding entry in Type 0 font");
        return;
    }
    if (!(cMap = CMap::parse(collection->toStr(), &obj1))) {
        return;
    }
    if (cMap->getCMapName()) {
        encodingName = cMap->getCMapName()->toStr();
    } else {
        encodingName = "Custom";
    }

    // CIDToGIDMap (for embedded TrueType fonts)
    obj1 = desFontDict->lookup("CIDToGIDMap");
    if (obj1.isStream() && obj1.streamRewind()) {
        while ((c1 = obj1.streamGetChar()) != EOF && (c2 = obj1.streamGetChar()) != EOF) {
            cidToGID.push_back((c1 << 8) + c2);
        }
    } else if (!obj1.isName("Identity") && !obj1.isNull()) {
        error(errSyntaxError, -1, "Invalid CIDToGIDMap entry in CID font");
    }

    //----- character metrics -----

    // default char width
    obj1 = desFontDict->lookup("DW");
    if (obj1.isInt()) {
        widths.defWidth = obj1.getInt() * 0.001;
    }

    // char width exceptions
    obj1 = desFontDict->lookup("W");
    if (obj1.isArray()) {
        int i = 0;
        while (i + 1 < obj1.arrayGetLength()) {
            obj2 = obj1.arrayGet(i);
            obj3 = obj1.arrayGet(i + 1);
            if (obj2.isInt() && obj3.isInt() && i + 2 < obj1.arrayGetLength()) {
                obj4 = obj1.arrayGet(i + 2);
                if (obj4.isNum()) {
                    GfxFontCIDWidthExcep excep { .first = static_cast<CID>(obj2.getInt()), .last = static_cast<CID>(obj3.getInt()), .width = obj4.getNum() * 0.001 };
                    widths.exceps.push_back(excep);
                } else {
                    error(errSyntaxError, -1, "Bad widths array in Type 0 font");
                }
                i += 3;
            } else if (obj2.isInt() && obj3.isArray()) {
                int j = obj2.getInt();
                if (likely(j < INT_MAX - obj3.arrayGetLength())) {
                    for (int k = 0; k < obj3.arrayGetLength(); ++k) {
                        obj4 = obj3.arrayGet(k);
                        if (obj4.isNum()) {
                            GfxFontCIDWidthExcep excep { .first = static_cast<CID>(j), .last = static_cast<CID>(j), .width = obj4.getNum() * 0.001 };
                            widths.exceps.push_back(excep);
                            ++j;
                        } else {
                            error(errSyntaxError, -1, "Bad widths array in Type 0 font");
                        }
                    }
                }
                i += 2;
            } else {
                error(errSyntaxError, -1, "Bad widths array in Type 0 font");
                ++i;
            }
        }
        std::ranges::sort(widths.exceps, cmpWidthExcepFunctor());
    }

    // default metrics for vertical font
    obj1 = desFontDict->lookup("DW2");
    if (obj1.isArrayOfLength(2)) {
        obj2 = obj1.arrayGet(0);
        if (obj2.isNum()) {
            widths.defVY = obj2.getNum() * 0.001;
        }
        obj2 = obj1.arrayGet(1);
        if (obj2.isNum()) {
            widths.defHeight = obj2.getNum() * 0.001;
        }
    }