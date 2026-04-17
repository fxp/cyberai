xmlParseAttValueInternal(xmlParserCtxtPtr ctxt, int *attlen, int *alloc,
                         int normalize, int isNamespace) {
    unsigned maxLength = (ctxt->options & XML_PARSE_HUGE) ?
                         XML_MAX_HUGE_LENGTH :
                         XML_MAX_TEXT_LENGTH;
    xmlSBuf buf;
    xmlChar *ret;
    int c, l, quote, flags, chunkSize;
    int inSpace = 1;
    int replaceEntities;

    /* Always expand namespace URIs */
    replaceEntities = (ctxt->replaceEntities) || (isNamespace);

    xmlSBufInit(&buf, maxLength);

    GROW;

    quote = CUR;
    if ((quote != '"') && (quote != '\'')) {
	xmlFatalErr(ctxt, XML_ERR_ATTRIBUTE_NOT_STARTED, NULL);
	return(NULL);
    }
    NEXTL(1);

    if (ctxt->inSubset == 0)
        flags = XML_ENT_CHECKED | XML_ENT_VALIDATED;
    else
        flags = XML_ENT_VALIDATED;

    inSpace = 1;
    chunkSize = 0;

    while (1) {
        if (PARSER_STOPPED(ctxt))
            goto error;

        if (CUR_PTR >= ctxt->input->end) {
            xmlFatalErrMsg(ctxt, XML_ERR_ATTRIBUTE_NOT_FINISHED,
                           "AttValue: ' expected\n");
            goto error;
        }

        /*
         * TODO: Check growth threshold
         */
        if (ctxt->input->end - CUR_PTR < 10)
            GROW;

        c = CUR;

        if (c >= 0x80) {
            l = xmlUTF8MultibyteLen(ctxt, CUR_PTR,
                    "invalid character in attribute value\n");
            if (l == 0) {
                if (chunkSize > 0) {
                    xmlSBufAddString(&buf, CUR_PTR - chunkSize, chunkSize);
                    chunkSize = 0;
                }
                xmlSBufAddReplChar(&buf);
                NEXTL(1);
            } else {
                chunkSize += l;
                NEXTL(l);
            }

            inSpace = 0;
        } else if (c != '&') {
            if (c > 0x20) {
                if (c == quote)
                    break;

                if (c == '<')
                    xmlFatalErr(ctxt, XML_ERR_LT_IN_ATTRIBUTE, NULL);

                chunkSize += 1;
                inSpace = 0;
            } else if (!IS_BYTE_CHAR(c)) {
                xmlFatalErrMsg(ctxt, XML_ERR_INVALID_CHAR,
                        "invalid character in attribute value\n");
                if (chunkSize > 0) {
                    xmlSBufAddString(&buf, CUR_PTR - chunkSize, chunkSize);
                    chunkSize = 0;
                }
                xmlSBufAddReplChar(&buf);
                inSpace = 0;
            } else {
                /* Whitespace */
                if ((normalize) && (inSpace)) {
                    /* Skip char */
                    if (chunkSize > 0) {
                        xmlSBufAddString(&buf, CUR_PTR - chunkSize, chunkSize);
                        chunkSize = 0;
                    }
                } else if (c < 0x20) {
                    /* Convert to space */
                    if (chunkSize > 0) {
                        xmlSBufAddString(&buf, CUR_PTR - chunkSize, chunkSize);
                        chunkSize = 0;
                    }

                    xmlSBufAddCString(&buf, " ", 1);
                } else {
                    chunkSize += 1;
                }

                inSpace = 1;

                if ((c == 0xD) && (NXT(1) == 0xA))
                    CUR_PTR++;
            }

            NEXTL(1);
        } else if (NXT(1) == '#') {
            int val;

            if (chunkSize > 0) {
