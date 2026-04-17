                atts[nbatts++] = attname;
                atts[nbatts++] = aprefix;
                atts[nbatts++] = (const xmlChar *) (ptrdiff_t) nsIndex;
                atts[nbatts++] = attr->value.name;
                atts[nbatts++] = attr->valueEnd;
                if ((ctxt->standalone == 1) && (attr->external != 0)) {
                    xmlValidityError(ctxt, XML_DTD_STANDALONE_DEFAULTED,
                            "standalone: attribute %s on %s defaulted "
                            "from external subset\n",
                            attname, localname);
                }
                nbdef++;
	    }
	}
    }

    /*
     * Using a single hash table for nsUri/localName pairs cannot
     * detect duplicate QNames reliably. The following example will
     * only result in two namespace errors.
     *
     * <doc xmlns:a="a" xmlns:b="a">
     *   <elem a:a="" b:a="" b:a=""/>
     * </doc>
     *
     * If we saw more than one namespace error but no duplicate QNames
     * were found, we have to scan for duplicate QNames.
     */
    if ((numDupErr == 0) && (numNsErr > 1)) {
        memset(ctxt->attrHash, -1,
               attrHashSize * sizeof(ctxt->attrHash[0]));

        for (i = 0, j = 0; j < nratts; i += 5, j++) {
            unsigned hashValue, nameHashValue, prefixHashValue;
            int res;

            aprefix = atts[i+1];
            if (aprefix == NULL)
                continue;

            attname = atts[i];
            /* Hash values always have bit 31 set, see dict.c */
            nameHashValue = ctxt->attallocs[j] | 0x80000000;
            prefixHashValue = xmlDictComputeHash(ctxt->dict, aprefix);

            hashValue = xmlDictCombineHash(nameHashValue, prefixHashValue);
            res = xmlAttrHashInsertQName(ctxt, attrHashSize, attname,
                                         aprefix, hashValue, i);
            if (res < INT_MAX)
                xmlErrAttributeDup(ctxt, aprefix, attname);
        }
    }

    /*
     * Reconstruct attribute pointers
     */
    for (i = 0, j = 0; i < nbatts; i += 5, j++) {
        /* namespace URI */
        nsIndex = (ptrdiff_t) atts[i+2];
        if (nsIndex == INT_MAX)
            atts[i+2] = NULL;
        else if (nsIndex == INT_MAX - 1)
            atts[i+2] = ctxt->str_xml_ns;
        else
            atts[i+2] = ctxt->nsTab[nsIndex * 2 + 1];

        if ((j < nratts) && (ctxt->attallocs[j] & 0x80000000) == 0) {
            atts[i+3] = BASE_PTR + (ptrdiff_t) atts[i+3];  /* value */
            atts[i+4] = BASE_PTR + (ptrdiff_t) atts[i+4];  /* valuend */
        }
    }

    uri = xmlParserNsLookupUri(ctxt, &hprefix);
    if ((prefix != NULL) && (uri == NULL)) {
	xmlNsErr(ctxt, XML_NS_ERR_UNDEFINED_NAMESPACE,
	         "Namespace prefix %s on %s is not defined\n",
		 prefix, localname, NULL);
    }
    *pref = prefix;
    *URI = uri;

    /*
     * SAX callback
     */
    if ((ctxt->sax != NULL) && (ctxt->sax->startElementNs != NULL) &&
	(!ctxt->disableSAX)) {
	if (nbNs > 0)
	    ctxt->sax->startElementNs(ctxt->userData, localname, prefix, uri,
                          nbNs, ctxt->nsTab + 2 * (ctxt->nsNr - nbNs),
			  nbatts / 5, nbdef, atts);
	else
	    ctxt->sax->startElementNs(ctxt->userData, localname, prefix, uri,
                          0, NULL, nbatts / 5, nbdef, atts);
    }

done:
    /*
     * Free allocated attribute values
     */
    if (attval != 0) {
	for (i = 0, j = 0; j < nratts; i += 5, j++)
	    if (ctxt->attallocs[j] & 0x80000000)
	        xmlFree((xmlChar *) atts[i+3]);
    }

    *nbNsPtr = nbNs;
    return(localname);
}

/**
