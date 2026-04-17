        } else if (str[1] == '#') {
            int val;

	    val = xmlParseStringCharRef(ctxt, &str);
	    if (val == 0) {
                pent->content[0] = 0;
                break;
            }
	} else {
            xmlChar *name;
            xmlEntityPtr ent;

	    name = xmlParseStringEntityRef(ctxt, &str);
	    if (name == NULL) {
                pent->content[0] = 0;
                break;
            }

            ent = xmlLookupGeneralEntity(ctxt, name, /* inAttr */ 1);
            xmlFree(name);

            if ((ent != NULL) &&
                (ent->etype != XML_INTERNAL_PREDEFINED_ENTITY)) {
                if ((ent->flags & flags) != flags) {
                    pent->flags |= XML_ENT_EXPANDING;
                    xmlCheckEntityInAttValue(ctxt, ent, depth);
                    pent->flags &= ~XML_ENT_EXPANDING;
                }

                xmlSaturatedAdd(&expandedSize, ent->expandedSize);
                xmlSaturatedAdd(&expandedSize, XML_ENT_FIXED_COST);
            }
        }
    }

done:
    if (ctxt->inSubset == 0)
        pent->expandedSize = expandedSize;

    pent->flags |= flags;
}

/**
 * xmlExpandEntityInAttValue:
 * @ctxt:  parser context
 * @buf:  string buffer
 * @str:  entity or attribute value
 * @pent:  entity for entity value, NULL for attribute values
 * @normalize:  whether to collapse whitespace
 * @inSpace:  whitespace state
 * @depth:  nesting depth
 * @check:  whether to check for amplification
 *
 * Expand general entity references in an entity or attribute value.
 * Perform attribute value normalization.
 */
static void
xmlExpandEntityInAttValue(xmlParserCtxtPtr ctxt, xmlSBuf *buf,
                          const xmlChar *str, xmlEntityPtr pent, int normalize,
                          int *inSpace, int depth, int check) {
    int maxDepth = (ctxt->options & XML_PARSE_HUGE) ? 40 : 20;
    int c, chunkSize;

    if (str == NULL)
        return;

    depth += 1;
    if (depth > maxDepth) {
	xmlFatalErrMsg(ctxt, XML_ERR_RESOURCE_LIMIT,
                       "Maximum entity nesting depth exceeded");
	return;
    }

    if (pent != NULL) {
        if (pent->flags & XML_ENT_EXPANDING) {
            xmlFatalErr(ctxt, XML_ERR_ENTITY_LOOP, NULL);
            xmlHaltParser(ctxt);
            return;
        }

        if (check) {
            if (xmlParserEntityCheck(ctxt, pent->length))
                return;
        }
    }

    chunkSize = 0;

    /*
     * Note that entity values are already validated. No special
     * handling for multi-byte characters is needed.
     */
    while (!PARSER_STOPPED(ctxt)) {
        c = *str;

	if (c != '&') {
            if (c == 0)
                break;

            /*
             * If this function is called without an entity, it is used to
             * expand entities in an attribute content where less-than was
             * already unscaped and is allowed.
             */
            if ((pent != NULL) && (c == '<')) {
                xmlFatalErrMsgStr(ctxt, XML_ERR_LT_IN_ATTRIBUTE,
                        "'<' in entity '%s' is not allowed in attributes "
                        "values\n", pent->name);
                break;
            }

            if (c <= 0x20) {
                if ((normalize) && (*inSpace)) {
                    /* Skip char */
                    if (chunkSize > 0) {
                        xmlSBufAddString(buf, str - chunkSize, chunkSize);
                        chunkSize = 0;
                    }
                } else if (c < 0x20) {
                    if (chunkSize > 0) {
                        xmlSBufAddString(buf, str - chunkSize, chunkSize);
                        chunkSize = 0;
                    }

                    xmlSBufAddCString(buf, " ", 1);
                } else {
                    chunkSize += 1;
                }

                *inSpace = 1;
            } else {
                chunkSize += 1;
                *inSpace = 0;
            }

            str += 1;
