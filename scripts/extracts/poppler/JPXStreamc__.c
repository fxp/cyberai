// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 31/31



unsigned int JPXStream::finishBitBuf()
{
    if (bitBufSkip) {
        bufStr->getChar();
        --byteCount;
    }
    return byteCount;
}
