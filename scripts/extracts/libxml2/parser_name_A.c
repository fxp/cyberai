xmlParseNameComplex(xmlParserCtxtPtr ctxt) {
    const xmlChar *ret;
    int len = 0, l;
    int c;
    int maxLength = (ctxt->options & XML_PARSE_HUGE) ?
                    XML_MAX_TEXT_LENGTH :
                    XML_MAX_NAME_LENGTH;

    /*
     * Handler for more complex cases
     */
    c = CUR_CHAR(l);
    if ((ctxt->options & XML_PARSE_OLD10) == 0) {
        /*
	 * Use the new checks of production [4] [4a] amd [5] of the
	 * Update 5 of XML-1.0
	 */
	if ((c == ' ') || (c == '>') || (c == '/') || /* accelerators */
	    (!(((c >= 'a') && (c <= 'z')) ||
	       ((c >= 'A') && (c <= 'Z')) ||
	       (c == '_') || (c == ':') ||
	       ((c >= 0xC0) && (c <= 0xD6)) ||
	       ((c >= 0xD8) && (c <= 0xF6)) ||
	       ((c >= 0xF8) && (c <= 0x2FF)) ||
	       ((c >= 0x370) && (c <= 0x37D)) ||
	       ((c >= 0x37F) && (c <= 0x1FFF)) ||
	       ((c >= 0x200C) && (c <= 0x200D)) ||
	       ((c >= 0x2070) && (c <= 0x218F)) ||
	       ((c >= 0x2C00) && (c <= 0x2FEF)) ||
	       ((c >= 0x3001) && (c <= 0xD7FF)) ||
	       ((c >= 0xF900) && (c <= 0xFDCF)) ||
	       ((c >= 0xFDF0) && (c <= 0xFFFD)) ||
	       ((c >= 0x10000) && (c <= 0xEFFFF))))) {
	    return(NULL);
	}
	len += l;
	NEXTL(l);
	c = CUR_CHAR(l);
	while ((c != ' ') && (c != '>') && (c != '/') && /* accelerators */
	       (((c >= 'a') && (c <= 'z')) ||
	        ((c >= 'A') && (c <= 'Z')) ||
	        ((c >= '0') && (c <= '9')) || /* !start */
	        (c == '_') || (c == ':') ||
	        (c == '-') || (c == '.') || (c == 0xB7) || /* !start */
	        ((c >= 0xC0) && (c <= 0xD6)) ||
	        ((c >= 0xD8) && (c <= 0xF6)) ||
	        ((c >= 0xF8) && (c <= 0x2FF)) ||
	        ((c >= 0x300) && (c <= 0x36F)) || /* !start */
	        ((c >= 0x370) && (c <= 0x37D)) ||
	        ((c >= 0x37F) && (c <= 0x1FFF)) ||
	        ((c >= 0x200C) && (c <= 0x200D)) ||
	        ((c >= 0x203F) && (c <= 0x2040)) || /* !start */
	        ((c >= 0x2070) && (c <= 0x218F)) ||
	        ((c >= 0x2C00) && (c <= 0x2FEF)) ||
	        ((c >= 0x3001) && (c <= 0xD7FF)) ||
	        ((c >= 0xF900) && (c <= 0xFDCF)) ||
	        ((c >= 0xFDF0) && (c <= 0xFFFD)) ||
	        ((c >= 0x10000) && (c <= 0xEFFFF))
		)) {
            if (len <= INT_MAX - l)
	        len += l;
	    NEXTL(l);
	    c = CUR_CHAR(l);
	}
    } else {
	if ((c == ' ') || (c == '>') || (c == '/') || /* accelerators */
	    (!IS_LETTER(c) && (c != '_') &&
	     (c != ':'))) {
	    return(NULL);
	}
	len += l;
	NEXTL(l);
	c = CUR_CHAR(l);

	while ((c != ' ') && (c != '>') && (c != '/') && /* test bigname.xml */
	       ((IS_LETTER(c)) || (IS_DIGIT(c)) ||
		(c == '.') || (c == '-') ||
		(c == '_') || (c == ':') ||
		(IS_COMBINING(c)) ||
		(IS_EXTENDER(c)))) {
            if (len <= INT_MAX - l)
	        len += l;
	    NEXTL(l);
	    c = CUR_CHAR(l);
	}
    }
    if (len > maxLength) {
        xmlFatalErr(ctxt, XML_ERR_NAME_TOO_LONG, "Name");
        return(NULL);
    }
    if (ctxt->input->cur - ctxt->input->base < len) {
        /*
         * There were a couple of bugs where PERefs lead to to a change
         * of the buffer. Check the buffer size to avoid passing an invalid
         * pointer to xmlDictLookup.
         */
        xmlFatalErr(ctxt, XML_ERR_INTERNAL_ERROR,
                    "unexpected change of input buffer");
        return (NULL);
    }
    if ((*ctxt->input->cur == '\n') && (ctxt->input->cur[-1] == '\r'))
        ret = xmlDictLookup(ctxt->dict, ctxt->input->cur - (len + 1), len);
    else
        ret = xmlDictLookup(ctxt->dict, ctxt->input->cur - len, len);
    if (ret == NULL)
        xmlErrMemory(ctxt);
    return(ret);
}

/**
 * xmlParseName:
 * @ctxt:  an XML parser context
 *
 * DEPRECATED: Internal function, don't use.
 *
 * parse an XML name.
 *
 * [4] NameChar ::= Letter | Digit | '.' | '-' | '_' | ':' |
 *                  CombiningChar | Extender
 *
 * [5] Name ::= (Letter | '_' | ':') (NameChar)*
 *
 * [6] Names ::= Name (#x20 Name)*
 *
 * Returns the Name parsed or NULL
 */

const xmlChar *
xmlParseName(xmlParserCtxtPtr ctxt) {
    const xmlChar *in;
    const xmlChar *ret;
    size_t count = 0;
    size_t maxLength = (ctxt->options & XML_PARSE_HUGE) ?
                       XML_MAX_TEXT_LENGTH :
                       XML_MAX_NAME_LENGTH;

    GROW;

    /*
     * Accelerator for simple ASCII names
     */
    in = ctxt->input->cur;
    if (((*in >= 0x61) && (*in <= 0x7A)) ||
	((*in >= 0x41) && (*in <= 0x5A)) ||
	(*in == '_') || (*in == ':')) {
	in++;
	while (((*in >= 0x61) && (*in <= 0x7A)) ||
	       ((*in >= 0x41) && (*in <= 0x5A)) ||
	       ((*in >= 0x30) && (*in <= 0x39)) ||
	       (*in == '_') || (*in == '-') ||
	       (*in == ':') || (*in == '.'))
	    in++;
	if ((*in > 0) && (*in < 0x80)) {
	    count = in - ctxt->input->cur;
            if (count > maxLength) {
                xmlFatalErr(ctxt, XML_ERR_NAME_TOO_LONG, "Name");
