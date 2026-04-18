// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 2/19



constexpr int headerSearchSize = 1024; // read this many bytes at beginning of
                                       //   file to look for '%PDF'
constexpr int pdfIdLength = 32; // PDF Document IDs (PermanentId, UpdateId) length

constexpr int linearizationSearchSize = 1024; // read this many bytes at beginning of
                                              // file to look for linearization
                                              // dictionary

constexpr int xrefSearchSize = 1024; // read this many bytes at end of file
                                     //   to look for 'startxref'

//------------------------------------------------------------------------
// PDFDoc
//------------------------------------------------------------------------

#define pdfdocLocker() const std::scoped_lock locker(mutex)

PDFDoc::PDFDoc() = default;

PDFDoc::PDFDoc(std::unique_ptr<GooString> &&fileNameA, const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword, const std::function<void()> &xrefReconstructedCallback) : fileName(std::move(fileNameA))
{
#ifdef _WIN32
    const size_t n = fileName->size();
    for (size_t i = 0; i < n; ++i) {
        fileNameU.push_back((wchar_t)(fileName->getChar(i) & 0xff));
    }

    std::u16string u16fileName = utf8ToUtf16(fileName->toStr());
    wchar_t *wFileName = (wchar_t *)u16fileName.data();
    file = GooFile::open(wFileName);
#else
    file = GooFile::open(fileName->toStr());
#endif

    if (!file) {
        // fopen() has failed.
        // Keep a copy of the errno returned by fopen so that it can be
        // referred to later.
        fopenErrno = errno;
        error(errIO, -1, "Couldn't open file '{0:t}': {1:s}.", fileName.get(), strerror(errno));
        errCode = errOpenFile;
        return;
    }

    // create stream
    str = std::make_unique<FileStream>(file.get(), 0, false, file->size(), Object::null());

    ok = setup(ownerPassword, userPassword, xrefReconstructedCallback);
}

#ifdef _WIN32
PDFDoc::PDFDoc(wchar_t *fileNameA, int fileNameLen, const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword, const std::function<void()> &xrefReconstructedCallback)
{
    OSVERSIONINFO version;

    // save both Unicode and 8-bit copies of the file name
    std::unique_ptr<GooString> fileNameG = std::make_unique<GooString>();
    for (int i = 0; i < fileNameLen; ++i) {
        fileNameG->push_back((char)fileNameA[i]);
        fileNameU.push_back(fileNameA[i]);
    }
    fileName = std::move(fileNameG);

    // try to open file
    // NB: _wfopen is only available in NT
    version.dwOSVersionInfoSize = sizeof(version);
    GetVersionEx(&version);
    if (version.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        file = GooFile::open(fileNameU.c_str());
    } else {
        file = GooFile::open(fileName->toStr());
    }
    if (!file) {
        error(errIO, -1, "Couldn't open file '{0:t}'", fileName.get());
        errCode = errOpenFile;
        return;
    }

    // create stream
    str = std::make_unique<FileStream>(file.get(), 0, false, file->size(), Object::null());

    ok = setup(ownerPassword, userPassword, xrefReconstructedCallback);
}
#endif

PDFDoc::PDFDoc(std::unique_ptr<BaseStream> strA, const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword, const std::function<void()> &xrefReconstructedCallback)
{
    if (strA->getFileName()) {
        fileName = strA->getFileName()->copy();
#ifdef _WIN32
        const size_t n = fileName->size();
        for (size_t i = 0; i < n; ++i) {
            fileNameU.push_back((wchar_t)(fileName->getChar(i) & 0xff));
        }
#endif
    }
    str = std::move(strA);
    ok = setup(ownerPassword, userPassword, xrefReconstructedCallback);
}

bool PDFDoc::setup(const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword, const std::function<void()> &xrefReconstructedCallback)
{
    pdfdocLocker();

    if (str->getLength() <= 0) {
        error(errSyntaxError, -1, "Document stream is empty");
        errCode = errDamaged;
        return false;
    }

    str->setPos(0, -1);
    if (str->getPos() < 0) {
        error(errSyntaxError, -1, "Document base stream is not seekable");
        errCode = errFileIO;
        return false;
    }

    if (!str->rewind()) {
        error(errSyntaxError, -1, "Document base stream rewind failure");
        errCode = errFileIO;
        return false;
    }

    // check footer
    // Adobe does not seem to enforce %%EOF, so we do the same
    //  if (!checkFooter()) return false;

    // check header
    checkHeader();

    bool wasReconstructed = false;