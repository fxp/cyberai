            /*
             * tag if some deallocation is needed
             */
            if (alloc != 0) attval = 1;
            attvalue = NULL; /* moved into atts */
        }

next_attr:
        if ((attvalue != NULL) && (alloc != 0)) {
            xmlFree(attvalue);
            attvalue = NULL;
        }

	GROW
	if ((RAW == '>') || (((RAW == '/') && (NXT(1) == '>'))))
	    break;
	if (SKIP_BLANKS == 0) {
	    xmlFatalErrMsg(ctxt, XML_ERR_SPACE_REQUIRED,
			   "attributes construct error\n");
	    break;
	}
        GROW;
    }

    /*
     * Namespaces from default attributes
     */
    if (ctxt->attsDefault != NULL) {
        xmlDefAttrsPtr defaults;

	defaults = xmlHashLookup2(ctxt->attsDefault, localname, prefix);
	if (defaults != NULL) {
	    for (i = 0; i < defaults->nbAttrs; i++) {
                xmlDefAttr *attr = &defaults->attrs[i];

	        attname = attr->name.name;
		aprefix = attr->prefix.name;

		if ((attname == ctxt->str_xmlns) && (aprefix == NULL)) {
                    xmlParserEntityCheck(ctxt, attr->expandedSize);

                    if (xmlParserNsPush(ctxt, NULL, &attr->value, NULL, 1) > 0)
                        nbNs++;
		} else if (aprefix == ctxt->str_xmlns) {
                    xmlParserEntityCheck(ctxt, attr->expandedSize);

                    if (xmlParserNsPush(ctxt, &attr->name, &attr->value,
                                      NULL, 1) > 0)
                        nbNs++;
		} else {
                    nbTotalDef += 1;
                }
	    }
	}
    }

    /*
     * Resolve attribute namespaces
     */
    for (i = 0; i < nbatts; i += 5) {
        attname = atts[i];
        aprefix = atts[i+1];

        /*
	* The default namespace does not apply to attribute names.
	*/
	if (aprefix == NULL) {
            nsIndex = NS_INDEX_EMPTY;
        } else if (aprefix == ctxt->str_xml) {
            nsIndex = NS_INDEX_XML;
        } else {
            haprefix.name = aprefix;
            haprefix.hashValue = (size_t) atts[i+2];
            nsIndex = xmlParserNsLookup(ctxt, &haprefix, NULL);

	    if ((nsIndex == INT_MAX) || (nsIndex < ctxt->nsdb->minNsIndex)) {
                xmlNsErr(ctxt, XML_NS_ERR_UNDEFINED_NAMESPACE,
		    "Namespace prefix %s for %s on %s is not defined\n",
		    aprefix, attname, localname);
                nsIndex = NS_INDEX_EMPTY;
            }
        }

        atts[i+2] = (const xmlChar *) (ptrdiff_t) nsIndex;
    }

    /*
     * Maximum number of attributes including default attributes.
     */
    maxAtts = nratts + nbTotalDef;

    /*
     * Verify that attribute names are unique.
     */
    if (maxAtts > 1) {
        attrHashSize = 4;
        while (attrHashSize / 2 < (unsigned) maxAtts)
            attrHashSize *= 2;

        if (attrHashSize > ctxt->attrHashMax) {
            xmlAttrHashBucket *tmp;

            tmp = xmlRealloc(ctxt->attrHash, attrHashSize * sizeof(tmp[0]));
            if (tmp == NULL) {
                xmlErrMemory(ctxt);
                goto done;
            }

            ctxt->attrHash = tmp;
            ctxt->attrHashMax = attrHashSize;
        }

        memset(ctxt->attrHash, -1, attrHashSize * sizeof(ctxt->attrHash[0]));

        for (i = 0, j = 0; j < nratts; i += 5, j++) {
            const xmlChar *nsuri;
            unsigned hashValue, nameHashValue, uriHashValue;
            int res;

            attname = atts[i];
            aprefix = atts[i+1];
            nsIndex = (ptrdiff_t) atts[i+2];
            /* Hash values always have bit 31 set, see dict.c */
            nameHashValue = ctxt->attallocs[j] | 0x80000000;

