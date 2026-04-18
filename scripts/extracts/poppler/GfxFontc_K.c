// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 11/20



    // merge differences into encoding
    if (obj1.isDict()) {
        Object obj2 = obj1.dictLookup("Differences");
        if (obj2.isArray()) {
            encodingName = "Custom";
            hasEncoding = true;
            int code = 0;
            for (int i = 0; i < obj2.arrayGetLength(); ++i) {
                Object obj3 = obj2.arrayGet(i);
                if (obj3.isInt()) {
                    code = obj3.getInt();
                } else if (obj3.isName()) {
                    if (code >= 0 && code < 256) {
                        if (encFree[code]) {
                            gfree(enc[code]);
                        }
                        enc[code] = copyString(obj3.getName());
                        encFree[code] = true;
                        ++code;
                    }
                } else {
                    error(errSyntaxError, -1, "Wrong type in font encoding resource differences ({0:s})", obj3.getTypeName());
                }
            }
        }
    }

    //----- build the mapping to Unicode -----

    // pass 1: use the name-to-Unicode mapping table
    missing = hex = false;
    for (int code = 0; code < 256; ++code) {
        if ((charName = enc[code])) {
            if (isZapfDingbats) {
                // include ZapfDingbats names
                toUnicode[code] = globalParams->mapNameToUnicodeAll(charName);
            } else {
                toUnicode[code] = globalParams->mapNameToUnicodeText(charName);
            }
            if (!toUnicode[code] && (strcmp(charName, ".notdef") != 0)) {
                // if it wasn't in the name-to-Unicode table, check for a
                // name that looks like 'Axx' or 'xx', where 'A' is any letter
                // and 'xx' is two hex digits
                if ((strlen(charName) == 3 && isalpha(charName[0]) && isxdigit(charName[1]) && isxdigit(charName[2])
                     && ((charName[1] >= 'a' && charName[1] <= 'f') || (charName[1] >= 'A' && charName[1] <= 'F') || (charName[2] >= 'a' && charName[2] <= 'f') || (charName[2] >= 'A' && charName[2] <= 'F')))
                    || (strlen(charName) == 2 && isxdigit(charName[0]) && isxdigit(charName[1]) &&
                        // Only check idx 1 to avoid misidentifying a decimal
                        // number like a0
                        ((charName[1] >= 'a' && charName[1] <= 'f') || (charName[1] >= 'A' && charName[1] <= 'F')))) {
                    hex = true;
                }
                missing = true;
            }
        } else {
            toUnicode[code] = 0;
        }
    }

    const bool numeric = testForNumericNames(fontDict, hex);

    // construct the char code -> Unicode mapping object
    ctu = CharCodeToUnicode::make8BitToUnicode(toUnicode);

    // pass 1a: Expand ligatures in the Alphabetic Presentation Form
    // block (eg "fi", "ffi") to normal form
    for (int code = 0; code < 256; ++code) {
        if (unicodeIsAlphabeticPresentationForm(toUnicode[code])) {
            Unicode *normalized = unicodeNormalizeNFKC(&toUnicode[code], 1, &len, nullptr);
            if (len > 1) {
                ctu->setMapping((CharCode)code, normalized, len);
            }
            gfree(normalized);
        }
    }

    // pass 2: try to fill in the missing chars, looking for ligatures, numeric
    // references and variants
    if (missing) {
        for (int code = 0; code < 256; ++code) {
            if (!toUnicode[code]) {
                if ((charName = enc[code]) && (strcmp(charName, ".notdef") != 0)
                    && (n = parseCharName(charName, uBuf, sizeof(uBuf) / sizeof(*uBuf),
                                          false, // don't check simple names (pass 1)
                                          true, // do check ligatures
                                          numeric, hex,
                                          true))) { // do check variants
                    ctu->setMapping((CharCode)code, uBuf, n);
                    continue;
                }

                // do a simple pass-through
                // mapping for unknown character names
                uBuf[0] = code;
                ctu->setMapping((CharCode)code, uBuf, 1);
            }
        }
    }

    // merge in a ToUnicode CMap, if there is one -- this overwrites
    // existing entries in ctu, i.e., the ToUnicode CMap takes
    // precedence, but the other encoding info is allowed to fill in any
    // holes
    ctu = readToUnicodeCMap(fontDict, 16, std::move(ctu));

    //----- get the character widths -----

    // initialize all widths
    for (double &width : widths) {
        width = missingWidth * 0.001;
    }