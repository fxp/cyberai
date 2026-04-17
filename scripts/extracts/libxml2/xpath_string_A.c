xmlXPathSubstringFunction(xmlXPathParserContextPtr ctxt, int nargs) {
    xmlXPathObjectPtr str, start, len;
    double le=0, in;
    int i = 1, j = INT_MAX;

    if (nargs < 2) {
	CHECK_ARITY(2);
    }
    if (nargs > 3) {
	CHECK_ARITY(3);
    }
    /*
     * take care of possible last (position) argument
    */
    if (nargs == 3) {
	CAST_TO_NUMBER;
	CHECK_TYPE(XPATH_NUMBER);
	len = valuePop(ctxt);
	le = len->floatval;
	xmlXPathReleaseObject(ctxt->context, len);
    }

    CAST_TO_NUMBER;
    CHECK_TYPE(XPATH_NUMBER);
    start = valuePop(ctxt);
    in = start->floatval;
    xmlXPathReleaseObject(ctxt->context, start);
    CAST_TO_STRING;
    CHECK_TYPE(XPATH_STRING);
    str = valuePop(ctxt);

    if (!(in < INT_MAX)) { /* Logical NOT to handle NaNs */
        i = INT_MAX;
    } else if (in >= 1.0) {
        i = (int)in;
        if (in - floor(in) >= 0.5)
            i += 1;
    }

    if (nargs == 3) {
        double rin, rle, end;

        rin = floor(in);
        if (in - rin >= 0.5)
            rin += 1.0;

        rle = floor(le);
        if (le - rle >= 0.5)
            rle += 1.0;

        end = rin + rle;
        if (!(end >= 1.0)) { /* Logical NOT to handle NaNs */
            j = 1;
        } else if (end < INT_MAX) {
            j = (int)end;
        }
    }

    i -= 1;
    j -= 1;

    if ((i < j) && (i < xmlUTF8Strlen(str->stringval))) {
        xmlChar *ret = xmlUTF8Strsub(str->stringval, i, j - i);
        if (ret == NULL)
            xmlXPathPErrMemory(ctxt);
	valuePush(ctxt, xmlXPathCacheNewString(ctxt, ret));
	xmlFree(ret);
    } else {
	valuePush(ctxt, xmlXPathCacheNewCString(ctxt, ""));
    }

    xmlXPathReleaseObject(ctxt->context, str);
}

/**
 * xmlXPathSubstringBeforeFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the substring-before() XPath function
 *    string substring-before(string, string)
 * The substring-before function returns the substring of the first
 * argument string that precedes the first occurrence of the second
 * argument string in the first argument string, or the empty string
 * if the first argument string does not contain the second argument
 * string. For example, substring-before("1999/04/01","/") returns 1999.
 */
void
xmlXPathSubstringBeforeFunction(xmlXPathParserContextPtr ctxt, int nargs) {
    xmlXPathObjectPtr str = NULL;
    xmlXPathObjectPtr find = NULL;
    const xmlChar *point;
    xmlChar *result;

    CHECK_ARITY(2);
    CAST_TO_STRING;
    find = valuePop(ctxt);
    CAST_TO_STRING;
    str = valuePop(ctxt);
    if (ctxt->error != 0)
        goto error;

    point = xmlStrstr(str->stringval, find->stringval);
    if (point == NULL) {
        result = xmlStrdup(BAD_CAST "");
    } else {
        result = xmlStrndup(str->stringval, point - str->stringval);
    }
    if (result == NULL) {
        xmlXPathPErrMemory(ctxt);
        goto error;
    }
    valuePush(ctxt, xmlXPathCacheWrapString(ctxt, result));

error:
    xmlXPathReleaseObject(ctxt->context, str);
    xmlXPathReleaseObject(ctxt->context, find);
}

/**
 * xmlXPathSubstringAfterFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the substring-after() XPath function
 *    string substring-after(string, string)
 * The substring-after function returns the substring of the first
 * argument string that follows the first occurrence of the second
 * argument string in the first argument string, or the empty string
 * if the first argument string does not contain the second argument
 * string. For example, substring-after("1999/04/01","/") returns 04/01,
 * and substring-after("1999/04/01","19") returns 99/04/01.
 */
void
xmlXPathSubstringAfterFunction(xmlXPathParserContextPtr ctxt, int nargs) {
    xmlXPathObjectPtr str = NULL;
    xmlXPathObjectPtr find = NULL;
    const xmlChar *point;
    xmlChar *result;

    CHECK_ARITY(2);
    CAST_TO_STRING;
    find = valuePop(ctxt);
    CAST_TO_STRING;
    str = valuePop(ctxt);
    if (ctxt->error != 0)
        goto error;

    point = xmlStrstr(str->stringval, find->stringval);
