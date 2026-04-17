                    if (attname != ctxt->str_xml) {
                        xmlNsErr(ctxt, XML_NS_ERR_XML_NAMESPACE,
                     "xml namespace URI cannot be the default namespace\n",
                                 NULL, NULL, NULL);
                    }
                    goto next_attr;
                }
                if ((len == 29) &&
                    (xmlStrEqual(uri,
                             BAD_CAST "http://www.w3.org/2000/xmlns/"))) {
                    xmlNsErr(ctxt, XML_NS_ERR_XML_NAMESPACE,
                         "reuse of the xmlns namespace name is forbidden\n",
                             NULL, NULL, NULL);
                    goto next_attr;
                }
            }

            if (xmlParserNsPush(ctxt, NULL, &huri, NULL, 0) > 0)
                nbNs++;
        } else if (aprefix == ctxt->str_xmlns) {
            xmlHashedString huri;
            xmlURIPtr parsedUri;

            huri = xmlDictLookupHashed(ctxt->dict, attvalue, len);
            uri = huri.name;
            if (uri == NULL) {
                xmlErrMemory(ctxt);
                goto next_attr;
            }

            if (attname == ctxt->str_xml) {
                if (uri != ctxt->str_xml_ns) {
                    xmlNsErr(ctxt, XML_NS_ERR_XML_NAMESPACE,
                             "xml namespace prefix mapped to wrong URI\n",
                             NULL, NULL, NULL);
                }
                /*
                 * Do not keep a namespace definition node
                 */
                goto next_attr;
            }
            if (uri == ctxt->str_xml_ns) {
                if (attname != ctxt->str_xml) {
                    xmlNsErr(ctxt, XML_NS_ERR_XML_NAMESPACE,
                             "xml namespace URI mapped to wrong prefix\n",
                             NULL, NULL, NULL);
                }
                goto next_attr;
            }
            if (attname == ctxt->str_xmlns) {
                xmlNsErr(ctxt, XML_NS_ERR_XML_NAMESPACE,
                         "redefinition of the xmlns prefix is forbidden\n",
                         NULL, NULL, NULL);
                goto next_attr;
            }
            if ((len == 29) &&
                (xmlStrEqual(uri,
                             BAD_CAST "http://www.w3.org/2000/xmlns/"))) {
                xmlNsErr(ctxt, XML_NS_ERR_XML_NAMESPACE,
                         "reuse of the xmlns namespace name is forbidden\n",
                         NULL, NULL, NULL);
                goto next_attr;
            }
            if ((uri == NULL) || (uri[0] == 0)) {
                xmlNsErr(ctxt, XML_NS_ERR_XML_NAMESPACE,
                         "xmlns:%s: Empty XML namespace is not allowed\n",
                              attname, NULL, NULL);
                goto next_attr;
            } else {
                if (xmlParseURISafe((const char *) uri, &parsedUri) < 0) {
                    xmlErrMemory(ctxt);
                    goto next_attr;
                }
                if (parsedUri == NULL) {
                    xmlNsErr(ctxt, XML_WAR_NS_URI,
                         "xmlns:%s: '%s' is not a valid URI\n",
                                       attname, uri, NULL);
                } else {
                    if ((ctxt->pedantic) && (parsedUri->scheme == NULL)) {
                        xmlNsWarn(ctxt, XML_WAR_NS_URI_RELATIVE,
                                  "xmlns:%s: URI %s is not absolute\n",
                                  attname, uri, NULL);
                    }
                    xmlFreeURI(parsedUri);
                }
            }

            if (xmlParserNsPush(ctxt, &hattname, &huri, NULL, 0) > 0)
                nbNs++;
        } else {
            /*
             * Populate attributes array, see above for repurposing
             * of xmlChar pointers.
             */
            if ((atts == NULL) || (nbatts + 5 > maxatts)) {
                if (xmlCtxtGrowAttrs(ctxt, nbatts + 5) < 0) {
                    goto next_attr;
                }
                maxatts = ctxt->maxatts;
                atts = ctxt->atts;
            }
            ctxt->attallocs[nratts++] = (hattname.hashValue & 0x7FFFFFFF) |
                                        ((unsigned) alloc << 31);
            atts[nbatts++] = attname;
            atts[nbatts++] = aprefix;
            atts[nbatts++] = (const xmlChar *) (size_t) haprefix.hashValue;
            if (alloc) {
                atts[nbatts++] = attvalue;
                attvalue += len;
                atts[nbatts++] = attvalue;
            } else {
                /*
                 * attvalue points into the input buffer which can be
                 * reallocated. Store differences to input->base instead.
                 * The pointers will be reconstructed later.
                 */
                atts[nbatts++] = (void *) (attvalue - BASE_PTR);
                attvalue += len;
                atts[nbatts++] = (void *) (attvalue - BASE_PTR);
            }
