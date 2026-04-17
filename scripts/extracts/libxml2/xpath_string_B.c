    if (point == NULL) {
        result = xmlStrdup(BAD_CAST "");
    } else {
        result = xmlStrdup(point + xmlStrlen(find->stringval));
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
 * xmlXPathNormalizeFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the normalize-space() XPath function
 *    string normalize-space(string?)
 * The normalize-space function returns the argument string with white
 * space normalized by stripping leading and trailing whitespace
 * and replacing sequences of whitespace characters by a single
 * space. Whitespace characters are the same allowed by the S production
 * in XML. If the argument is omitted, it defaults to the context
 * node converted to a string, in other words the value of the context node.
 */
void
xmlXPathNormalizeFunction(xmlXPathParserContextPtr ctxt, int nargs) {
    xmlChar *source, *target;
    int blank;

    if (ctxt == NULL) return;
    if (nargs == 0) {
        /* Use current context node */
        source = xmlXPathCastNodeToString(ctxt->context->node);
        if (source == NULL)
            xmlXPathPErrMemory(ctxt);
        valuePush(ctxt, xmlXPathCacheWrapString(ctxt, source));
        nargs = 1;
    }

    CHECK_ARITY(1);
    CAST_TO_STRING;
    CHECK_TYPE(XPATH_STRING);
    source = ctxt->value->stringval;
    if (source == NULL)
        return;
    target = source;

    /* Skip leading whitespaces */
    while (IS_BLANK_CH(*source))
        source++;

    /* Collapse intermediate whitespaces, and skip trailing whitespaces */
    blank = 0;
    while (*source) {
        if (IS_BLANK_CH(*source)) {
	    blank = 1;
        } else {
            if (blank) {
                *target++ = 0x20;
                blank = 0;
            }
            *target++ = *source;
        }
        source++;
    }
    *target = 0;
}

/**
 * xmlXPathTranslateFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the translate() XPath function
 *    string translate(string, string, string)
 * The translate function returns the first argument string with
 * occurrences of characters in the second argument string replaced
 * by the character at the corresponding position in the third argument
 * string. For example, translate("bar","abc","ABC") returns the string
 * BAr. If there is a character in the second argument string with no
 * character at a corresponding position in the third argument string
 * (because the second argument string is longer than the third argument
 * string), then occurrences of that character in the first argument
 * string are removed. For example, translate("--aaa--","abc-","ABC")
 * returns "AAA". If a character occurs more than once in second
 * argument string, then the first occurrence determines the replacement
 * character. If the third argument string is longer than the second
 * argument string, then excess characters are ignored.
 */
void
xmlXPathTranslateFunction(xmlXPathParserContextPtr ctxt, int nargs) {
    xmlXPathObjectPtr str = NULL;
    xmlXPathObjectPtr from = NULL;
    xmlXPathObjectPtr to = NULL;
    xmlBufPtr target;
    int offset, max;
    int ch;
    const xmlChar *point;
    xmlChar *cptr, *content;

    CHECK_ARITY(3);

    CAST_TO_STRING;
    to = valuePop(ctxt);
    CAST_TO_STRING;
    from = valuePop(ctxt);
    CAST_TO_STRING;
    str = valuePop(ctxt);
    if (ctxt->error != 0)
        goto error;

    /*
     * Account for quadratic runtime
     */
    if (ctxt->context->opLimit != 0) {
        unsigned long f1 = xmlStrlen(from->stringval);
        unsigned long f2 = xmlStrlen(str->stringval);

        if ((f1 > 0) && (f2 > 0)) {
            unsigned long p;

            f1 = f1 / 10 + 1;
            f2 = f2 / 10 + 1;
            p = f1 > ULONG_MAX / f2 ? ULONG_MAX : f1 * f2;
            if (xmlXPathCheckOpLimit(ctxt, p) < 0)
                goto error;
        }
    }

    target = xmlBufCreateSize(64);
