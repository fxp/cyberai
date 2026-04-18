// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 10/19



    outStr->printf("<<");
    for (int i = 0; i < dict->getLength(); i++) {
        std::string keyName(dict->getKey(i));
        outStr->printf("/%s ", sanitizedName(keyName).c_str());
        Object obj1 = dict->getValNF(i).copy();
        writeObject(&obj1, outStr, xRef, numOffset, fileKey, encAlgorithm, keyLength, ref, alreadyWrittenDicts);
    }
    outStr->printf(">> ");

    if (deleteSet) {
        delete alreadyWrittenDicts;
    }
}

void PDFDoc::writeStream(Stream *str, OutStream *outStr)
{
    if (!str->rewind()) {
        return;
    }
    outStr->printf("stream\r\n");
    for (int c = str->getChar(); c != EOF; c = str->getChar()) {
        outStr->printf("%c", c);
    }
    outStr->printf("\r\nendstream\r\n");
}

void PDFDoc::writeRawStream(Stream *str, OutStream *outStr)
{
    Object obj1 = str->getDict()->lookup("Length");
    if (!obj1.isInt() && !obj1.isInt64()) {
        error(errSyntaxError, -1, "PDFDoc::writeRawStream, no Length in stream dict");
        return;
    }

    Goffset length;
    if (obj1.isInt()) {
        length = obj1.getInt();
    } else {
        length = obj1.getInt64();
    }

    outStr->printf("stream\r\n");
    if (!str->unfilteredRewind()) {
        error(errSyntaxError, -1, "PDFDoc::writeRawStream, rewind failed");
        return;
    }
    for (Goffset i = 0; i < length; i++) {
        int c = str->getUnfilteredChar();
        if (unlikely(c == EOF)) {
            error(errSyntaxError, -1, "PDFDoc::writeRawStream: EOF reading stream");
            break;
        }
        outStr->printf("%c", c);
    }
    (void)str->rewind();
    outStr->printf("\r\nendstream\r\n");
}

void PDFDoc::writeString(const std::string &s, OutStream *outStr, const unsigned char *fileKey, CryptAlgorithm encAlgorithm, int keyLength, Ref ref)
{
    // Encrypt string if encryption is enabled
    std::string sCopy;
    if (fileKey) {
        auto *enc = new EncryptStream(std::make_unique<MemStream>(s.c_str(), 0, s.size(), Object::null()), fileKey, encAlgorithm, keyLength, ref);
        std::string sEnc;
        int c;
        if (!enc->rewind()) {
            return;
        }
        while ((c = enc->getChar()) != EOF) {
            sEnc.push_back((char)c);
        }

        delete enc;
        sCopy = sEnc;
    } else {
        sCopy = s;
    }

    // Write data
    if (hasUnicodeByteOrderMark(sCopy)) {
        // unicode string don't necessary end with \0
        const char *c = sCopy.c_str();
        std::stringstream stream;
        stream << std::setfill('0') << std::hex;
        for (size_t i = 0; i < sCopy.size(); i++) {
            stream << std::setw(2) << (0xff & (unsigned int)*(c + i));
        }
        outStr->printf("<");
        outStr->printf("%s", stream.str().c_str());
        outStr->printf("> ");
    } else {
        const char *c = sCopy.c_str();
        outStr->printf("(");
        for (size_t i = 0; i < sCopy.size(); i++) {
            char unescaped = *(c + i) & 0x000000ff;
            // escape if needed
            if (unescaped == '\r') {
                outStr->printf("\\r");
            } else if (unescaped == '\n') {
                outStr->printf("\\n");
            } else {
                if (unescaped == '(' || unescaped == ')' || unescaped == '\\') {
                    outStr->printf("%c", '\\');
                }
                outStr->printf("%c", unescaped);
            }
        }
        outStr->printf(") ");
    }
}

Goffset PDFDoc::writeObjectHeader(Ref *ref, OutStream *outStr)
{
    Goffset offset = outStr->getPos();
    outStr->printf("%i %i obj\r\n", ref->num, ref->gen);
    return offset;
}

void PDFDoc::writeObject(Object *obj, OutStream *outStr, XRef *xRef, unsigned int numOffset, const unsigned char *fileKey, CryptAlgorithm encAlgorithm, int keyLength, int objNum, int objGen, std::set<Dict *> *alreadyWrittenDicts)
{
    writeObject(obj, outStr, xRef, numOffset, fileKey, encAlgorithm, keyLength, { .num = objNum, .gen = objGen }, alreadyWrittenDicts);
}

void PDFDoc::writeObject(Object *obj, OutStream *outStr, XRef *xRef, unsigned int numOffset, const unsigned char *fileKey, CryptAlgorithm encAlgorithm, int keyLength, Ref ref, std::set<Dict *> *alreadyWrittenDicts)
{
    Array *array;