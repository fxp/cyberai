// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 5/24



    // Incremental save to avoid breaking any existing signatures
    if (doc->saveAs(saveFilename, writeForceIncremental) != errNone) {
        error(errIO, -1, "signDocument: error saving to file \"{0:s}\"", saveFilename.c_str());
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::WriteFailed, .message = ERROR_IN_CODE_LOCATION };
    }

    // Get start/end offset of signature object in the saved PDF
    Goffset objStart, objEnd;
    const GooString fname(saveFilename);
    if (!getObjectStartEnd(fname, vref.num, &objStart, &objEnd, ownerPassword, userPassword)) {
        error(errIO, -1, "signDocument: unable to get signature object offsets");
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::InternalError, .message = ERROR_IN_CODE_LOCATION };
    }

    // Update byte range of signature in the saved PDF
    Goffset sigStart, sigEnd, fileSize;
    FILE *file = openFile(saveFilename.c_str(), "r+b");
    if (!updateOffsets(file, objStart, objEnd, &sigStart, &sigEnd, &fileSize)) {
        error(errIO, -1, "signDocument: unable update byte range");
        fclose(file);
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::WriteFailed, .message = ERROR_IN_CODE_LOCATION };
    }

    // compute hash of byte ranges
    if (!hashFileRange(file, sigHandler.get(), 0LL, sigStart)) {
        fclose(file);
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::InternalError, .message = ERROR_IN_CODE_LOCATION };
    }
    if (!hashFileRange(file, sigHandler.get(), sigEnd, fileSize)) {
        fclose(file);
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::InternalError, .message = ERROR_IN_CODE_LOCATION };
    }

    // and sign it
    auto signature = sigHandler->signDetached(password);
    if (std::holds_alternative<CryptoSign::SigningErrorMessage>(signature)) {
        fclose(file);
        return std::get<CryptoSign::SigningErrorMessage>(signature);
    }

    if (std::get<std::vector<unsigned char>>(signature).size() > maxExpectedSignatureSize) {
        error(errInternal, -1, "signature too large");
        fclose(file);
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::InternalError, .message = ERROR_IN_CODE_LOCATION };
    }

    // pad with zeroes to placeholder length
    std::get<std::vector<unsigned char>>(signature).resize(maxExpectedSignatureSize, '\0');

    // write signature to saved file
    if (!updateSignature(file, sigStart, sigEnd, std::get<std::vector<unsigned char>>(signature))) {
        error(errIO, -1, "signDocument: unable update signature");
        fclose(file);
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::WriteFailed, .message = ERROR_IN_CODE_LOCATION };
    }
    signatureField->setSignature(std::get<std::vector<unsigned char>>(std::move(signature)));

    fclose(file);

    return {};
}

static std::tuple<double, double> calculateDxDy(int rot, const PDFRectangle *rect)
{
    switch (rot) {
    case 90:
        return { rect->y2 - rect->y1, rect->x2 - rect->x1 };

    case 180:
        return { rect->x2 - rect->y2, rect->y2 - rect->y1 };

    case 270:
        return { rect->y2 - rect->y1, rect->x2 - rect->x1 };

    default: // assume rot == 0
        return { rect->x2 - rect->x1, rect->y2 - rect->y1 };
    }
}

std::optional<CryptoSign::SigningErrorMessage> FormWidgetSignature::signDocumentWithAppearance(const std::string &saveFilename, const std::string &certNickname, const std::string &password, const GooString *reason,
                                                                                               const GooString *location, const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword,
                                                                                               const GooString &signatureText, const GooString &signatureTextLeft, double fontSize, double leftFontSize,
                                                                                               std::unique_ptr<AnnotColor> &&fontColor, double borderWidth, std::unique_ptr<AnnotColor> &&borderColor,
                                                                                               std::unique_ptr<AnnotColor> &&backgroundColor)
{
    // Set the appearance
    GooString *aux = getField()->getDefaultAppearance();
    std::string originalDefaultAppearance = aux ? aux->toStr() : std::string();

    Form *form = doc->getCatalog()->getCreateForm();