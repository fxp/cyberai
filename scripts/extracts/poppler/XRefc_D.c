// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 4/13



/* Read one xref table section.  Also reads the associated trailer
 * dictionary, and returns the prev pointer (if any).
 * Arguments:
 *   pos                Points to a Goffset containing the offset of the XRef
 *                      section to be read. If a prev pointer is found, *pos is
 *                      updated with its value
 *   followedXRefStm    Used in case of nested readXRef calls to spot circular
 *                      references in XRefStm pointers
 *   xrefStreamObjsNum  If not NULL, every time a XRef stream is encountered,
 *                      its object number is appended
 * Return value:
 *   true if a prev pointer is found, otherwise false
 */
bool XRef::readXRef(Goffset *pos, std::vector<Goffset> *followedXRefStm, std::vector<int> *xrefStreamObjsNum)
{
    Parser *parser;
    Object obj;
    bool more;

    Goffset parsePos;
    if (unlikely(checkedAdd(start, *pos, &parsePos))) {
        ok = false;
        return false;
    }
    if (parsePos < 0) {
        ok = false;
        return false;
    }

    // start up a parser, parse one token
    parser = new Parser(nullptr, str->makeSubStream(parsePos, false, 0, Object::null()), true);
    obj = parser->getObj(true);

    // parse an old-style xref table
    if (obj.isCmd("xref")) {
        more = readXRefTable(parser, pos, followedXRefStm, xrefStreamObjsNum);

        // parse an xref stream
    } else if (obj.isInt()) {
        const int objNum = obj.getInt();
        if (obj = parser->getObj(true), !obj.isInt()) {
            goto err1;
        }
        if (obj = parser->getObj(true), !obj.isCmd("obj")) {
            goto err1;
        }
        if (obj = parser->getObj(), !obj.isStream()) {
            goto err1;
        }
        if (trailerDict.isNone()) {
            xRefStream = true;
        }
        if (xrefStreamObjsNum) {
            xrefStreamObjsNum->push_back(objNum);
        }
        more = readXRefStream(obj.getStream(), pos);

    } else {
        goto err1;
    }

    delete parser;
    return more;

err1:
    delete parser;
    ok = false;
    return false;
}

bool XRef::readXRefTable(Parser *parser, Goffset *pos, std::vector<Goffset> *followedXRefStm, std::vector<int> *xrefStreamObjsNum)
{
    XRefEntry entry;
    bool more;
    Object obj, obj2;
    Goffset pos2;
    int first, n;

    while (true) {
        obj = parser->getObj(true);
        if (obj.isCmd("trailer")) {
            break;
        }
        if (!obj.isInt()) {
            goto err0;
        }
        first = obj.getInt();
        obj = parser->getObj(true);
        if (!obj.isInt()) {
            goto err0;
        }
        n = obj.getInt();
        if (first < 0 || n < 0 || first > INT_MAX - n) {
            goto err0;
        }
        if (first + n > size) {
            if (resize(first + n) != first + n) {
                error(errSyntaxError, -1, "Invalid 'obj' parameters'");
                goto err0;
            }
        }
        for (int i = first; i < first + n; ++i) {
            obj = parser->getObj(true);
            if (obj.isInt()) {
                entry.offset = obj.getInt();
            } else if (obj.isInt64()) {
                entry.offset = obj.getInt64();
            } else {
                goto err0;
            }
            obj = parser->getObj(true);
            if (!obj.isInt()) {
                goto err0;
            }
            entry.gen = obj.getInt();
            entry.flags = 0;
            obj = parser->getObj(true);
            if (obj.isCmd("n")) {
                entry.type = xrefEntryUncompressed;
            } else if (obj.isCmd("f")) {
                entry.type = xrefEntryFree;
            } else {
                goto err0;
            }
            if (entries[i].offset == -1) {
                entries[i].offset = entry.offset;
                entries[i].gen = entry.gen;
                entries[i].type = entry.type;
                entries[i].flags = entry.flags;
                entries[i].obj.setToNull();

                // PDF files of patents from the IBM Intellectual Property
                // Network have a bug: the xref table claims to start at 1
                // instead of 0.
                if (i == 1 && first == 1 && entries[1].offset == 0 && entries[1].gen == 65535 && entries[1].type == xrefEntryFree) {
                    i = first = 0;
                    entries[0].offset = 0;
                    entries[0].gen = 65535;
                    entries[0].type = xrefEntryFree;
                    entries[0].flags = entries[1].flags;
                    entries[0].obj = std::move(entries[1].obj);

                    entries[1].offset = -1;
                    entries[1].obj.setToNull();
                }
            }
            if (i > last) {
                last = i;
            }
        }
    }

    // read the trailer dictionary
    obj = parser->getObj();
    if (!obj.isDict()) {
        goto err0;
    }