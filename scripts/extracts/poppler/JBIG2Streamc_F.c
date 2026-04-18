// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 38/38



bool JBIG2Stream::resetIntStats(int symCodeLen)
{
    iadhStats->resetContext();
    iadwStats->resetContext();
    iaexStats->resetContext();
    iaaiStats->resetContext();
    iadtStats->resetContext();
    iaitStats->resetContext();
    iafsStats->resetContext();
    iadsStats->resetContext();
    iardxStats->resetContext();
    iardyStats->resetContext();
    iardwStats->resetContext();
    iardhStats->resetContext();
    iariStats->resetContext();
    if (symCodeLen + 1 >= 31) {
        return false;
    }
    if (iaidStats != nullptr && iaidStats->getContextSize() == 1 << (symCodeLen + 1)) {
        iaidStats->resetContext();
    } else {
        delete iaidStats;
        iaidStats = new JArithmeticDecoderStats(1 << (symCodeLen + 1));
        if (!iaidStats->isValid()) {
            delete iaidStats;
            iaidStats = nullptr;
            return false;
        }
    }
    return true;
}

bool JBIG2Stream::readUByte(unsigned int *x)
{
    int c0;

    if ((c0 = curStr->getChar()) == EOF) {
        return false;
    }
    ++byteCounter;
    *x = (unsigned int)c0;
    return true;
}

bool JBIG2Stream::readByte(int *x)
{
    int c0;

    if ((c0 = curStr->getChar()) == EOF) {
        return false;
    }
    ++byteCounter;
    *x = c0;
    if (c0 & 0x80) {
        *x |= -1 - 0xff;
    }
    return true;
}

bool JBIG2Stream::readUWord(unsigned int *x)
{
    int c0, c1;

    if ((c0 = curStr->getChar()) == EOF || (c1 = curStr->getChar()) == EOF) {
        return false;
    }
    byteCounter += 2;
    *x = (unsigned int)((c0 << 8) | c1);
    return true;
}

bool JBIG2Stream::readULong(unsigned int *x)
{
    int c0, c1, c2, c3;

    if ((c0 = curStr->getChar()) == EOF || (c1 = curStr->getChar()) == EOF || (c2 = curStr->getChar()) == EOF || (c3 = curStr->getChar()) == EOF) {
        return false;
    }
    byteCounter += 4;
    *x = (unsigned int)((c0 << 24) | (c1 << 16) | (c2 << 8) | c3);
    return true;
}

bool JBIG2Stream::readLong(int *x)
{
    int c0, c1, c2, c3;

    if ((c0 = curStr->getChar()) == EOF || (c1 = curStr->getChar()) == EOF || (c2 = curStr->getChar()) == EOF || (c3 = curStr->getChar()) == EOF) {
        return false;
    }
    byteCounter += 4;
    *x = ((c0 << 24) | (c1 << 16) | (c2 << 8) | c3);
    if (c0 & 0x80) {
        *x |= -1 - (int)0xffffffff;
    }
    return true;
}
