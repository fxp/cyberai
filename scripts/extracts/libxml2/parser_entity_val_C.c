        } else if (str[1] == '#') {
            int val;

            if (chunkSize > 0) {
                xmlSBufAddString(buf, str - chunkSize, chunkSize);
                chunkSize = 0;
            }

	    val = xmlParseStringCharRef(ctxt, &str);
	    if (val == 0) {
                if (pent != NULL)
                    pent->content[0] = 0;
                break;
            }

            if (val == ' ') {
                if ((!normalize) || (!*inSpace))
                    xmlSBufAddCString(buf, " ", 1);
                *inSpace = 1;
            } else {
                xmlSBufAddChar(buf, val);
                *inSpace = 0;
            }
	} else {
            xmlChar *name;
            xmlEntityPtr ent;

            if (chunkSize > 0) {
                xmlSBufAddString(buf, str - chunkSize, chunkSize);
                chunkSize = 0;
            }

	    name = xmlParseStringEntityRef(ctxt, &str);
            if (name == NULL) {
                if (pent != NULL)
                    pent->content[0] = 0;
                break;
            }

            ent = xmlLookupGeneralEntity(ctxt, name, /* inAttr */ 1);
            xmlFree(name);

	    if ((ent != NULL) &&
		(ent->etype == XML_INTERNAL_PREDEFINED_ENTITY)) {
		if (ent->content == NULL) {
		    xmlFatalErrMsg(ctxt, XML_ERR_INTERNAL_ERROR,
			    "predefined entity has no content\n");
                    break;
                }

                xmlSBufAddString(buf, ent->content, ent->length);

                *inSpace = 0;
	    } else if ((ent != NULL) && (ent->content != NULL)) {
                if (pent != NULL)
                    pent->flags |= XML_ENT_EXPANDING;
		xmlExpandEntityInAttValue(ctxt, buf, ent->content, ent,
                                          normalize, inSpace, depth, check);
                if (pent != NULL)
                    pent->flags &= ~XML_ENT_EXPANDING;
	    }
        }
    }

    if (chunkSize > 0)
        xmlSBufAddString(buf, str - chunkSize, chunkSize);

    return;
}

/**
 * xmlExpandEntitiesInAttValue:
 * @ctxt:  parser context
 * @str:  entity or attribute value
 * @normalize:  whether to collapse whitespace
 *
 * Expand general entity references in an entity or attribute value.
 * Perform attribute value normalization.
 *
 * Returns the expanded attribtue value.
 */
xmlChar *
xmlExpandEntitiesInAttValue(xmlParserCtxtPtr ctxt, const xmlChar *str,
                            int normalize) {
    unsigned maxLength = (ctxt->options & XML_PARSE_HUGE) ?
                         XML_MAX_HUGE_LENGTH :
                         XML_MAX_TEXT_LENGTH;
    xmlSBuf buf;
    int inSpace = 1;

    xmlSBufInit(&buf, maxLength);

    xmlExpandEntityInAttValue(ctxt, &buf, str, NULL, normalize, &inSpace,
                              ctxt->inputNr, /* check */ 0);

    if ((normalize) && (inSpace) && (buf.size > 0))
        buf.size--;

    return(xmlSBufFinish(&buf, NULL, ctxt, "AttValue length too long"));
}

/**
 * xmlParseAttValueInternal:
 * @ctxt:  an XML parser context
 * @len:  attribute len result
 * @alloc:  whether the attribute was reallocated as a new string
 * @normalize:  if 1 then further non-CDATA normalization must be done
 *
 * parse a value for an attribute.
 * NOTE: if no normalization is needed, the routine will return pointers
 *       directly from the data buffer.
 *
 * 3.3.3 Attribute-Value Normalization:
 * Before the value of an attribute is passed to the application or
 * checked for validity, the XML processor must normalize it as follows:
 * - a character reference is processed by appending the referenced
 *   character to the attribute value
 * - an entity reference is processed by recursively processing the
 *   replacement text of the entity
 * - a whitespace character (#x20, #xD, #xA, #x9) is processed by
 *   appending #x20 to the normalized value, except that only a single
 *   #x20 is appended for a "#xD#xA" sequence that is part of an external
 *   parsed entity or the literal entity value of an internal parsed entity
 * - other characters are processed by appending them to the normalized value
 * If the declared value is not CDATA, then the XML processor must further
 * process the normalized attribute value by discarding any leading and
 * trailing space (#x20) characters, and by replacing sequences of space
 * (#x20) characters by a single space (#x20) character.
 * All attributes for which no declaration has been read should be treated
 * by a non-validating parser as if declared CDATA.
 *
 * Returns the AttValue parsed or NULL. The value has to be freed by the
 *     caller if it was copied, this can be detected by val[*len] == 0.
 */
static xmlChar *
