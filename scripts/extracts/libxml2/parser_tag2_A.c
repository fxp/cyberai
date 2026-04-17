xmlParseStartTag2(xmlParserCtxtPtr ctxt, const xmlChar **pref,
                  const xmlChar **URI, int *nbNsPtr) {
    xmlHashedString hlocalname;
    xmlHashedString hprefix;
    xmlHashedString hattname;
    xmlHashedString haprefix;
    const xmlChar *localname;
    const xmlChar *prefix;
    const xmlChar *attname;
    const xmlChar *aprefix;
    const xmlChar *uri;
    xmlChar *attvalue = NULL;
    const xmlChar **atts = ctxt->atts;
    unsigned attrHashSize = 0;
    int maxatts = ctxt->maxatts;
    int nratts, nbatts, nbdef;
    int i, j, nbNs, nbTotalDef, attval, nsIndex, maxAtts;
    int alloc = 0;
    int numNsErr = 0;
    int numDupErr = 0;

    if (RAW != '<') return(NULL);
    NEXT1;

    nbatts = 0;
    nratts = 0;
    nbdef = 0;
    nbNs = 0;
    nbTotalDef = 0;
    attval = 0;

    if (xmlParserNsStartElement(ctxt->nsdb) < 0) {
        xmlErrMemory(ctxt);
        return(NULL);
    }

    hlocalname = xmlParseQNameHashed(ctxt, &hprefix);
    if (hlocalname.name == NULL) {
	xmlFatalErrMsg(ctxt, XML_ERR_NAME_REQUIRED,
		       "StartTag: invalid element name\n");
        return(NULL);
    }
    localname = hlocalname.name;
    prefix = hprefix.name;

    /*
     * Now parse the attributes, it ends up with the ending
     *
     * (S Attribute)* S?
     */
    SKIP_BLANKS;
    GROW;

    /*
     * The ctxt->atts array will be ultimately passed to the SAX callback
     * containing five xmlChar pointers for each attribute:
     *
     * [0] attribute name
     * [1] attribute prefix
     * [2] namespace URI
     * [3] attribute value
     * [4] end of attribute value
     *
     * To save memory, we reuse this array temporarily and store integers
     * in these pointer variables.
     *
     * [0] attribute name
     * [1] attribute prefix
     * [2] hash value of attribute prefix, and later namespace index
     * [3] for non-allocated values: ptrdiff_t offset into input buffer
     * [4] for non-allocated values: ptrdiff_t offset into input buffer
     *
     * The ctxt->attallocs array contains an additional unsigned int for
     * each attribute, containing the hash value of the attribute name
     * and the alloc flag in bit 31.
     */

    while (((RAW != '>') &&
	   ((RAW != '/') || (NXT(1) != '>')) &&
	   (IS_BYTE_CHAR(RAW))) && (PARSER_STOPPED(ctxt) == 0)) {
	int len = -1;

	hattname = xmlParseAttribute2(ctxt, prefix, localname,
                                          &haprefix, &attvalue, &len,
                                          &alloc);
        if (hattname.name == NULL)
	    break;
        if (attvalue == NULL)
            goto next_attr;
        attname = hattname.name;
        aprefix = haprefix.name;
	if (len < 0) len = xmlStrlen(attvalue);

        if ((attname == ctxt->str_xmlns) && (aprefix == NULL)) {
            xmlHashedString huri;
            xmlURIPtr parsedUri;

            huri = xmlDictLookupHashed(ctxt->dict, attvalue, len);
            uri = huri.name;
            if (uri == NULL) {
                xmlErrMemory(ctxt);
                goto next_attr;
            }
            if (*uri != 0) {
                if (xmlParseURISafe((const char *) uri, &parsedUri) < 0) {
                    xmlErrMemory(ctxt);
                    goto next_attr;
                }
                if (parsedUri == NULL) {
                    xmlNsErr(ctxt, XML_WAR_NS_URI,
                             "xmlns: '%s' is not a valid URI\n",
                                       uri, NULL, NULL);
                } else {
                    if (parsedUri->scheme == NULL) {
                        xmlNsWarn(ctxt, XML_WAR_NS_URI_RELATIVE,
                                  "xmlns: URI %s is not absolute\n",
                                  uri, NULL, NULL);
                    }
                    xmlFreeURI(parsedUri);
                }
                if (uri == ctxt->str_xml_ns) {
