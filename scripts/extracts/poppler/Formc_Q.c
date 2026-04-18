// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 17/24



    Object location_obj = sig_dict.dictLookup("Location");
    if (location_obj.isString()) {
        signature_info->setLocation(location_obj.takeString());
    }

    Object reason_obj = sig_dict.dictLookup("Reason");
    if (reason_obj.isString()) {
        signature_info->setReason(reason_obj.takeString());
    }

    // retrieve SigningTime
    Object time_of_signing = sig_dict.dictLookup("M");
    if (time_of_signing.isString()) {
        const std::string &time_str = time_of_signing.getString();
        signature_info->setSigningTime(dateStringToTime(time_str)); // Put this information directly in SignatureInfo object
    }

    // check if subfilter is supported for signature validation, only detached signatures work for now
    Object subfilterName = sig_dict.dictLookup("SubFilter");
    if (subfilterName.getType() == objName && subfilterName.getName()) {
        signature_type = CryptoSign::signatureTypeFromString(subfilterName.getName());
        switch (signature_type) {
        case CryptoSign::SignatureType::adbe_pkcs7_sha1:
        case CryptoSign::SignatureType::adbe_pkcs7_detached:
        case CryptoSign::SignatureType::ETSI_CAdES_detached:
        case CryptoSign::SignatureType::g10c_pgp_signature_detached:
            signature_info->setSubFilterSupport(true);
            break;
        case CryptoSign::SignatureType::unknown_signature_type:
        case CryptoSign::SignatureType::unsigned_signature_field:
            break;
        }
    }
}

void FormFieldSignature::hashSignedDataBlock(CryptoSign::VerificationInterface *handler, Goffset block_len)
{
    if (!handler) {
        return;
    }
    const int BLOCK_SIZE = 4096;
    unsigned char signed_data_buffer[BLOCK_SIZE];

    Goffset i = 0;
    while (i < block_len) {
        Goffset bytes_left = block_len - i;
        if (bytes_left < BLOCK_SIZE) {
            doc->getBaseStream()->doGetChars(static_cast<int>(bytes_left), signed_data_buffer);
            handler->addData(signed_data_buffer, static_cast<int>(bytes_left));
            i = block_len;
        } else {
            doc->getBaseStream()->doGetChars(BLOCK_SIZE, signed_data_buffer);
            handler->addData(signed_data_buffer, BLOCK_SIZE);
            i += BLOCK_SIZE;
        }
    }
}

CryptoSign::SignatureType FormWidgetSignature::signatureType() const
{
    return static_cast<FormFieldSignature *>(field)->getSignatureType();
}

void FormWidgetSignature::setSignatureType(CryptoSign::SignatureType fst)
{
    static_cast<FormFieldSignature *>(field)->setSignatureType(fst);
}

SignatureInfo *FormFieldSignature::validateSignatureAsync(bool doVerifyCert, bool forceRevalidation, time_t validationTime, bool ocspRevocationCheck, bool enableAIA, const std::function<void()> &doneCallback)
{
    auto backend = CryptoSign::Factory::createActive();
    if (!backend) {
        if (doneCallback) {
            doneCallback();
        }
        return signature_info;
    }

    if (signature_info->getSignatureValStatus() != SIGNATURE_NOT_VERIFIED && !forceRevalidation) {
        if (doneCallback) {
            doneCallback();
        }
        return signature_info;
    }

    if (signature.empty()) {
        error(errSyntaxError, 0, "Invalid or missing Signature string");
        if (doneCallback) {
            doneCallback();
        }
        return signature_info;
    }

    if (!byte_range.isArray()) {
        error(errSyntaxError, 0, "Invalid or missing ByteRange array");
        if (doneCallback) {
            doneCallback();
        }
        return signature_info;
    }

    int arrayLen = byte_range.arrayGetLength();
    if (arrayLen < 2) {
        error(errSyntaxError, 0, "Too few elements in ByteRange array");
        if (doneCallback) {
            doneCallback();
        }
        return signature_info;
    }

    // Some signatures are supposed to be padded with zeroes, but are instead padded with crap
    // so preprocess them before sending them into the slightly stricter cryptobackend
    auto [unpadded, _] = getCheckedSignature();
    if (!unpadded) {
        error(errSyntaxError, 0, "Invalid or missing Signature string");
        return signature_info;
    }

    signature_handler = backend->createVerificationHandler(std::move(*unpadded), signature_type);

    if (!signature_handler) {
        if (doneCallback) {
            doneCallback();
        }
        return signature_info;
    }

    Goffset fileLength = doc->getBaseStream()->getLength();
    for (int i = 0; i < arrayLen / 2; i++) {
        Object offsetObj = byte_range.arrayGet(i * 2);
        Object lenObj = byte_range.arrayGet(i * 2 + 1);

        if (!offsetObj.isIntOrInt64() || !lenObj.isIntOrInt64()) {
            error(errSyntaxError, 0, "Illegal values in ByteRange array");
            if (doneCallback) {
                doneCallback();
            }
            return signature_info;
        }

        Goffset offset = offsetObj.getIntOrInt64();
        Goffset len = lenObj.getIntOrInt64();