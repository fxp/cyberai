// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Parser.cc
// Segment 2/3



        // dictionary or stream
    } else if (!simpleOnly && buf1.isCmd("<<")) {
        shift(objNum);
        obj = Object(std::make_unique<Dict>(lexer.getXRef()));
        bool hasContentsEntry = false;
        while (!buf1.isCmd(">>") && !buf1.isEOF()) {
            if (!buf1.isName()) {
                error(errSyntaxError, getPos(), "Dictionary key must be a name object");
                if (strict) {
                    goto err;
                }
                shift();
            } else {
                // buf1 will go away in shift(), so keep the key
                const auto key = std::move(buf1);
                shift();
                if (buf1.isEOF() || buf1.isError()) {
                    if (strict && buf1.isError()) {
                        goto err;
                    }
                    break;
                }
                // We don't decrypt strings that are the value of "Contents" key entries. We decrypt them if needed a few lines below.
                // The "Contents" field of Sig dictionaries is not encrypted, but we can't know the type of the dictionary here yet
                // so we don't decrypt any Contents and if later we find it's not a Sig dictionary we decrypt it
                const bool isContents = !hasContentsEntry && key.isName("Contents");
                hasContentsEntry = hasContentsEntry || isContents;
                Object obj2 = getObj(false, fileKey, encAlgorithm, keyLength, objNum, objGen, recursion + 1, /*strict*/ false, /*decryptString*/ !isContents);
                if (unlikely(recursion + 1 >= recursionLimit)) {
                    break;
                }
                obj.dictAdd(key.getName(), std::move(obj2));
            }
        }
        if (buf1.isEOF()) {
            error(errSyntaxError, getPos(), "End of file inside dictionary");
            if (strict) {
                goto err;
            }
        }
        if (fileKey && hasContentsEntry) {
            Dict *dict = obj.getDict();
            const bool isSigDict = dict->is("Sig");
            if (!isSigDict) {
                const Object &contentsObj = dict->lookupNF("Contents");
                if (contentsObj.isString()) {
                    std::unique_ptr<GooString> s = decryptedString(contentsObj.getString(), fileKey, encAlgorithm, keyLength, objNum, objGen);
                    dict->set("Contents", Object(std::move(s)));
                }
            }
        }
        // stream objects are not allowed inside content streams or
        // object streams
        if (buf2.isCmd("stream")) {
            if (allowStreams) {
                if (auto str = makeStream(std::move(obj), fileKey, encAlgorithm, keyLength, objNum, objGen, recursion + 1, strict)) {
                    return Object(std::move(str));
                }
            }
            return Object::error();
        }
        shift();

        // indirect reference or integer
    } else if (buf1.isInt()) {
        const int num = buf1.getInt();
        shift();
        if (buf1.isInt() && buf2.isCmd("R")) {
            const int gen = buf1.getInt();
            shift();
            shift();

            if (unlikely(num <= 0 || gen < 0)) {
                return Object();
            }

            Ref r;
            r.num = num;
            r.gen = gen;
            return Object(r);
        }
        return Object(num);

        // string
    } else if (decryptString && buf1.isString() && fileKey) {
        std::unique_ptr<GooString> s2 = decryptedString(buf1.getString(), fileKey, encAlgorithm, keyLength, objNum, objGen);
        obj = Object(std::move(s2));
        shift();

        // simple object
    } else {
        // avoid re-allocating memory for complex objects like strings by
        // shallow copy of <buf1> to <obj> and nulling <buf1> so that
        // subsequent buf1.free() won't free this memory
        obj = std::move(buf1);
        shift();
    }

    return obj;

err:
    return Object::error();
}

std::unique_ptr<Stream> Parser::makeStream(Object &&dict, const unsigned char *fileKey, CryptAlgorithm encAlgorithm, int keyLength, int objNum, int objGen, int recursion, bool strict)
{
    BaseStream *baseStr;
    Goffset length;
    Goffset pos, endPos;

    if (XRef *xref = lexer.getXRef()) {
        XRefEntry *entry = xref->getEntry(objNum, false);
        if (entry) {
            if (!entry->getFlag(XRefEntry::Parsing) || (objNum == 0 && objGen == 0)) {
                entry->setFlag(XRefEntry::Parsing, true);
            } else {
                error(errSyntaxError, getPos(), "Object '{0:d} {1:d} obj' is being already parsed", objNum, objGen);
                return nullptr;
            }
        }
    }

    // get stream start position
    lexer.skipToNextLine();
    Stream *lexerStream;
    if (!(lexerStream = lexer.getStream())) {
        return nullptr;
    }
    pos = lexerStream->getPos();