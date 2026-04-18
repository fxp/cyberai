// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 12/38



private:
    JBIG2HuffmanTable *table;
};

JBIG2CodeTable::JBIG2CodeTable(unsigned int segNumA, JBIG2HuffmanTable *tableA) : JBIG2Segment(segNumA)
{
    table = tableA;
}

JBIG2CodeTable::~JBIG2CodeTable()
{
    gfree(table);
}

//------------------------------------------------------------------------
// JBIG2Stream
//------------------------------------------------------------------------

JBIG2Stream::JBIG2Stream(std::unique_ptr<Stream> strA, Object &&globalsStreamA, Object *globalsStreamRefA) : OwnedFilterStream(std::move(strA))
{
    arithDecoder = new JArithmeticDecoder();
    genericRegionStats = std::make_unique<JArithmeticDecoderStats>(1 << 1);
    refinementRegionStats = std::make_unique<JArithmeticDecoderStats>(1 << 1);
    iadhStats = new JArithmeticDecoderStats(1 << 9);
    iadwStats = new JArithmeticDecoderStats(1 << 9);
    iaexStats = new JArithmeticDecoderStats(1 << 9);
    iaaiStats = new JArithmeticDecoderStats(1 << 9);
    iadtStats = new JArithmeticDecoderStats(1 << 9);
    iaitStats = new JArithmeticDecoderStats(1 << 9);
    iafsStats = new JArithmeticDecoderStats(1 << 9);
    iadsStats = new JArithmeticDecoderStats(1 << 9);
    iardxStats = new JArithmeticDecoderStats(1 << 9);
    iardyStats = new JArithmeticDecoderStats(1 << 9);
    iardwStats = new JArithmeticDecoderStats(1 << 9);
    iardhStats = new JArithmeticDecoderStats(1 << 9);
    iariStats = new JArithmeticDecoderStats(1 << 9);
    iaidStats = new JArithmeticDecoderStats(1 << 1);
    huffDecoder = new JBIG2HuffmanDecoder();
    mmrDecoder = new JBIG2MMRDecoder();

    if (globalsStreamA.isStream()) {
        globalsStream = std::move(globalsStreamA);
        if (globalsStreamRefA->isRef()) {
            globalsStreamRef = globalsStreamRefA->getRef();
        }
    }

    curStr = nullptr;
    dataPtr = dataEnd = nullptr;
}

JBIG2Stream::~JBIG2Stream()
{
    close();
    delete arithDecoder;
    delete iadhStats;
    delete iadwStats;
    delete iaexStats;
    delete iaaiStats;
    delete iadtStats;
    delete iaitStats;
    delete iafsStats;
    delete iadsStats;
    delete iardxStats;
    delete iardyStats;
    delete iardwStats;
    delete iardhStats;
    delete iariStats;
    delete iaidStats;
    delete huffDecoder;
    delete mmrDecoder;
}

bool JBIG2Stream::rewind()
{
    segments.resize(0);
    globalSegments.resize(0);
    bool rewindSuccess = true;

    // read the globals stream
    if (globalsStream.isStream()) {
        curStr = globalsStream.getStream();
        rewindSuccess = curStr->rewind();
        arithDecoder->setStream(curStr);
        huffDecoder->setStream(curStr);
        mmrDecoder->setStream(curStr);
        rewindSuccess = rewindSuccess && readSegments();
        curStr->close();
        // swap the newly read segments list into globalSegments
        std::swap(segments, globalSegments);
    }

    // read the main stream
    curStr = str;
    rewindSuccess = rewindSuccess && curStr->rewind();
    arithDecoder->setStream(curStr);
    huffDecoder->setStream(curStr);
    mmrDecoder->setStream(curStr);
    rewindSuccess = rewindSuccess && readSegments();

    if (pageBitmap) {
        dataPtr = pageBitmap->getDataPtr();
        dataEnd = dataPtr + pageBitmap->getDataSize();
    } else {
        dataPtr = dataEnd = nullptr;
    }

    return rewindSuccess;
}

void JBIG2Stream::close()
{
    pageBitmap.reset();
    segments.resize(0);
    globalSegments.resize(0);
    dataPtr = dataEnd = nullptr;
    FilterStream::close();
}

int JBIG2Stream::getChar()
{
    if (dataPtr && dataPtr < dataEnd) {
        return (*dataPtr++ ^ 0xff) & 0xff;
    }
    return EOF;
}

int JBIG2Stream::lookChar()
{
    if (dataPtr && dataPtr < dataEnd) {
        return (*dataPtr ^ 0xff) & 0xff;
    }
    return EOF;
}

Goffset JBIG2Stream::getPos()
{
    if (pageBitmap == nullptr) {
        return 0;
    }
    return dataPtr - pageBitmap->getDataPtr();
}

int JBIG2Stream::getChars(int nChars, unsigned char *buffer)
{
    int n, i;

    if (nChars <= 0 || !dataPtr) {
        return 0;
    }
    if (dataEnd - dataPtr < nChars) {
        n = (int)(dataEnd - dataPtr);
    } else {
        n = nChars;
    }
    for (i = 0; i < n; ++i) {
        buffer[i] = *dataPtr++ ^ 0xff;
    }
    return n;
}

std::optional<std::string> JBIG2Stream::getPSFilter(int /*psLevel*/, const char * /*indent*/)
{
    return {};
}

bool JBIG2Stream::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}

bool JBIG2Stream::readSegments()
{
    unsigned int segNum, segFlags, segType, page, segLength;
    unsigned int refFlags, nRefSegs;
    std::vector<unsigned int> refSegs;
    int c1, c2, c3;

    bool done = false;
    while (!done && readULong(&segNum)) {

        // segment header flags
        if (!readUByte(&segFlags)) {
            error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
            return false;
        }
        segType = segFlags & 0x3f;