xmlXPathNodeSetMergeAndClear(xmlNodeSetPtr set1, xmlNodeSetPtr set2)
{
    {
	int i, j, initNbSet1;
	xmlNodePtr n1, n2;

	initNbSet1 = set1->nodeNr;
	for (i = 0;i < set2->nodeNr;i++) {
	    n2 = set2->nodeTab[i];
	    /*
	    * Skip duplicates.
	    */
	    for (j = 0; j < initNbSet1; j++) {
		n1 = set1->nodeTab[j];
		if (n1 == n2) {
		    goto skip_node;
		} else if ((n1->type == XML_NAMESPACE_DECL) &&
		    (n2->type == XML_NAMESPACE_DECL))
		{
		    if ((((xmlNsPtr) n1)->next == ((xmlNsPtr) n2)->next) &&
			(xmlStrEqual(((xmlNsPtr) n1)->prefix,
			((xmlNsPtr) n2)->prefix)))
		    {
			/*
			* Free the namespace node.
			*/
			xmlXPathNodeSetFreeNs((xmlNsPtr) n2);
			goto skip_node;
		    }
		}
	    }
	    /*
	    * grow the nodeTab if needed
	    */
	    if (set1->nodeMax == 0) {
		set1->nodeTab = (xmlNodePtr *) xmlMalloc(
		    XML_NODESET_DEFAULT * sizeof(xmlNodePtr));
		if (set1->nodeTab == NULL)
		    goto error;
		memset(set1->nodeTab, 0,
		    XML_NODESET_DEFAULT * sizeof(xmlNodePtr));
		set1->nodeMax = XML_NODESET_DEFAULT;
	    } else if (set1->nodeNr >= set1->nodeMax) {
		xmlNodePtr *temp;

                if (set1->nodeMax >= XPATH_MAX_NODESET_LENGTH)
                    goto error;
		temp = (xmlNodePtr *) xmlRealloc(
		    set1->nodeTab, set1->nodeMax * 2 * sizeof(xmlNodePtr));
		if (temp == NULL)
		    goto error;
		set1->nodeTab = temp;
		set1->nodeMax *= 2;
	    }
	    set1->nodeTab[set1->nodeNr++] = n2;
skip_node:
            set2->nodeTab[i] = NULL;
	}
    }
    set2->nodeNr = 0;
    return(set1);

error:
    xmlXPathFreeNodeSet(set1);
    xmlXPathNodeSetClear(set2, 1);
    return(NULL);
}

/**
 * xmlXPathNodeSetMergeAndClearNoDupls:
 * @set1:  the first NodeSet or NULL
 * @set2:  the second NodeSet
 *
 * Merges two nodesets, all nodes from @set2 are added to @set1.
 * Doesn't check for duplicate nodes. Clears set2.
 *
 * Returns @set1 once extended or NULL in case of error.
 *
 * Frees @set1 in case of error.
 */
static xmlNodeSetPtr
xmlXPathNodeSetMergeAndClearNoDupls(xmlNodeSetPtr set1, xmlNodeSetPtr set2)
{
    {
	int i;
	xmlNodePtr n2;

	for (i = 0;i < set2->nodeNr;i++) {
	    n2 = set2->nodeTab[i];
	    if (set1->nodeMax == 0) {
		set1->nodeTab = (xmlNodePtr *) xmlMalloc(
		    XML_NODESET_DEFAULT * sizeof(xmlNodePtr));
		if (set1->nodeTab == NULL)
		    goto error;
		memset(set1->nodeTab, 0,
		    XML_NODESET_DEFAULT * sizeof(xmlNodePtr));
		set1->nodeMax = XML_NODESET_DEFAULT;
	    } else if (set1->nodeNr >= set1->nodeMax) {
		xmlNodePtr *temp;

                if (set1->nodeMax >= XPATH_MAX_NODESET_LENGTH)
                    goto error;
		temp = (xmlNodePtr *) xmlRealloc(
		    set1->nodeTab, set1->nodeMax * 2 * sizeof(xmlNodePtr));
		if (temp == NULL)
		    goto error;
		set1->nodeTab = temp;
		set1->nodeMax *= 2;
	    }
	    set1->nodeTab[set1->nodeNr++] = n2;
            set2->nodeTab[i] = NULL;
	}
    }
    set2->nodeNr = 0;
    return(set1);

error:
    xmlXPathFreeNodeSet(set1);
    xmlXPathNodeSetClear(set2, 1);
    return(NULL);
}

/**
 * xmlXPathNodeSetDel:
 * @cur:  the initial node set
 * @val:  an xmlNodePtr
 *
 * Removes an xmlNodePtr from an existing NodeSet
 */
void
xmlXPathNodeSetDel(xmlNodeSetPtr cur, xmlNodePtr val) {
    int i;

    if (cur == NULL) return;
    if (val == NULL) return;
