            if (nsIndex == NS_INDEX_EMPTY) {
                /*
                 * Prefix with empty namespace means an undeclared
                 * prefix which was already reported above.
                 */
                if (aprefix != NULL)
                    continue;
                nsuri = NULL;
                uriHashValue = URI_HASH_EMPTY;
            } else if (nsIndex == NS_INDEX_XML) {
                nsuri = ctxt->str_xml_ns;
                uriHashValue = URI_HASH_XML;
            } else {
                nsuri = ctxt->nsTab[nsIndex * 2 + 1];
                uriHashValue = ctxt->nsdb->extra[nsIndex].uriHashValue;
            }

            hashValue = xmlDictCombineHash(nameHashValue, uriHashValue);
            res = xmlAttrHashInsert(ctxt, attrHashSize, attname, nsuri,
                                    hashValue, i);
            if (res < 0)
                continue;

            /*
             * [ WFC: Unique Att Spec ]
             * No attribute name may appear more than once in the same
             * start-tag or empty-element tag.
             * As extended by the Namespace in XML REC.
             */
            if (res < INT_MAX) {
                if (aprefix == atts[res+1]) {
                    xmlErrAttributeDup(ctxt, aprefix, attname);
                    numDupErr += 1;
                } else {
                    xmlNsErr(ctxt, XML_NS_ERR_ATTRIBUTE_REDEFINED,
                             "Namespaced Attribute %s in '%s' redefined\n",
                             attname, nsuri, NULL);
                    numNsErr += 1;
                }
            }
        }
    }

    /*
     * Default attributes
     */
    if (ctxt->attsDefault != NULL) {
        xmlDefAttrsPtr defaults;

	defaults = xmlHashLookup2(ctxt->attsDefault, localname, prefix);
	if (defaults != NULL) {
	    for (i = 0; i < defaults->nbAttrs; i++) {
                xmlDefAttr *attr = &defaults->attrs[i];
                const xmlChar *nsuri;
                unsigned hashValue, uriHashValue;
                int res;

	        attname = attr->name.name;
		aprefix = attr->prefix.name;

		if ((attname == ctxt->str_xmlns) && (aprefix == NULL))
                    continue;
		if (aprefix == ctxt->str_xmlns)
                    continue;

                if (aprefix == NULL) {
                    nsIndex = NS_INDEX_EMPTY;
                    nsuri = NULL;
                    uriHashValue = URI_HASH_EMPTY;
                } if (aprefix == ctxt->str_xml) {
                    nsIndex = NS_INDEX_XML;
                    nsuri = ctxt->str_xml_ns;
                    uriHashValue = URI_HASH_XML;
                } else if (aprefix != NULL) {
                    nsIndex = xmlParserNsLookup(ctxt, &attr->prefix, NULL);
                    if ((nsIndex == INT_MAX) ||
                        (nsIndex < ctxt->nsdb->minNsIndex)) {
                        xmlNsErr(ctxt, XML_NS_ERR_UNDEFINED_NAMESPACE,
                                 "Namespace prefix %s for %s on %s is not "
                                 "defined\n",
                                 aprefix, attname, localname);
                        nsIndex = NS_INDEX_EMPTY;
                        nsuri = NULL;
                        uriHashValue = URI_HASH_EMPTY;
                    } else {
                        nsuri = ctxt->nsTab[nsIndex * 2 + 1];
                        uriHashValue = ctxt->nsdb->extra[nsIndex].uriHashValue;
                    }
                }

                /*
                 * Check whether the attribute exists
                 */
                if (maxAtts > 1) {
                    hashValue = xmlDictCombineHash(attr->name.hashValue,
                                                   uriHashValue);
                    res = xmlAttrHashInsert(ctxt, attrHashSize, attname, nsuri,
                                            hashValue, nbatts);
                    if (res < 0)
                        continue;
                    if (res < INT_MAX) {
                        if (aprefix == atts[res+1])
                            continue;
                        xmlNsErr(ctxt, XML_NS_ERR_ATTRIBUTE_REDEFINED,
                                 "Namespaced Attribute %s in '%s' redefined\n",
                                 attname, nsuri, NULL);
                    }
                }

                xmlParserEntityCheck(ctxt, attr->expandedSize);

                if ((atts == NULL) || (nbatts + 5 > maxatts)) {
                    if (xmlCtxtGrowAttrs(ctxt, nbatts + 5) < 0) {
                        localname = NULL;
                        goto done;
                    }
                    maxatts = ctxt->maxatts;
                    atts = ctxt->atts;
                }

