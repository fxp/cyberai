            } else {
                str += 1;
            }
        }
    }

    if (chunk < str)
        xmlSBufAddString(buf, chunk, str - chunk);

    return;
}

/**
 * xmlParseEntityValue:
 * @ctxt:  an XML parser context
 * @orig:  if non-NULL store a copy of the original entity value
 *
 * DEPRECATED: Internal function, don't use.
 *
 * parse a value for ENTITY declarations
 *
 * [9] EntityValue ::= '"' ([^%&"] | PEReference | Reference)* '"' |
 *	               "'" ([^%&'] | PEReference | Reference)* "'"
 *
 * Returns the EntityValue parsed with reference substituted or NULL
 */
xmlChar *
xmlParseEntityValue(xmlParserCtxtPtr ctxt, xmlChar **orig) {
    unsigned maxLength = (ctxt->options & XML_PARSE_HUGE) ?
                         XML_MAX_HUGE_LENGTH :
                         XML_MAX_TEXT_LENGTH;
    xmlSBuf buf;
    const xmlChar *start;
    int quote, length;

    xmlSBufInit(&buf, maxLength);

    GROW;

    quote = CUR;
    if ((quote != '"') && (quote != '\'')) {
	xmlFatalErr(ctxt, XML_ERR_ATTRIBUTE_NOT_STARTED, NULL);
	return(NULL);
    }
    CUR_PTR++;

    length = 0;

    /*
     * Copy raw content of the entity into a buffer
     */
    while (1) {
        int c;

        if (PARSER_STOPPED(ctxt))
            goto error;

        if (CUR_PTR >= ctxt->input->end) {
            xmlFatalErrMsg(ctxt, XML_ERR_ENTITY_NOT_FINISHED, NULL);
            goto error;
        }

        c = CUR;

        if (c == 0) {
            xmlFatalErrMsg(ctxt, XML_ERR_INVALID_CHAR,
                    "invalid character in entity value\n");
            goto error;
        }
        if (c == quote)
            break;
        NEXTL(1);
        length += 1;

        /*
         * TODO: Check growth threshold
         */
        if (ctxt->input->end - CUR_PTR < 10)
            GROW;
    }

    start = CUR_PTR - length;

    if (orig != NULL) {
        *orig = xmlStrndup(start, length);
        if (*orig == NULL)
            xmlErrMemory(ctxt);
    }

    xmlExpandPEsInEntityValue(ctxt, &buf, start, length, ctxt->inputNr);

    NEXTL(1);

    return(xmlSBufFinish(&buf, NULL, ctxt, "entity length too long"));

error:
    xmlSBufCleanup(&buf, ctxt, "entity length too long");
    return(NULL);
}

/**
 * xmlCheckEntityInAttValue:
 * @ctxt:  parser context
 * @pent:  entity
 * @depth:  nesting depth
 *
 * Check an entity reference in an attribute value for validity
 * without expanding it.
 */
static void
xmlCheckEntityInAttValue(xmlParserCtxtPtr ctxt, xmlEntityPtr pent, int depth) {
    int maxDepth = (ctxt->options & XML_PARSE_HUGE) ? 40 : 20;
    const xmlChar *str;
    unsigned long expandedSize = pent->length;
    int c, flags;

    depth += 1;
    if (depth > maxDepth) {
	xmlFatalErrMsg(ctxt, XML_ERR_RESOURCE_LIMIT,
                       "Maximum entity nesting depth exceeded");
	return;
    }

    if (pent->flags & XML_ENT_EXPANDING) {
        xmlFatalErr(ctxt, XML_ERR_ENTITY_LOOP, NULL);
        xmlHaltParser(ctxt);
        return;
    }

    /*
     * If we're parsing a default attribute value in DTD content,
     * the entity might reference other entities which weren't
     * defined yet, so the check isn't reliable.
     */
    if (ctxt->inSubset == 0)
        flags = XML_ENT_CHECKED | XML_ENT_VALIDATED;
    else
        flags = XML_ENT_VALIDATED;

    str = pent->content;
    if (str == NULL)
        goto done;

    /*
     * Note that entity values are already validated. We only check
     * for illegal less-than signs and compute the expanded size
     * of the entity. No special handling for multi-byte characters
     * is needed.
     */
    while (!PARSER_STOPPED(ctxt)) {
        c = *str;

	if (c != '&') {
            if (c == 0)
                break;

            if (c == '<')
                xmlFatalErrMsgStr(ctxt, XML_ERR_LT_IN_ATTRIBUTE,
                        "'<' in entity '%s' is not allowed in attributes "
                        "values\n", pent->name);

            str += 1;
