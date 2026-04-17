 * @ctxt:  the XPath Parser context
 *
 * parse an XML name
 *
 * [4] NameChar ::= Letter | Digit | '.' | '-' | '_' | ':' |
 *                  CombiningChar | Extender
 *
 * [5] Name ::= (Letter | '_' | ':') (NameChar)*
 *
 * Returns the namespace name or NULL
 */

xmlChar *
xmlXPathParseName(xmlXPathParserContextPtr ctxt) {
    const xmlChar *in;
    xmlChar *ret;
    size_t count = 0;

    if ((ctxt == NULL) || (ctxt->cur == NULL)) return(NULL);
    /*
     * Accelerator for simple ASCII names
     */
    in = ctxt->cur;
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
	    count = in - ctxt->cur;
            if (count > XML_MAX_NAME_LENGTH) {
                ctxt->cur = in;
                XP_ERRORNULL(XPATH_EXPR_ERROR);
            }
	    ret = xmlStrndup(ctxt->cur, count);
            if (ret == NULL)
                xmlXPathPErrMemory(ctxt);
	    ctxt->cur = in;
	    return(ret);
	}
    }
    return(xmlXPathParseNameComplex(ctxt, 1));
}

static xmlChar *
xmlXPathParseNameComplex(xmlXPathParserContextPtr ctxt, int qualified) {
    xmlChar *ret;
    xmlChar buf[XML_MAX_NAMELEN + 5];
    int len = 0, l;
    int c;

    /*
     * Handler for more complex cases
     */
    c = CUR_CHAR(l);
    if ((c == ' ') || (c == '>') || (c == '/') || /* accelerators */
        (c == '[') || (c == ']') || (c == '@') || /* accelerators */
        (c == '*') || /* accelerators */
	(!IS_LETTER(c) && (c != '_') &&
         ((!qualified) || (c != ':')))) {
	return(NULL);
    }

    while ((c != ' ') && (c != '>') && (c != '/') && /* test bigname.xml */
	   ((IS_LETTER(c)) || (IS_DIGIT(c)) ||
            (c == '.') || (c == '-') ||
	    (c == '_') || ((qualified) && (c == ':')) ||
	    (IS_COMBINING(c)) ||
	    (IS_EXTENDER(c)))) {
	COPY_BUF(l,buf,len,c);
	NEXTL(l);
	c = CUR_CHAR(l);
	if (len >= XML_MAX_NAMELEN) {
	    /*
	     * Okay someone managed to make a huge name, so he's ready to pay
	     * for the processing speed.
	     */
	    xmlChar *buffer;
	    int max = len * 2;

            if (len > XML_MAX_NAME_LENGTH) {
                XP_ERRORNULL(XPATH_EXPR_ERROR);
            }
	    buffer = (xmlChar *) xmlMallocAtomic(max);
	    if (buffer == NULL) {
                xmlXPathPErrMemory(ctxt);
                return(NULL);
	    }
	    memcpy(buffer, buf, len);
	    while ((IS_LETTER(c)) || (IS_DIGIT(c)) || /* test bigname.xml */
		   (c == '.') || (c == '-') ||
		   (c == '_') || ((qualified) && (c == ':')) ||
		   (IS_COMBINING(c)) ||
		   (IS_EXTENDER(c))) {
		if (len + 10 > max) {
                    xmlChar *tmp;
                    if (max > XML_MAX_NAME_LENGTH) {
                        xmlFree(buffer);
                        XP_ERRORNULL(XPATH_EXPR_ERROR);
                    }
		    max *= 2;
		    tmp = (xmlChar *) xmlRealloc(buffer, max);
		    if (tmp == NULL) {
                        xmlFree(buffer);
                        xmlXPathPErrMemory(ctxt);
                        return(NULL);
		    }
                    buffer = tmp;
		}
		COPY_BUF(l,buffer,len,c);
		NEXTL(l);
		c = CUR_CHAR(l);
	    }
	    buffer[len] = 0;
	    return(buffer);
	}
    }
    if (len == 0)
	return(NULL);
    ret = xmlStrndup(buf, len);
    if (ret == NULL)
        xmlXPathPErrMemory(ctxt);
    return(ret);
}

#define MAX_FRAC 20

/**
 * xmlXPathStringEvalNumber:
 * @str:  A string to scan
