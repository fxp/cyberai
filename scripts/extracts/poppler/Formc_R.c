// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 18/24



        if (offset < 0 || offset >= fileLength || len < 0 || len > fileLength || offset + len > fileLength) {
            error(errSyntaxError, 0, "Illegal values in ByteRange array");
            if (doneCallback) {
                doneCallback();
            }
            return signature_info;
        }

        doc->getBaseStream()->setPos(offset);
        hashSignedDataBlock(signature_handler.get(), len);
    }

    if (!signature_info->isSubfilterSupported()) {
        error(errUnimplemented, 0, "Unable to validate this type of signature");
        if (doneCallback) {
            doneCallback();
        }
        return signature_info;
    }
    const SignatureValidationStatus sig_val_state = signature_handler->validateSignature();
    signature_info->setSignatureValStatus(sig_val_state);
    signature_info->setSignerName(signature_handler->getSignerName());
    signature_info->setSubjectDN(signature_handler->getSignerSubjectDN());
    signature_info->setHashAlgorithm(signature_handler->getHashAlgorithm());

    // verify if signature contains a 'signing time' attribute
    if (signature_handler->getSigningTime() != std::chrono::system_clock::time_point {}) {
        signature_info->setSigningTime(std::chrono::system_clock::to_time_t(signature_handler->getSigningTime()));
    }

    signature_info->setCertificateInfo(signature_handler->getCertificateInfo());

    if (sig_val_state != SIGNATURE_VALID || !doVerifyCert) {
        if (doneCallback) {
            doneCallback();
        }
        return signature_info;
    }

    signature_handler->validateCertificateAsync(std::chrono::system_clock::from_time_t(validationTime), ocspRevocationCheck, enableAIA, doneCallback);

    return signature_info;
}

CertificateValidationStatus FormFieldSignature::validateSignatureResult()
{
    if (!signature_handler) {
        return CERTIFICATE_GENERIC_ERROR;
    }
    return signature_handler->validateCertificateResult();
}

std::vector<Goffset> FormFieldSignature::getSignedRangeBounds() const
{
    std::vector<Goffset> range_vec;
    if (byte_range.isArray()) {
        if (byte_range.arrayGetLength() == 4) {
            for (int i = 0; i < 2; ++i) {
                const Object offsetObj(byte_range.arrayGet(2 * i));
                const Object lenObj(byte_range.arrayGet(2 * i + 1));
                if (offsetObj.isIntOrInt64() && lenObj.isIntOrInt64()) {
                    const Goffset offset = offsetObj.getIntOrInt64();
                    const Goffset len = lenObj.getIntOrInt64();
                    range_vec.push_back(offset);
                    range_vec.push_back(offset + len);
                }
            }
        }
    }
    return range_vec;
}

std::pair<std::optional<std::vector<unsigned char>>, int64_t> FormFieldSignature::getCheckedSignature()
{
    const std::vector<Goffset> ranges = getSignedRangeBounds();
    if (ranges.size() != 4) {
        return { {}, 0 };
    }
    if (signature.size() < 150) { // This can't be a valid signature, and it simplifies later validation code.
        return { {}, 0 };
    }
    BaseStream *stream = doc->getBaseStream();
    Goffset checkedFileSize = stream->getLength();
    if (signature_type == CryptoSign::SignatureType::g10c_pgp_signature_detached) {
        // Padding here is done as pgp packets, so keep no need to try to unmangle it
        return { signature, checkedFileSize };
    }
    // we have a asn1 object
    if (signature[0] == 0x30) {
        // 0x80 is indefinite lenth.
        // defined length is anything above 0x80 with the number of bytes used for length in the next bits
        if (signature[1] == 0x80) {
            // infdefinite length, let's just do all of it if it ends with two nulls
            if (signature[signature.size() - 1] == 0 && signature[signature.size() - 2] == 0) {
                return { signature, checkedFileSize };
            }
            return { {}, 0 };
        }
        if (signature[1] > 0x80) {
            size_t lengthLength = signature[1] - 0x80;
            size_t length = 0;
            for (size_t i = 0; i < lengthLength; i++) {
                length <<= 8;
                length += signature[2 + i];
            }
            length += 2 + lengthLength; // we also need the two initial bytes 0x30 and 0x8?  and the number of length bytes
            if (length > signature.size()) {
                return { {}, 0 };
            }
            return { std::vector(signature.data(), signature.data() + length), checkedFileSize };
        }
    }
    return { {}, 0 };
}

void FormFieldSignature::print(int indent)
{
    printf("%*s- (%d %d): [signature] terminal: %s children: %zu\n", indent, "", ref.num, ref.gen, terminal ? "Yes" : "No", terminal ? widgets.size() : children.size());
}

//------------------------------------------------------------------------
// Form
//------------------------------------------------------------------------

Form::Form(PDFDoc *docA) : doc(docA)
{
    Object obj1;