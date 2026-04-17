                xmlSBufAddString(&buf, CUR_PTR - chunkSize, chunkSize);
                chunkSize = 0;
            }

            val = xmlParseCharRef(ctxt);
            if (val == 0)
                goto error;

            if ((val == '&') && (!replaceEntities)) {
                /*
                 * The reparsing will be done in xmlStringGetNodeList()
                 * called by the attribute() function in SAX.c
                 */
                xmlSBufAddCString(&buf, "&#38;", 5);
                inSpace = 0;
            } else if (val == ' ') {
                if ((!normalize) || (!inSpace))
                    xmlSBufAddCString(&buf, " ", 1);
                inSpace = 1;
            } else {
                xmlSBufAddChar(&buf, val);
                inSpace = 0;
            }
        } else {
            const xmlChar *name;
            xmlEntityPtr ent;

            if (chunkSize > 0) {
                xmlSBufAddString(&buf, CUR_PTR - chunkSize, chunkSize);
                chunkSize = 0;
            }

            name = xmlParseEntityRefInternal(ctxt);
            if (name == NULL) {
                /*
                 * Probably a literal '&' which wasn't escaped.
                 * TODO: Handle gracefully in recovery mode.
                 */
                continue;
            }

            ent = xmlLookupGeneralEntity(ctxt, name, /* isAttr */ 1);
            if (ent == NULL)
                continue;

            if (ent->etype == XML_INTERNAL_PREDEFINED_ENTITY) {
                if ((ent->content[0] == '&') && (!replaceEntities))
                    xmlSBufAddCString(&buf, "&#38;", 5);
                else
                    xmlSBufAddString(&buf, ent->content, ent->length);
                inSpace = 0;
            } else if (replaceEntities) {
                xmlExpandEntityInAttValue(ctxt, &buf, ent->content, ent,
                                          normalize, &inSpace, ctxt->inputNr,
                                          /* check */ 1);
            } else {
                if ((ent->flags & flags) != flags)
                    xmlCheckEntityInAttValue(ctxt, ent, ctxt->inputNr);

                if (xmlParserEntityCheck(ctxt, ent->expandedSize)) {
                    ent->content[0] = 0;
                    goto error;
                }

                /*
                 * Just output the reference
                 */
                xmlSBufAddCString(&buf, "&", 1);
                xmlSBufAddString(&buf, ent->name, xmlStrlen(ent->name));
                xmlSBufAddCString(&buf, ";", 1);

                inSpace = 0;
            }
	}
    }

    if ((buf.mem == NULL) && (alloc != NULL)) {
        ret = (xmlChar *) CUR_PTR - chunkSize;

        if (attlen != NULL)
            *attlen = chunkSize;
        if ((normalize) && (inSpace) && (chunkSize > 0))
            *attlen -= 1;
        *alloc = 0;

        /* Report potential error */
        xmlSBufCleanup(&buf, ctxt, "AttValue length too long");
    } else {
        if (chunkSize > 0)
            xmlSBufAddString(&buf, CUR_PTR - chunkSize, chunkSize);

        if ((normalize) && (inSpace) && (buf.size > 0))
            buf.size--;

        ret = xmlSBufFinish(&buf, attlen, ctxt, "AttValue length too long");

        if (ret != NULL) {
            if (attlen != NULL)
                *attlen = buf.size;
            if (alloc != NULL)
                *alloc = 1;
        }
    }

    NEXTL(1);

    return(ret);

error:
    xmlSBufCleanup(&buf, ctxt, "AttValue length too long");
    return(NULL);
}

/**
 * xmlParseAttValue:
