// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 7/24



    const auto bufSize = static_cast<size_t>(objEnd - objStart);
    if (Gfseek(f, objStart, SEEK_SET) != 0) {
        return false;
    }
    std::vector<char> buf(bufSize + 1);
    if (fread(buf.data(), 1, bufSize, f) != bufSize) {
        return false;
    }
    buf[bufSize] = 0; // prevent string functions from searching past the end

    // search for the Contents field which contains the signature placeholder
    // which always must start with hex digits 000
    *sigStart = -1;
    *sigEnd = -1;
    for (size_t i = 0; i < bufSize - 14; i++) {
        if (buf[i] == '/' && strncmp(&buf[i], "/Contents <000", 14) == 0) {
            *sigStart = objStart + i + 10;
            char *p = strchr(&buf[i], '>');
            if (p) {
                *sigEnd = objStart + (p - buf.data()) + 1;
            }
            break;
        }
    }

    if (*sigStart == -1 || *sigEnd == -1) {
        return false;
    }

    // Search for ByteRange array and update offsets
    for (size_t i = 0; i < bufSize - 10; i++) {
        if (buf[i] == '/' && strncmp(&buf[i], "/ByteRange", 10) == 0) {
            // update range
            char *p = setNextOffset(&buf[i], *sigStart);
            if (!p) {
                return false;
            }
            p = setNextOffset(p, *sigEnd);
            if (!p) {
                return false;
            }
            p = setNextOffset(p, *fileSize - *sigEnd);
            if (!p) {
                return false;
            }
            break;
        }
    }

    // write buffer back to disk
    if (Gfseek(f, objStart, SEEK_SET) != 0) {
        return false;
    }
    fwrite(buf.data(), bufSize, 1, f);
    return true;
}

// Overwrite signature string in the file with new signature
bool FormWidgetSignature::updateSignature(FILE *f, Goffset sigStart, Goffset sigEnd, const std::vector<unsigned char> &signature)
{
    if (signature.size() * 2 + 2 != size_t(sigEnd - sigStart)) {
        return false;
    }

    if (Gfseek(f, sigStart, SEEK_SET) != 0) {
        return false;
    }
    fprintf(f, "<");
    for (unsigned char value : signature) {
        fprintf(f, "%2.2x", value);
    }
    fprintf(f, "> ");
    return true;
}

bool FormWidgetSignature::createSignature(Object &vObj, Ref vRef, const GooString &name, int placeholderLength, const GooString *reason, const GooString *location, CryptoSign::SignatureType signatureType)
{
    vObj.dictAdd("Type", Object(objName, "Sig"));
    vObj.dictAdd("Filter", Object(objName, "Adobe.PPKLite"));
    vObj.dictAdd("SubFilter", Object(objName, toStdString(signatureType).c_str()));
    vObj.dictAdd("Name", Object(name.copy()));
    vObj.dictAdd("M", Object(timeToDateString(nullptr)));
    if (reason && !reason->empty()) {
        vObj.dictAdd("Reason", Object(reason->copy()));
    }
    if (location && !location->empty()) {
        vObj.dictAdd("Location", Object(location->copy()));
    }

    vObj.dictAdd("Contents", Object(objHexString, std::string(placeholderLength, '\0')));
    Object bObj(std::make_unique<Array>(xref));
    // reserve space in byte range for maximum number of bytes
    bObj.arrayAdd(Object(0LL));
    bObj.arrayAdd(Object(9999999999LL));
    bObj.arrayAdd(Object(9999999999LL));
    bObj.arrayAdd(Object(9999999999LL));
    vObj.dictAdd("ByteRange", bObj.copy());
    field->getObj()->dictSet("V", Object(vRef));
    xref->setModifiedObject(field->getObj(), field->getRef());
    return true;
}

std::vector<Goffset> FormWidgetSignature::getSignedRangeBounds() const
{
    return static_cast<FormFieldSignature *>(field)->getSignedRangeBounds();
}

std::pair<std::optional<std::vector<unsigned char>>, int64_t> FormWidgetSignature::getCheckedSignature()
{
    return static_cast<FormFieldSignature *>(field)->getCheckedSignature();
}

void FormWidgetSignature::updateWidgetAppearance()
{
    if (widget) {
        widget->updateAppearanceStream();
    }
}

//========================================================================
// FormField
//========================================================================

FormField::FormField(PDFDoc *docA, Object &&aobj, const Ref aref, FormField *parentA, std::set<int> *usedParents, FormFieldType ty)
{
    doc = docA;
    xref = doc->getXRef();
    obj = std::move(aobj);
    Dict *dict = obj.getDict();
    ref = aref;
    type = ty;
    parent = parentA;
    terminal = false;
    readOnly = false;
    fullyQualifiedName = nullptr;
    quadding = VariableTextQuadding::leftJustified;
    hasQuadding = false;
    standAlone = false;
    noExport = false;