// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 9/13



// we can print at high res if we are only doing security handler revision
// 2 (and we are allowed to print at all), or with security handler rev
// 3 and we are allowed to print, and bit 12 is set.
bool XRef::okToPrintHighRes(bool ignoreOwnerPW) const
{
    if (encrypted) {
        if (2 == encRevision) {
            return (okToPrint(ignoreOwnerPW));
        }
        if (encRevision >= 3) {
            return (okToPrint(ignoreOwnerPW) && (permFlags & permHighResPrint));
        }
        // something weird - unknown security handler version
        return false;
    }
    return true;
}

bool XRef::okToChange(bool ignoreOwnerPW) const
{
    return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permChange);
}

bool XRef::okToCopy(bool ignoreOwnerPW) const
{
    return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permCopy);
}

bool XRef::okToAddNotes(bool ignoreOwnerPW) const
{
    return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permNotes);
}

bool XRef::okToFillForm(bool ignoreOwnerPW) const
{
    return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permFillForm);
}

bool XRef::okToAccessibility(bool ignoreOwnerPW) const
{
    return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permAccessibility);
}

bool XRef::okToAssemble(bool ignoreOwnerPW) const
{
    return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permAssemble);
}

Object XRef::getCatalog()
{
    Object catalog = fetch(rootNum, rootGen);
    if (catalog.isDict()) {
        return catalog;
    }
    bool wasReconstructed = false;
    if (constructXRef(&wasReconstructed, true)) {
        catalog = fetch(rootNum, rootGen);
    }
    return catalog;
}

Object XRef::fetch(const Ref ref, int recursion)
{
    return fetch(ref.num, ref.gen, recursion);
}

Object XRef::fetch(int num, int gen, int recursion, Goffset *endPos)
{
    XRefEntry *e;
    Object obj1, obj2, obj3;

    xrefLocker();

    const Ref ref = { .num = num, .gen = gen };

    if (!refsBeingFetched.insert(ref)) {
        return Object::null();
    }

    // Will remove ref from refsBeingFetched once it's destroyed, i.e. the function returns
    auto remover = std::make_unique<RefRecursionCheckerRemover>(refsBeingFetched, ref);

    // check for bogus ref - this can happen in corrupted PDF files
    if (num < 0 || num >= size) {
        goto err;
    }

    e = getEntry(num);
    if (!e->obj.isNull()) { // check for updated object
        return e->obj.copy();
    }

    switch (e->type) {

    case xrefEntryUncompressed: {
        if (e->gen != gen || e->offset < 0) {
            goto err;
        }
        Goffset subStreamOffset;
        if (checkedAdd(start, e->offset, &subStreamOffset)) {
            goto err;
        }
        Parser parser { this, str->makeSubStream(subStreamOffset, false, 0, Object::null()), true };
        obj1 = parser.getObj(recursion);
        obj2 = parser.getObj(recursion);
        obj3 = parser.getObj(recursion);
        if (!obj1.isInt() || obj1.getInt() != num || !obj2.isInt() || obj2.getInt() != gen || !obj3.isCmd("obj")) {
            // some buggy pdf have obj1234 for ints that represent 1234
            // try to recover here
            if (obj1.isInt() && obj1.getInt() == num && obj2.isInt() && obj2.getInt() == gen && obj3.isCmd()) {
                const char *cmd = obj3.getCmd();
                if (strlen(cmd) > 3 && cmd[0] == 'o' && cmd[1] == 'b' && cmd[2] == 'j') {
                    char *end_ptr;
                    long longNumber = strtol(cmd + 3, &end_ptr, 0);
                    if (longNumber <= INT_MAX && longNumber >= INT_MIN && *end_ptr == '\0') {
                        int number = longNumber;
                        error(errSyntaxWarning, -1, "Cmd was not obj but {0:s}, assuming the creator meant obj {1:d}", cmd, number);
                        if (endPos) {
                            *endPos = parser.getPos();
                        }
                        return Object(number);
                    }
                }
            }
            goto err;
        }
        Object obj = parser.getObj(false, (encrypted && !e->getFlag(XRefEntry::Unencrypted)) ? fileKey : nullptr, encAlgorithm, keyLength, num, gen, recursion);
        if (endPos) {
            *endPos = parser.getPos();
        }
        return obj;
    }

    case xrefEntryCompressed: {
#if 0 // Adobe apparently ignores the generation number on compressed objects
    if (gen != 0) {
      goto err;
    }
#endif
        if (e->offset >= (unsigned int)size || (entries[e->offset].type != xrefEntryUncompressed && entries[e->offset].type != xrefEntryNone)) {
            error(errSyntaxError, -1, "Invalid object stream");
            goto err;
        }