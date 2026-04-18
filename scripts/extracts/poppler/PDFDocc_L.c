// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 12/19



            writeDictionary(stream->getDict(), outStr, xRef, numOffset, fileKey, encAlgorithm, keyLength, ref, alreadyWrittenDicts);
            writeStream(stream, outStr);
        } else if (fileKey != nullptr && stream->getKind() == strFile && static_cast<FileStream *>(stream)->getNeedsEncryptionOnSave()) {
            auto *encStream = new EncryptStream(*stream, fileKey, encAlgorithm, keyLength, ref);
            writeDictionary(encStream->getDict(), outStr, xRef, numOffset, fileKey, encAlgorithm, keyLength, ref, alreadyWrittenDicts);
            writeStream(encStream, outStr);
            delete encStream;
        } else {
            // raw stream copy
            auto *fs = dynamic_cast<FilterStream *>(stream);
            if (fs) {
                BaseStream *bs = fs->getBaseStream();
                if (bs) {
                    Goffset streamEnd;
                    if (xRef->getStreamEnd(bs->getStart(), &streamEnd)) {
                        Goffset val = streamEnd - bs->getStart();
                        stream->getDict()->set("Length", Object(val));
                    }
                }
            }
            writeDictionary(stream->getDict(), outStr, xRef, numOffset, fileKey, encAlgorithm, keyLength, ref, alreadyWrittenDicts);
            writeRawStream(stream, outStr);
        }
        break;
    }
    case objRef:
        outStr->printf("%i %i R ", obj->getRef().num + numOffset, obj->getRef().gen);
        break;
    case objCmd:
        outStr->printf("%s\n", obj->getCmd());
        break;
    case objError:
        outStr->printf("error\r\n");
        break;
    case objEOF:
        outStr->printf("eof\r\n");
        break;
    case objNone:
        outStr->printf("none\r\n");
        break;
    default:
        error(errUnimplemented, -1, "Unhandled objType : {0:d}, please report a bug with a testcase", obj->getType());
        break;
    }
}

void PDFDoc::writeObjectFooter(OutStream *outStr)
{
    outStr->printf("\r\nendobj\r\n");
}

Object PDFDoc::createTrailerDict(int uxrefSize, bool incrUpdate, Goffset startxRef, Ref *root, XRef *xRef, const char *fileName, Goffset fileSize)
{
    auto trailerDict = std::make_unique<Dict>(xRef);
    trailerDict->set("Size", Object(uxrefSize));

    // build a new ID, as recommended in the reference, uses:
    // - current time
    // - file name
    // - file size
    // - values of entry in information dictionary
    GooString message;
    char buffer[256];
    sprintf(buffer, "%i", (int)time(nullptr));
    message.append(buffer);

    if (fileName) {
        message.append(fileName);
    }

    sprintf(buffer, "%lli", (long long)fileSize);
    message.append(buffer);

    // info dict -- only use text string
    if (!xRef->getTrailerDict()->isNone()) {
        Object docInfo = xRef->getDocInfo();
        if (docInfo.isDict()) {
            for (int i = 0; i < docInfo.getDict()->getLength(); i++) {
                Object obj2 = docInfo.getDict()->getVal(i);
                if (obj2.isString()) {
                    message.append(obj2.getString());
                }
            }
        }
    }

    bool hasEncrypt = false;
    if (!xRef->getTrailerDict()->isNone()) {
        Object obj2 = xRef->getTrailerDict()->dictLookupNF("Encrypt").copy();
        if (!obj2.isNull()) {
            trailerDict->set("Encrypt", std::move(obj2));
            hasEncrypt = true;
        }
    }

    // calculate md5 digest
    unsigned char digest[16];
    md5((unsigned char *)message.c_str(), message.size(), digest);

    // create ID array
    // In case of encrypted files, the ID must not be changed because it's used to calculate the key
    if (incrUpdate || hasEncrypt) {
        // only update the second part of the array
        Object obj4 = xRef->getTrailerDict()->getDict()->lookup("ID");
        if (!obj4.isArray()) {
            if (hasEncrypt) {
                error(errSyntaxWarning, -1, "PDFDoc::createTrailerDict original file's ID entry isn't an array. Trying to continue");
            }
        } else {
            auto array = std::make_unique<Array>(xRef);
            // Get the first part of the ID
            array->add(obj4.arrayGet(0));
            array->add(Object(std::make_unique<GooString>((const char *)digest, 16)));
            trailerDict->set("ID", Object(std::move(array)));
        }
    } else {
        // new file => same values for the two identifiers
        auto array = std::make_unique<Array>(xRef);
        array->add(Object(std::make_unique<GooString>((const char *)digest, 16)));
        array->add(Object(std::make_unique<GooString>((const char *)digest, 16)));
        trailerDict->set("ID", Object(std::move(array)));
    }

    trailerDict->set("Root", Object(*root));

    if (incrUpdate) {
        trailerDict->set("Prev", Object(startxRef));
    }