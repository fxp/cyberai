// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 7/36



std::unique_ptr<Stream> FileStream::makeSubStream(Goffset startA, bool limitedA, Goffset lengthA, Object &&dictA)
{
    return std::make_unique<FileStream>(file, startA, limitedA, lengthA, std::move(dictA));
}

bool FileStream::rewind()
{
    savePos = offset;
    offset = start;
    saved = true;
    bufPtr = bufEnd = buf;
    bufPos = start;

    return true;
}

void FileStream::close()
{
    if (saved) {
        offset = savePos;
        saved = false;
    }
}

bool FileStream::fillBuf()
{
    int n;

    bufPos += bufEnd - buf;
    bufPtr = bufEnd = buf;
    if (limited && bufPos >= start + length) {
        return false;
    }
    if (limited && bufPos + fileStreamBufSize > start + length) {
        n = start + length - bufPos;
    } else {
        n = fileStreamBufSize;
    }
    n = file->read(buf, n, offset);
    if (n == -1) {
        return false;
    }
    offset += n;
    bufEnd = buf + n;
    return bufPtr < bufEnd;
}

void FileStream::setPos(Goffset pos, int dir)
{
    Goffset size;

    if (dir >= 0) {
        offset = bufPos = pos;
    } else {
        size = file->size();
        if (pos > size) {
            pos = size;
        }
        offset = size - pos;
        bufPos = offset;
    }
    bufPtr = bufEnd = buf;
}

void FileStream::moveStart(Goffset delta)
{
    start += delta;
    bufPtr = bufEnd = buf;
    bufPos = start;
}

//------------------------------------------------------------------------
// CachedFileStream
//------------------------------------------------------------------------

CachedFileStream::CachedFileStream(std::shared_ptr<CachedFile> ccA, Goffset startA, bool limitedA, Goffset lengthA, Object &&dictA) : BaseStream(std::move(dictA), lengthA)
{
    cc = std::move(ccA);
    start = startA;
    limited = limitedA;
    length = lengthA;
    bufPtr = bufEnd = buf;
    bufPos = start;
    savePos = 0;
    saved = false;
}

CachedFileStream::~CachedFileStream()
{
    close();
}

std::unique_ptr<BaseStream> CachedFileStream::copy()
{
    return std::make_unique<CachedFileStream>(cc, start, limited, length, dict.copy());
}

std::unique_ptr<Stream> CachedFileStream::makeSubStream(Goffset startA, bool limitedA, Goffset lengthA, Object &&dictA)
{
    return std::make_unique<CachedFileStream>(cc, startA, limitedA, lengthA, std::move(dictA));
}

bool CachedFileStream::rewind()
{
    savePos = (unsigned int)cc->tell();
    cc->seek(start, SEEK_SET);

    saved = true;
    bufPtr = bufEnd = buf;
    bufPos = start;

    return true;
}

void CachedFileStream::close()
{
    if (saved) {
        cc->seek(savePos, SEEK_SET);
        saved = false;
    }
}

bool CachedFileStream::fillBuf()
{
    int n;

    bufPos += bufEnd - buf;
    bufPtr = bufEnd = buf;
    if (limited && bufPos >= start + length) {
        return false;
    }
    if (limited && bufPos + cachedStreamBufSize > start + length) {
        n = start + length - bufPos;
    } else {
        n = cachedStreamBufSize - (bufPos % cachedStreamBufSize);
    }
    n = cc->read(buf, 1, n);
    bufEnd = buf + n;
    return bufPtr < bufEnd;
}

void CachedFileStream::setPos(Goffset pos, int dir)
{
    unsigned int size;

    if (dir >= 0) {
        if (cc->seek(pos, SEEK_SET) == 0) {
            bufPos = pos;
        } else {
            cc->seek(0, SEEK_END);
            bufPos = pos = (unsigned int)cc->tell();
            error(errInternal, pos, "CachedFileStream: Seek beyond end attempted, capped to file size");
        }
    } else {
        cc->seek(0, SEEK_END);
        size = (unsigned int)cc->tell();

        if (pos > size) {
            pos = size;
        }

        cc->seek(-(int)pos, SEEK_END);
        bufPos = (unsigned int)cc->tell();
    }

    bufPtr = bufEnd = buf;
}

void CachedFileStream::moveStart(Goffset delta)
{
    start += delta;
    bufPtr = bufEnd = buf;
    bufPos = start;
}

MemStream::~MemStream() = default;

AutoFreeMemStream::~AutoFreeMemStream() = default;

bool AutoFreeMemStream::isFilterRemovalForbidden() const
{
    return filterRemovalForbidden;
}

void AutoFreeMemStream::setFilterRemovalForbidden(bool forbidden)
{
    filterRemovalForbidden = forbidden;
}

//------------------------------------------------------------------------
// EmbedStream
//------------------------------------------------------------------------

EmbedStream::EmbedStream(Stream *strA, Object &&dictA, bool limitedA, Goffset lengthA, bool reusableA) : BaseStream(std::move(dictA), lengthA), limited(limitedA), reusable(reusableA), initialLength(length)
{
    str = strA;
    length = lengthA;
    replay = false;
    if (reusable) {
        bufData = (unsigned char *)gmalloc(16384);
        bufMax = 16384;
        bufLen = 0;
    }
}

EmbedStream::~EmbedStream()
{
    if (reusable) {
        gfree(bufData);
    }
}

bool EmbedStream::rewind()
{
    if (length == initialLength) {
        // In general one can't rewind EmbedStream
        // but if we have not read anything yet, that's fine
        return true;
    }