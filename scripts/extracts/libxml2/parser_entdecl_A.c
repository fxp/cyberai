xmlParseEntityDecl(xmlParserCtxtPtr ctxt) {
    const xmlChar *name = NULL;
    xmlChar *value = NULL;
    xmlChar *URI = NULL, *literal = NULL;
    const xmlChar *ndata = NULL;
    int isParameter = 0;
    xmlChar *orig = NULL;

    if ((CUR != '<') || (NXT(1) != '!'))
        return;
    SKIP(2);

    /* GROW; done in the caller */
    if (CMP6(CUR_PTR, 'E', 'N', 'T', 'I', 'T', 'Y')) {
	int inputid = ctxt->input->id;
	SKIP(6);
	if (SKIP_BLANKS_PE == 0) {
	    xmlFatalErrMsg(ctxt, XML_ERR_SPACE_REQUIRED,
			   "Space required after '<!ENTITY'\n");
	}

	if (RAW == '%') {
	    NEXT;
	    if (SKIP_BLANKS_PE == 0) {
		xmlFatalErrMsg(ctxt, XML_ERR_SPACE_REQUIRED,
			       "Space required after '%%'\n");
	    }
	    isParameter = 1;
	}

        name = xmlParseName(ctxt);
	if (name == NULL) {
	    xmlFatalErrMsg(ctxt, XML_ERR_NAME_REQUIRED,
	                   "xmlParseEntityDecl: no name\n");
            return;
	}
	if (xmlStrchr(name, ':') != NULL) {
	    xmlNsErr(ctxt, XML_NS_ERR_COLON,
		     "colons are forbidden from entities names '%s'\n",
		     name, NULL, NULL);
	}
	if (SKIP_BLANKS_PE == 0) {
	    xmlFatalErrMsg(ctxt, XML_ERR_SPACE_REQUIRED,
			   "Space required after the entity name\n");
	}

	/*
	 * handle the various case of definitions...
	 */
	if (isParameter) {
	    if ((RAW == '"') || (RAW == '\'')) {
	        value = xmlParseEntityValue(ctxt, &orig);
		if (value) {
		    if ((ctxt->sax != NULL) &&
			(!ctxt->disableSAX) && (ctxt->sax->entityDecl != NULL))
			ctxt->sax->entityDecl(ctxt->userData, name,
		                    XML_INTERNAL_PARAMETER_ENTITY,
				    NULL, NULL, value);
		}
	    } else {
	        URI = xmlParseExternalID(ctxt, &literal, 1);
		if ((URI == NULL) && (literal == NULL)) {
		    xmlFatalErr(ctxt, XML_ERR_VALUE_REQUIRED, NULL);
		}
		if (URI) {
                    if (xmlStrchr(URI, '#')) {
                        xmlFatalErr(ctxt, XML_ERR_URI_FRAGMENT, NULL);
                    } else {
                        if ((ctxt->sax != NULL) &&
                            (!ctxt->disableSAX) &&
                            (ctxt->sax->entityDecl != NULL))
                            ctxt->sax->entityDecl(ctxt->userData, name,
                                        XML_EXTERNAL_PARAMETER_ENTITY,
                                        literal, URI, NULL);
                    }
		}
	    }
	} else {
	    if ((RAW == '"') || (RAW == '\'')) {
	        value = xmlParseEntityValue(ctxt, &orig);
		if ((ctxt->sax != NULL) &&
		    (!ctxt->disableSAX) && (ctxt->sax->entityDecl != NULL))
		    ctxt->sax->entityDecl(ctxt->userData, name,
				XML_INTERNAL_GENERAL_ENTITY,
				NULL, NULL, value);
		/*
		 * For expat compatibility in SAX mode.
		 */
		if ((ctxt->myDoc == NULL) ||
		    (xmlStrEqual(ctxt->myDoc->version, SAX_COMPAT_MODE))) {
		    if (ctxt->myDoc == NULL) {
			ctxt->myDoc = xmlNewDoc(SAX_COMPAT_MODE);
			if (ctxt->myDoc == NULL) {
			    xmlErrMemory(ctxt);
			    goto done;
			}
			ctxt->myDoc->properties = XML_DOC_INTERNAL;
		    }
		    if (ctxt->myDoc->intSubset == NULL) {
			ctxt->myDoc->intSubset = xmlNewDtd(ctxt->myDoc,
					    BAD_CAST "fake", NULL, NULL);
                        if (ctxt->myDoc->intSubset == NULL) {
