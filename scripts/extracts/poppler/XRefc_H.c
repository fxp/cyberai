// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 8/13



// Look for an object header ("nnn ggg obj") at [p].  The first
// character at *[p] is a digit.  [pos] is the position of *[p].
char *XRef::constructObjectEntry(char *p, Goffset pos, int *objNum)
{
    // we look for non-end-of-line space characters here, to deal with
    // situations like:
    //    nnn          <-- garbage digits on a line
    //    nnn nnn obj  <-- actual object
    // and we also ignore '\0' (because it's used to terminate the
    // buffer in this damage-scanning code)
    int num = 0;
    do {
        num = (num * 10) + (*p - '0');
        ++p;
    } while (*p >= '0' && *p <= '9' && num < 100000000);
    if (*p != '\t' && *p != '\x0c' && *p != ' ') {
        return p;
    }
    do {
        ++p;
    } while (*p == '\t' || *p == '\x0c' || *p == ' ');
    if (*p < '0' || *p > '9') {
        return p;
    }
    int gen = 0;
    do {
        gen = (gen * 10) + (*p - '0');
        ++p;
    } while (*p >= '0' && *p <= '9' && gen < 100000000);
    if (*p != '\t' && *p != '\x0c' && *p != ' ') {
        return p;
    }
    do {
        ++p;
    } while (*p == '\t' || *p == '\x0c' || *p == ' ');
    if (strncmp(p, "obj", 3) != 0) {
        return p;
    }

    if (constructXRefEntry(num, gen, pos - start, xrefEntryUncompressed)) {
        *objNum = num;
    }

    return p;
}

// Read the header from an object stream, and add xref entries for all
// of its objects.
void XRef::constructObjectStreamEntries(Object *objStr, int objStrObjNum)
{
    Object objN = objStr->streamGetDict()->lookup("N");

    // get the object count
    if (!objN.isInt()) {
        return;
    }
    int nObjects = objN.getInt();
    if (nObjects <= 0 || nObjects > 1000000) {
        return;
    }

    // parse the header: object numbers and offsets
    Parser parser { nullptr, objStr, false };
    for (int i = 0; i < nObjects; ++i) {
        auto obj1 = parser.getObj(true);
        auto obj2 = parser.getObj(true);
        if (obj1.isInt() && obj2.isInt()) {
            int num = obj1.getInt();
            if (num >= 0 && num < 1000000) {
                constructXRefEntry(num, i, objStrObjNum, xrefEntryCompressed);
            }
        }
    }
}

bool XRef::constructXRefEntry(int num, int gen, Goffset pos, XRefEntryType type)
{
    if (num >= size) {
        const int newSize = (num + 1 + 255) & ~255;
        if (newSize < 0) {
            return false;
        }
        if (resize(newSize) != newSize) {
            return false;
        }
    }

    if (entries[num].type == xrefEntryFree || gen >= entries[num].gen) {
        entries[num].offset = pos;
        entries[num].gen = gen;
        entries[num].type = type;
        if (num > last) {
            last = num;
        }
    }

    return true;
}

void XRef::setEncryption(int permFlagsA, bool ownerPasswordOkA, const unsigned char *fileKeyA, int keyLengthA, int encVersionA, int encRevisionA, CryptAlgorithm encAlgorithmA)
{
    int i;

    encrypted = true;
    permFlags = permFlagsA;
    ownerPasswordOk = ownerPasswordOkA;
    if (keyLengthA <= 32) {
        keyLength = keyLengthA;
    } else {
        keyLength = 32;
    }
    for (i = 0; i < keyLength; ++i) {
        fileKey[i] = fileKeyA[i];
    }
    encVersion = encVersionA;
    encRevision = encRevisionA;
    encAlgorithm = encAlgorithmA;
}

void XRef::getEncryptionParameters(unsigned char **fileKeyA, CryptAlgorithm *encAlgorithmA, int *keyLengthA)
{
    if (encrypted) {
        *fileKeyA = fileKey;
        *encAlgorithmA = encAlgorithm;
        *keyLengthA = keyLength;
    } else {
        // null encryption parameters
        *fileKeyA = nullptr;
        *encAlgorithmA = cryptRC4;
        *keyLengthA = 0;
    }
}

bool XRef::isRefEncrypted(Ref r)
{
    xrefLocker();

    const XRefEntry *e = getEntry(r.num);
    if (!e->obj.isNull()) { // check for updated object
        return false;
    }

    switch (e->type) {
    case xrefEntryUncompressed: {
        return encrypted && !e->getFlag(XRefEntry::Unencrypted);
    }

    case xrefEntryCompressed: {
        const Goffset objStrNum = e->offset;
        if (unlikely(objStrNum < 0 || objStrNum >= size)) {
            error(errSyntaxError, -1, "XRef::isRefEncrypted - Compressed object offset out of xref bounds");
            return false;
        }
        const Object objStr = fetch(static_cast<int>(e->offset), 0);
        return objStr.getStream()->isEncrypted();
    }

    default: {
    }
    }

    return false;
}

bool XRef::okToPrint(bool ignoreOwnerPW) const
{
    return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permPrint);
}