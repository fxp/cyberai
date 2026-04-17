xmlXPathNodeSetFilter(xmlXPathParserContextPtr ctxt,
		      xmlNodeSetPtr set,
		      int filterOpIndex,
                      int minPos, int maxPos,
		      int hasNsNodes)
{
    xmlXPathContextPtr xpctxt;
    xmlNodePtr oldnode;
    xmlDocPtr olddoc;
    xmlXPathStepOpPtr filterOp;
    int oldcs, oldpp;
    int i, j, pos;

    if ((set == NULL) || (set->nodeNr == 0))
        return;

    /*
    * Check if the node set contains a sufficient number of nodes for
    * the requested range.
    */
    if (set->nodeNr < minPos) {
        xmlXPathNodeSetClear(set, hasNsNodes);
        return;
    }

    xpctxt = ctxt->context;
    oldnode = xpctxt->node;
    olddoc = xpctxt->doc;
    oldcs = xpctxt->contextSize;
    oldpp = xpctxt->proximityPosition;
    filterOp = &ctxt->comp->steps[filterOpIndex];

    xpctxt->contextSize = set->nodeNr;

    for (i = 0, j = 0, pos = 1; i < set->nodeNr; i++) {
        xmlNodePtr node = set->nodeTab[i];
        int res;

        xpctxt->node = node;
        xpctxt->proximityPosition = i + 1;

        /*
        * Also set the xpath document in case things like
        * key() are evaluated in the predicate.
        *
        * TODO: Get real doc for namespace nodes.
        */
        if ((node->type != XML_NAMESPACE_DECL) &&
            (node->doc != NULL))
            xpctxt->doc = node->doc;

        res = xmlXPathCompOpEvalToBoolean(ctxt, filterOp, 1);

        if (ctxt->error != XPATH_EXPRESSION_OK)
            break;
        if (res < 0) {
            /* Shouldn't happen */
            xmlXPathErr(ctxt, XPATH_EXPR_ERROR);
            break;
        }

        if ((res != 0) && ((pos >= minPos) && (pos <= maxPos))) {
            if (i != j) {
                set->nodeTab[j] = node;
                set->nodeTab[i] = NULL;
            }

            j += 1;
        } else {
            /* Remove the entry from the initial node set. */
            set->nodeTab[i] = NULL;
            if (node->type == XML_NAMESPACE_DECL)
                xmlXPathNodeSetFreeNs((xmlNsPtr) node);
        }

        if (res != 0) {
            if (pos == maxPos) {
                i += 1;
                break;
            }

            pos += 1;
        }
    }

    /* Free remaining nodes. */
    if (hasNsNodes) {
        for (; i < set->nodeNr; i++) {
            xmlNodePtr node = set->nodeTab[i];
            if ((node != NULL) && (node->type == XML_NAMESPACE_DECL))
                xmlXPathNodeSetFreeNs((xmlNsPtr) node);
        }
    }

    set->nodeNr = j;

    /* If too many elements were removed, shrink table to preserve memory. */
    if ((set->nodeMax > XML_NODESET_DEFAULT) &&
        (set->nodeNr < set->nodeMax / 2)) {
        xmlNodePtr *tmp;
        int nodeMax = set->nodeNr;

        if (nodeMax < XML_NODESET_DEFAULT)
            nodeMax = XML_NODESET_DEFAULT;
        tmp = (xmlNodePtr *) xmlRealloc(set->nodeTab,
                nodeMax * sizeof(xmlNodePtr));
        if (tmp == NULL) {
            xmlXPathPErrMemory(ctxt);
        } else {
            set->nodeTab = tmp;
            set->nodeMax = nodeMax;
        }
    }

    xpctxt->node = oldnode;
    xpctxt->doc = olddoc;
    xpctxt->contextSize = oldcs;
    xpctxt->proximityPosition = oldpp;
}

#ifdef LIBXML_XPTR_LOCS_ENABLED
/**
 * xmlXPathLocationSetFilter:
 * @ctxt:  the XPath Parser context
