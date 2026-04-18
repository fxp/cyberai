// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 2/13



    obj1 = objStr.streamGetDict()->lookup("N", recursion);
    if (!obj1.isInt()) {
        return;
    }
    nObjects = obj1.getInt();
    if (nObjects <= 0) {
        return;
    }

    obj1 = objStr.streamGetDict()->lookup("First", recursion);
    if (!obj1.isInt() && !obj1.isInt64()) {
        return;
    }
    if (obj1.isInt()) {
        first = obj1.getInt();
    } else {
        first = obj1.getInt64();
    }
    if (first < 0) {
        return;
    }

    // this is an arbitrary limit to avoid integer overflow problems
    // in the 'new Object[nObjects]' call (Acrobat apparently limits
    // object streams to 100-200 objects)
    if (nObjects > 1000000) {
        error(errSyntaxError, -1, "Too many objects in an object stream");
        return;
    }
    if (!objStr.streamRewind()) {
        return;
    }
    objs = new Object[nObjects];
    objNums = (int *)gmallocn(nObjects, sizeof(int));
    offsets = (Goffset *)gmallocn(nObjects, sizeof(Goffset));

    // parse the header: object numbers and offsets
    auto embedStr = std::make_unique<EmbedStream>(objStr.getStream(), Object::null(), true, first);
    parser = new Parser(xref, std::move(embedStr), false);
    for (i = 0; i < nObjects; ++i) {
        obj1 = parser->getObj();
        Object obj2 = parser->getObj();
        if (!obj1.isInt() || !(obj2.isInt() || obj2.isInt64())) {
            delete parser;
            gfree(offsets);
            return;
        }
        objNums[i] = obj1.getInt();
        if (obj2.isInt()) {
            offsets[i] = obj2.getInt();
        } else {
            offsets[i] = obj2.getInt64();
        }
        if (objNums[i] < 0 || offsets[i] < 0 || (i > 0 && offsets[i] < offsets[i - 1])) {
            delete parser;
            gfree(offsets);
            return;
        }
    }
    {
        auto *str = parser->getStream();
        while (str && str->getChar() != EOF) {
            ;
        }
    }
    delete parser;

    // skip to the first object - this shouldn't be necessary because
    // the First key is supposed to be equal to offsets[0], but just in
    // case...
    for (Goffset pos = first; pos < offsets[0]; ++pos) {
        objStr.getStream()->getChar();
    }

    // parse the objects
    for (i = 0; i < nObjects; ++i) {
        std::unique_ptr<Stream> strPtr;
        if (i == nObjects - 1) {
            strPtr = std::make_unique<EmbedStream>(objStr.getStream(), Object::null(), false, 0);
        } else {
            strPtr = std::make_unique<EmbedStream>(objStr.getStream(), Object::null(), true, offsets[i + 1] - offsets[i]);
        }
        parser = new Parser(xref, std::move(strPtr), false);
        objs[i] = parser->getObj();
        auto *str = parser->getStream();
        while (str && str->getChar() != EOF) {
            ;
        }
        delete parser;
    }

    gfree(offsets);
    ok = true;
}

ObjectStream::~ObjectStream()
{
    delete[] objs;
    gfree(objNums);
}

Object ObjectStream::getObject(int objIdx, int objNum)
{
    if (objIdx < 0 || objIdx >= nObjects || objNum != objNums[objIdx]) {
        return Object::null();
    }
    return objs[objIdx].copy();
}

//------------------------------------------------------------------------
// XRef
//------------------------------------------------------------------------

#define xrefLocker() const std::scoped_lock locker(mutex)

XRef::XRef()
{
    ok = true;
    errCode = errNone;
    entries = nullptr;
    capacity = 0;
    size = 0;
    modified = false;
    streamEnds = nullptr;
    streamEndsLen = 0;
    mainXRefEntriesOffset = 0;
    xRefStream = false;
    scannedSpecialFlags = false;
    encrypted = false;
    permFlags = defPermFlags;
    ownerPasswordOk = false;
    rootNum = -1;
    rootGen = -1;
    xrefReconstructed = false;
    encAlgorithm = cryptNone;
    keyLength = 0;
}

XRef::XRef(const Object *trailerDictA) : XRef {}
{
    if (trailerDictA->isDict()) {
        trailerDict = trailerDictA->copy();
    }
}

XRef::XRef(BaseStream *strA, Goffset pos, Goffset mainXRefEntriesOffsetA, bool *wasReconstructed, bool reconstruct, const std::function<void()> &xrefReconstructedCallback) : XRef {}
{
    Object obj;

    mainXRefEntriesOffset = mainXRefEntriesOffsetA;

    xrefReconstructedCb = xrefReconstructedCallback;

    // read the trailer
    str = strA;
    start = str->getStart();
    prevXRefOffset = mainXRefOffset = pos;

    if (reconstruct && !(ok = constructXRef(wasReconstructed))) {
        errCode = errDamaged;
        return;
    }
    // if there was a problem with the 'startxref' position, try to
    // reconstruct the xref table
    if (prevXRefOffset == 0) {
        if (!(ok = constructXRef(wasReconstructed))) {
            errCode = errDamaged;
            return;
        }

        // read the xref table
    } else {
        std::vector<Goffset> followedXRefStm;
        readXRef(&prevXRefOffset, &followedXRefStm, nullptr);