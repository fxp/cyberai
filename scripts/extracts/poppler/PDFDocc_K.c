// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 11/19



    switch (obj->getType()) {
    case objBool:
        outStr->printf("%s ", obj->getBool() ? "true" : "false");
        break;
    case objInt:
        outStr->printf("%i ", obj->getInt());
        break;
    case objInt64:
        outStr->printf("%lli ", obj->getInt64());
        break;
    case objReal: {
        GooString s;
        s.appendf("{0:.10g}", obj->getReal());
        outStr->printf("%s ", s.c_str());
        break;
    }
    case objString:
        writeString(obj->getString(), outStr, fileKey, encAlgorithm, keyLength, ref);
        break;
    case objHexString: {
        const std::string &s = obj->getHexString();
        outStr->printf("<");
        for (auto c : s) {
            outStr->printf("%02x", c & 0xff);
        }
        outStr->printf("> ");
        break;
    }
    case objName: {
        const std::string &name(obj->getNameString());
        outStr->printf("/%s ", sanitizedName(name).c_str());
        break;
    }
    case objNull:
        outStr->printf("null ");
        break;
    case objArray:
        array = obj->getArray();
        outStr->printf("[");
        for (int i = 0; i < array->getLength(); i++) {
            Object obj1 = array->getNF(i).copy();
            writeObject(&obj1, outStr, xRef, numOffset, fileKey, encAlgorithm, keyLength, ref);
        }
        outStr->printf("] ");
        break;
    case objDict:
        writeDictionary(obj->getDict(), outStr, xRef, numOffset, fileKey, encAlgorithm, keyLength, ref, alreadyWrittenDicts);
        break;
    case objStream: {
        // We can't modify stream with the current implementation (no write functions in Stream API)
        // => the only type of streams which that have been modified are internal streams (=strWeird)
        Stream *stream = obj->getStream();
        if (stream->getKind() == strWeird || stream->getKind() == strCrypt) {
            // we write the stream unencoded => TODO: write stream encoder

            // Encrypt stream
            bool removeFilter = true;
            bool addEncryptstream = false;
            if (stream->getKind() == strWeird && fileKey) {
                Object filter = stream->getDict()->lookup("Filter");
                if (!filter.isName("Crypt")) {
                    if (filter.isArray()) {
                        for (int i = 0; i < filter.arrayGetLength(); i++) {
                            Object filterEle = filter.arrayGet(i);
                            if (filterEle.isName("Crypt")) {
                                removeFilter = false;
                                break;
                            }
                        }
                        if (removeFilter) {
                            addEncryptstream = true;
                        }
                    } else {
                        addEncryptstream = true;
                    }
                } else {
                    removeFilter = false;
                }
            } else if (fileKey != nullptr) { // Encrypt stream
                addEncryptstream = true;
            }

            std::unique_ptr<EncryptStream> encStream;
            std::unique_ptr<Stream> compressStream;
            Object filter = stream->getDict()->lookup("Filter");
            if (filter.isName("FlateDecode")) {
                compressStream = std::make_unique<FlateEncoder>(stream);
                stream = compressStream.get();
                removeFilter = false;
            }
            if (addEncryptstream) {
                encStream = std::make_unique<EncryptStream>(*stream, fileKey, encAlgorithm, keyLength, ref);
            }

            if (!stream->rewind()) {
                break;
            }
            // recalculate stream length
            Goffset tmp = 0;
            for (int c = stream->getChar(); c != EOF; c = stream->getChar()) {
                tmp++;
            }
            stream->getDict()->set("Length", Object(tmp));

            // Remove Stream encoding
            auto *internalStream = dynamic_cast<AutoFreeMemStream *>(stream);
            if (internalStream && internalStream->isFilterRemovalForbidden()) {
                removeFilter = false;
            }
            if (removeFilter) {
                stream->getDict()->remove("Filter");
            }
            stream->getDict()->remove("DecodeParms");