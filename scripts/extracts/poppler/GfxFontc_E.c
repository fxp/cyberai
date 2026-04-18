// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 5/20



    Object fontDesc = fontDict2->lookup("FontDescriptor");
    if (fontDesc.isDict()) {
        Object obj3 = fontDesc.dictLookupNF("FontFile").copy();
        if (obj3.isRef()) {
            *embID = obj3.getRef();
            if (expectedType != fontType1) {
                err = true;
            }
        }
        if (*embID == Ref::INVALID() && (obj3 = fontDesc.dictLookupNF("FontFile2").copy(), obj3.isRef())) {
            *embID = obj3.getRef();
            if (isType0) {
                expectedType = fontCIDType2;
            } else if (expectedType != fontTrueType) {
                err = true;
            }
        }
        if (*embID == Ref::INVALID() && (obj3 = fontDesc.dictLookupNF("FontFile3").copy(), obj3.isRef())) {
            *embID = obj3.getRef();
            Object obj4 = obj3.fetch(xref);
            if (obj4.isStream()) {
                subtype = obj4.streamGetDict()->lookup("Subtype");
                if (subtype.isName("Type1")) {
                    if (expectedType != fontType1) {
                        err = true;
                        expectedType = isType0 ? fontCIDType0 : fontType1;
                    }
                } else if (subtype.isName("Type1C")) {
                    if (expectedType == fontType1) {
                        expectedType = fontType1C;
                    } else if (expectedType != fontType1C) {
                        err = true;
                        expectedType = isType0 ? fontCIDType0C : fontType1C;
                    }
                } else if (subtype.isName("TrueType")) {
                    if (expectedType != fontTrueType) {
                        err = true;
                        expectedType = isType0 ? fontCIDType2 : fontTrueType;
                    }
                } else if (subtype.isName("CIDFontType0C")) {
                    if (expectedType == fontCIDType0) {
                        expectedType = fontCIDType0C;
                    } else {
                        err = true;
                        expectedType = isType0 ? fontCIDType0C : fontType1C;
                    }
                } else if (subtype.isName("OpenType")) {
                    if (expectedType == fontTrueType) {
                        expectedType = fontTrueTypeOT;
                    } else if (expectedType == fontType1) {
                        expectedType = fontType1COT;
                    } else if (expectedType == fontCIDType0) {
                        expectedType = fontCIDType0COT;
                    } else if (expectedType == fontCIDType2) {
                        expectedType = fontCIDType2OT;
                    } else {
                        err = true;
                    }
                } else {
                    error(errSyntaxError, -1, "Unknown font type '{0:s}'", subtype.isName() ? subtype.getName() : "???");
                }
            }
        }
    }

    t = fontUnknownType;
    if (*embID != Ref::INVALID()) {
        Object obj3(*embID);
        Object obj4 = obj3.fetch(xref);
        if (obj4.isStream() && obj4.streamRewind()) {
            fft = FoFiIdentifier::identifyStream(&readFromStream, obj4.getStream());
            obj4.streamClose();
            switch (fft) {
            case fofiIdType1PFA:
            case fofiIdType1PFB:
                t = fontType1;
                break;
            case fofiIdCFF8Bit:
                t = isType0 ? fontCIDType0C : fontType1C;
                break;
            case fofiIdCFFCID:
                t = fontCIDType0C;
                break;
            case fofiIdTrueType:
            case fofiIdTrueTypeCollection:
                t = isType0 ? fontCIDType2 : fontTrueType;
                break;
            case fofiIdOpenTypeCFF8Bit:
                t = isType0 ? fontCIDType0COT : fontType1COT;
                break;
            case fofiIdOpenTypeCFFCID:
                t = fontCIDType0COT;
                break;
            default:
                error(errSyntaxError, -1, "Embedded font file may be invalid");
                break;
            }
        }
    }

    if (t == fontUnknownType) {
        t = expectedType;
    }

    if (t != expectedType) {
        err = true;
    }

    if (err) {
        error(errSyntaxWarning, -1, "Mismatch between font type and embedded font file");
    }

    return t;
}

void GfxFont::readFontDescriptor(const Dict &fontDict)
{
    double t;

    // assume Times-Roman by default (for substitution purposes)
    flags = fontSerif;

    missingWidth = 0;

    Object obj1 = fontDict.lookup("FontDescriptor");
    if (obj1.isDict()) {

        // get flags
        Object obj2 = obj1.dictLookup("Flags");
        if (obj2.isInt()) {
            flags = obj2.getInt();
        }