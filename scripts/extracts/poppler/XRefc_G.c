// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 7/13



    intptr_t bufPos = start;
    char *p = buf;
    char *end = buf;
    bool startOfLine = true;
    bool space = true;
    bool eof = false;
    while (true) {
        if (end - p < 256 && !eof) {
            memcpy(buf, p, end - p);
            bufPos += p - buf;
            p = buf + (end - p);
            int n = (int)(buf + 4096 - p);
            int m = str->doGetChars(n, (unsigned char *)p);
            end = p + m;
            *end = '\0';
            p = buf;
            eof = m < n;
        }
        if (p == end && eof) {
            break;
        }
        if (startOfLine && !strncmp(p, "trailer", 7)) {
            constructTrailerDict((intptr_t)(bufPos + (p + 7 - buf)), needCatalogDict);
            p += 7;
            startOfLine = false;
            space = false;
        } else if (startOfLine && !strncmp(p, "endstream", 9)) {
            if (streamEndsLen == streamEndsSize) {
                streamEndsSize += 64;
                streamEnds = (Goffset *)greallocn(streamEnds, streamEndsSize, sizeof(Goffset));
            }
            streamEnds[streamEndsLen++] = ((Goffset)bufPos + (p - buf));
            p += 9;
            startOfLine = false;
            space = false;
        } else if (space && *p >= '0' && *p <= '9') {
            p = constructObjectEntry(p, ((Goffset)bufPos + (p - buf)), &lastObjNum);
            startOfLine = false;
            space = false;
        } else if (p[0] == '>' && p[1] == '>') {
            p += 2;
            startOfLine = false;
            space = false;
            // skip any PDF whitespace except for '\0'
            while (*p == '\t' || *p == '\n' || *p == '\x0c' || *p == '\r' || *p == ' ') {
                if (*p == '\n' || *p == '\r') {
                    startOfLine = true;
                }
                space = true;
                ++p;
            }
            if (!strncmp(p, "stream", 6)) {
                if (lastObjNum >= 0) {
                    if (streamObjNumsLen == streamObjNumsSize) {
                        streamObjNumsSize += 64;
                        streamObjNums = (int *)greallocn(streamObjNums, streamObjNumsSize, sizeof(int));
                    }
                    streamObjNums[streamObjNumsLen++] = lastObjNum;
                }
                p += 6;
                startOfLine = false;
                space = false;
            }
        } else {
            if (*p == '\n' || *p == '\r') {
                startOfLine = true;
                space = true;
            } else if (Lexer::isSpace(*p & 0xff)) {
                space = true;
            } else {
                startOfLine = false;
                space = false;
            }
            ++p;
        }
    }

    // read each stream object, check for xref or object stream
    for (int i = 0; i < streamObjNumsLen; ++i) {
        Object obj = fetch(streamObjNums[i], entries[streamObjNums[i]].gen);
        if (obj.isStream()) {
            Dict *dict = obj.streamGetDict();
            Object type = dict->lookup("Type");
            if (type.isName("XRef")) {
                saveTrailerDict(dict, true, needCatalogDict);
            } else if (type.isName("ObjStm")) {
                constructObjectStreamEntries(&obj, streamObjNums[i]);
            }
        }
    }

    gfree(streamObjNums);

    if (rootNum < 0) {
        error(errSyntaxError, -1, "Couldn't find trailer dictionary");
        return false;
    }
    return true;
}

// Attempt to construct a trailer dict at [pos] in the stream.
void XRef::constructTrailerDict(Goffset pos, bool needCatalogDict)
{
    Parser parser { nullptr,

                    str->makeSubStream(pos, false, 0, Object {}), false };
    Object newTrailerDict = parser.getObj();
    if (newTrailerDict.isDict()) {
        saveTrailerDict(newTrailerDict.getDict(), false, needCatalogDict);
    }
}

// If [dict] "looks like" a trailer dict (i.e., has a Root entry),
// save it as the trailer dict.
void XRef::saveTrailerDict(Dict *dict, bool isXRefStream, bool needCatalogDict)
{
    const Object &obj = dict->lookupNF("Root");
    if (obj.isRef() && (rootNum == -1 || !needCatalogDict)) {
        int newRootNum = obj.getRefNum();
        // the xref stream scanning code runs after all objects are found,
        // so we can check for a valid root object number at that point
        if (!isXRefStream || newRootNum <= last) {
            rootNum = newRootNum;
            rootGen = obj.getRefGen();
            trailerDict = Object { dict->copy(this) };
        }
    }
}