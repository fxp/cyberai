// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 19/19



        // Now remove the signature stuff in case the user wants to continue editing stuff
        // So the document object is clean
        const Object &vRefObj = sig->field->getObj()->dictLookupNF("V");
        if (vRefObj.isRef()) {
            getXRef()->removeIndirectObject(vRefObj.getRef());
        }
        destPage->removeAnnot(sig->annotWidget);
        catalog->removeFormFromAcroForm(sig->ref);
        getXRef()->removeIndirectObject(sig->ref);

        return res;
    }

    return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::InternalError, .message = ERROR_IN_CODE_LOCATION };
}
