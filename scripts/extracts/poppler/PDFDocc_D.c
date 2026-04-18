// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 4/19



static PDFSubtypePart pdfPartFromString(PDFSubtype subtype, const std::string &pdfsubver)
{
    const std::regex regex("PDF/(?:A|X|VT|E|UA)-([[:digit:]])(?:[[:alpha:]]{1,2})?:?([[:digit:]]{4})?");
    std::smatch match;
    PDFSubtypePart subtypePart = subtypePartNone;

    if (std::regex_search(pdfsubver, match, regex)) {
        int date = 0;
        const int part = std::stoi(match.str(1));

        if (match[2].matched) {
            date = std::stoi(match.str(2));
        }

        switch (subtype) {
        case subtypePDFX:
            switch (part) {
            case 1:
                switch (date) {
                case 2001:
                default:
                    subtypePart = subtypePart1;
                    break;
                case 2003:
                    subtypePart = subtypePart4;
                    break;
                }
                break;
            case 2:
                subtypePart = subtypePart5;
                break;
            case 3:
                switch (date) {
                case 2002:
                default:
                    subtypePart = subtypePart3;
                    break;
                case 2003:
                    subtypePart = subtypePart6;
                    break;
                }
                break;
            case 4:
                subtypePart = subtypePart7;
                break;
            case 5:
                subtypePart = subtypePart8;
                break;
            }
            break;
        default:
            subtypePart = (PDFSubtypePart)part;
            break;
        }
    }

    return subtypePart;
}

static PDFSubtypeConformance pdfConformanceFromString(const std::string &pdfsubver)
{
    const std::regex regex("PDF/(?:A|X|VT|E|UA)-[[:digit:]]([[:alpha:]]{1,3})");
    std::smatch match;

    // match contains the PDF conformance (A, B, G, N, P, PG or U)
    if (std::regex_search(pdfsubver, match, regex)) {
        // Convert to lowercase as the conformance may appear in both cases
        std::string conf = GooString::toLowerCase(match.str(1));
        switch (conf.size()) {
        case 1: {
            switch (conf[0]) {
            case 'a':
                return subtypeConfA;
            case 'b':
                return subtypeConfB;
            case 'g':
                return subtypeConfG;
            case 'n':
                return subtypeConfN;
            case 'p':
                return subtypeConfP;
            case 'u':
                return subtypeConfU;
            default:
                /**/
                break;
            }
            break;
        }
        case 2: {
            if (conf == std::string_view("pg")) {
                return subtypeConfPG;
                break;
            }
        }
        default:
            /**/
            break;
        }
        error(errSyntaxWarning, -1, "Unexpected pdf subtype {0:s}", conf.c_str());
    }

    return subtypeConfNone;
}

void PDFDoc::extractPDFSubtype()
{
    pdfSubtype = subtypeNull;
    pdfPart = subtypePartNull;
    pdfConformance = subtypeConfNull;

    std::unique_ptr<GooString> pdfSubtypeVersion;
    // Find PDF InfoDict subtype key if any
    if ((pdfSubtypeVersion = getDocInfoStringEntry("GTS_PDFA1Version"))) {
        pdfSubtype = subtypePDFA;
    } else if ((pdfSubtypeVersion = getDocInfoStringEntry("GTS_PDFEVersion"))) {
        pdfSubtype = subtypePDFE;
    } else if ((pdfSubtypeVersion = getDocInfoStringEntry("GTS_PDFUAVersion"))) {
        pdfSubtype = subtypePDFUA;
    } else if ((pdfSubtypeVersion = getDocInfoStringEntry("GTS_PDFVTVersion"))) {
        pdfSubtype = subtypePDFVT;
    } else if ((pdfSubtypeVersion = getDocInfoStringEntry("GTS_PDFXVersion"))) {
        pdfSubtype = subtypePDFX;
    } else {
        pdfSubtype = subtypeNone;
        pdfPart = subtypePartNone;
        pdfConformance = subtypeConfNone;
        return;
    }

    // Extract part from version string
    pdfPart = pdfPartFromString(pdfSubtype, pdfSubtypeVersion->toStr());

    // Extract conformance from version string
    pdfConformance = pdfConformanceFromString(pdfSubtypeVersion->toStr());
}

static void addSignatureFieldsToVector(FormField *ff, std::vector<FormFieldSignature *> &res)
{
    if (ff->getNumChildren() == 0) {
        if (ff->getType() == formSignature) {
            res.push_back(static_cast<FormFieldSignature *>(ff));
        }
    } else {
        for (int i = 0; i < ff->getNumChildren(); ++i) {
            FormField *children = ff->getChildren(i);
            addSignatureFieldsToVector(children, res);
        }
    }
}

std::vector<FormFieldSignature *> PDFDoc::getSignatureFields()
{
    // Unfortunately there's files with signatures in Forms but not in Annots
    // and files with signatures in Annots but no in forms so we need to search both
    std::vector<FormFieldSignature *> res;