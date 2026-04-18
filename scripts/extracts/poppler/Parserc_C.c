// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Parser.cc
// Segment 3/3



    // get length
    Object obj = dict.dictLookup("Length", recursion);
    if (obj.isInt()) {
        length = obj.getInt();
    } else if (obj.isInt64()) {
        length = obj.getInt64();
    } else {
        error(errSyntaxError, getPos(), "Bad 'Length' attribute in stream");
        if (strict) {
            return nullptr;
        }
        length = 0;
    }

    // check for length in damaged file
    if (lexer.hasXRef() && lexer.getXRef()->getStreamEnd(pos, &endPos)) {
        length = endPos - pos;
    }

    // in badly damaged PDF files, we can run off the end of the input
    // stream immediately after the "stream" token
    if (!lexer.getStream()) {
        return nullptr;
    }
    baseStr = lexer.getStream()->getBaseStream();

    // skip over stream data
    if (Lexer::LOOK_VALUE_NOT_CACHED != lexer.lookCharLastValueCached) {
        // take into account the fact that we've cached one value
        pos = pos - 1;
        lexer.lookCharLastValueCached = Lexer::LOOK_VALUE_NOT_CACHED;
    }
    if (unlikely(length < 0)) {
        return nullptr;
    }
    if (unlikely(pos > LLONG_MAX - length)) {
        return nullptr;
    }
    lexer.setPos(pos + length);

    // refill token buffers and check for 'endstream'
    shift(); // kill '>>'
    shift("endstream", objNum); // kill 'stream'
    if (buf1.isCmd("endstream")) {
        shift();
    } else {
        error(errSyntaxError, getPos(), "Missing 'endstream' or incorrect stream length");
        if (strict) {
            return nullptr;
        }
        if (lexer.hasXRef() && lexer.getStream()) {
            // shift until we find the proper endstream or we change to another object or reach eof
            length = lexer.getPos() - pos;
            if (buf1.isCmd("endstream")) {
                dict.dictSet("Length", Object(length));
            }
        } else {
            // When building the xref we can't use it so use this
            // kludge for broken PDF files: just add 5k to the length, and
            // hope its enough
            if (length < LLONG_MAX - pos - 5000) {
                length += 5000;
            }
        }
    }

    // make base stream
    auto str = baseStr->makeSubStream(pos, true, length, std::move(dict));

    // handle decryption
    if (fileKey) {
        str = std::make_unique<DecryptStream>(std::move(str), fileKey, encAlgorithm, keyLength, Ref { .num = objNum, .gen = objGen });
    }

    // get filters
    Dict *streamDict = str->getDict();
    str = Stream::addFilters(std::move(str), streamDict, recursion);

    if (XRef *xref = lexer.getXRef()) {
        // Don't try to reuse the entry from the block at the start
        // of the function, xref can change in the middle because of
        // reconstruction
        XRefEntry *entry = xref->getEntry(objNum, false);
        if (entry) {
            entry->setFlag(XRefEntry::Parsing, false);
        }
    }

    return str;
}

void Parser::shift(int objNum)
{
    if (inlineImg > 0) {
        if (inlineImg < 2) {
            ++inlineImg;
        } else {
            // in a damaged content stream, if 'ID' shows up in the middle
            // of a dictionary, we need to reset
            inlineImg = 0;
        }
    } else if (buf2.isCmd("ID")) {
        lexer.skipChar(); // skip char after 'ID' command
        inlineImg = 1;
    }
    buf1 = std::move(buf2);
    if (inlineImg > 0) { // don't buffer inline image data
        buf2.setToNull();
    } else {
        buf2 = lexer.getObj(objNum);
    }
}

void Parser::shift(const char *cmdA, int objNum)
{
    if (inlineImg > 0) {
        if (inlineImg < 2) {
            ++inlineImg;
        } else {
            // in a damaged content stream, if 'ID' shows up in the middle
            // of a dictionary, we need to reset
            inlineImg = 0;
        }
    } else if (buf2.isCmd("ID")) {
        lexer.skipChar(); // skip char after 'ID' command
        inlineImg = 1;
    }
    buf1 = std::move(buf2);
    if (inlineImg > 0) {
        buf2.setToNull();
    } else if (buf1.isCmd(cmdA)) {
        buf2 = lexer.getObj(objNum);
    } else {
        buf2 = lexer.getObj(cmdA, objNum);
    }
}
