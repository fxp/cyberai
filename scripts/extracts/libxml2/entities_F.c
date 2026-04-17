 */
xmlChar *
xmlEncodeEntitiesReentrant(xmlDocPtr doc, const xmlChar *input) {
    return xmlEncodeEntitiesInternal(doc, input, 0);
}

/**
 * xmlEncodeSpecialChars:
 * @doc:  the document containing the string
 * @input:  A string to convert to XML.
 *
 * Do a global encoding of a string, replacing the predefined entities
 * this routine is reentrant, and result must be deallocated.
 *
 * Returns A newly allocated string with the substitution done.
 */
xmlChar *
xmlEncodeSpecialChars(const xmlDoc *doc ATTRIBUTE_UNUSED, const xmlChar *input) {
    const xmlChar *cur = input;
    xmlChar *buffer = NULL;
    xmlChar *out = NULL;
    size_t buffer_size = 0;
    if (input == NULL) return(NULL);

    /*
     * allocate an translation buffer.
     */
    buffer_size = 1000;
    buffer = (xmlChar *) xmlMalloc(buffer_size);
    if (buffer == NULL)
	return(NULL);
    out = buffer;

    while (*cur != '\0') {
        size_t indx = out - buffer;
        if (indx + 10 > buffer_size) {

	    growBufferReentrant();
	    out = &buffer[indx];
	}

	/*
	 * By default one have to encode at least '<', '>', '"' and '&' !
	 */
	if (*cur == '<') {
	    *out++ = '&';
	    *out++ = 'l';
	    *out++ = 't';
	    *out++ = ';';
	} else if (*cur == '>') {
	    *out++ = '&';
	    *out++ = 'g';
	    *out++ = 't';
	    *out++ = ';';
	} else if (*cur == '&') {
	    *out++ = '&';
	    *out++ = 'a';
	    *out++ = 'm';
	    *out++ = 'p';
	    *out++ = ';';
	} else if (*cur == '"') {
	    *out++ = '&';
	    *out++ = 'q';
	    *out++ = 'u';
	    *out++ = 'o';
	    *out++ = 't';
	    *out++ = ';';
	} else if (*cur == '\r') {
	    *out++ = '&';
	    *out++ = '#';
	    *out++ = '1';
	    *out++ = '3';
	    *out++ = ';';
	} else {
	    /*
	     * Works because on UTF-8, all extended sequences cannot
	     * result in bytes in the ASCII range.
	     */
	    *out++ = *cur;
	}
	cur++;
    }
    *out = 0;
    return(buffer);

mem_error:
    xmlFree(buffer);
    return(NULL);
}

/**
 * xmlCreateEntitiesTable:
 *
 * create and initialize an empty entities hash table.
 * This really doesn't make sense and should be deprecated
 *
 * Returns the xmlEntitiesTablePtr just created or NULL in case of error.
 */
xmlEntitiesTablePtr
xmlCreateEntitiesTable(void) {
    return((xmlEntitiesTablePtr) xmlHashCreate(0));
}

/**
 * xmlFreeEntityWrapper:
 * @entity:  An entity
 * @name:  its name
 *
 * Deallocate the memory used by an entities in the hash table.
 */
static void
xmlFreeEntityWrapper(void *entity, const xmlChar *name ATTRIBUTE_UNUSED) {
    if (entity != NULL)
	xmlFreeEntity((xmlEntityPtr) entity);
}

/**
 * xmlFreeEntitiesTable:
 * @table:  An entity table
 *
 * Deallocate the memory used by an entities hash table.
 */
void
xmlFreeEntitiesTable(xmlEntitiesTablePtr table) {
    xmlHashFree(table, xmlFreeEntityWrapper);
}

#ifdef LIBXML_TREE_ENABLED
/**
 * xmlCopyEntity:
 * @ent:  An entity
 *
 * Build a copy of an entity
 *
 * Returns the new xmlEntitiesPtr or NULL in case of error.
 */
static void *
xmlCopyEntity(void *payload, const xmlChar *name ATTRIBUTE_UNUSED) {
    xmlEntityPtr ent = (xmlEntityPtr) payload;
    xmlEntityPtr cur;

    cur = (xmlEntityPtr) xmlMalloc(sizeof(xmlEntity));
    if (cur == NULL)
	return(NULL);
    memset(cur, 0, sizeof(xmlEntity));
    cur->type = XML_ENTITY_DECL;

    cur->etype = ent->etype;
    if (ent->name != NULL) {
	cur->name = xmlStrdup(ent->name);
        if (cur->name == NULL)
            goto error;
    }
    if (ent->ExternalID != NULL) {
	cur->ExternalID = xmlStrdup(ent->ExternalID);
        if (cur->ExternalID == NULL)
            goto error;
    }
    if (ent->SystemID != NULL) {
	cur->SystemID = xmlStrdup(ent->SystemID);
        if (cur->SystemID == NULL)
            goto error;
    }
    if (ent->content != NULL) {
	cur->content = xmlStrdup(ent->content);
        if (cur->content == NULL)
            goto error;
    }
    if (ent->orig != NULL) {
	cur->orig = xmlStrdup(ent->orig);
        if (cur->orig == NULL)
            goto error;
    }
    if (ent->URI != NULL) {
	cur->URI = xmlStrdup(ent->URI);
        if (cur->URI == NULL)
            goto error;
    }
    return(cur);

error:
    xmlFreeEntity(cur);
