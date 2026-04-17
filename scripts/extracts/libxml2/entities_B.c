    }
    ret->URI = NULL; /* to be computed by the layer knowing
			the defining entity */
    ret->orig = NULL;

    return(ret);

error:
    xmlFreeEntity(ret);
    return(NULL);
}

/**
 * xmlAddEntity:
 * @doc:  the document
 * @extSubset:  add to the external or internal subset
 * @name:  the entity name
 * @type:  the entity type XML_xxx_yyy_ENTITY
 * @ExternalID:  the entity external ID if available
 * @SystemID:  the entity system ID if available
 * @content:  the entity content
 * @out:  pointer to resulting entity (optional)
 *
 * Register a new entity for this document.
 *
 * Available since 2.13.0.
 *
 * Returns an xmlParserErrors error code.
 */
int
xmlAddEntity(xmlDocPtr doc, int extSubset, const xmlChar *name, int type,
	  const xmlChar *ExternalID, const xmlChar *SystemID,
	  const xmlChar *content, xmlEntityPtr *out) {
    xmlDtdPtr dtd;
    xmlDictPtr dict = NULL;
    xmlEntitiesTablePtr table = NULL;
    xmlEntityPtr ret, predef;
    int res;

    if (out != NULL)
        *out = NULL;
    if ((doc == NULL) || (name == NULL))
	return(XML_ERR_ARGUMENT);
    dict = doc->dict;

    if (extSubset)
        dtd = doc->extSubset;
    else
        dtd = doc->intSubset;
    if (dtd == NULL)
        return(XML_DTD_NO_DTD);

    switch (type) {
        case XML_INTERNAL_GENERAL_ENTITY:
        case XML_EXTERNAL_GENERAL_PARSED_ENTITY:
        case XML_EXTERNAL_GENERAL_UNPARSED_ENTITY:
            predef = xmlGetPredefinedEntity(name);
            if (predef != NULL) {
                int valid = 0;

                /* 4.6 Predefined Entities */
                if ((type == XML_INTERNAL_GENERAL_ENTITY) &&
                    (content != NULL)) {
                    int c = predef->content[0];

                    if (((content[0] == c) && (content[1] == 0)) &&
                        ((c == '>') || (c == '\'') || (c == '"'))) {
                        valid = 1;
                    } else if ((content[0] == '&') && (content[1] == '#')) {
                        if (content[2] == 'x') {
                            xmlChar *hex = BAD_CAST "0123456789ABCDEF";
                            xmlChar ref[] = "00;";

                            ref[0] = hex[c / 16 % 16];
                            ref[1] = hex[c % 16];
                            if (xmlStrcasecmp(&content[3], ref) == 0)
                                valid = 1;
                        } else {
                            xmlChar ref[] = "00;";

                            ref[0] = '0' + c / 10 % 10;
                            ref[1] = '0' + c % 10;
                            if (xmlStrEqual(&content[2], ref))
                                valid = 1;
                        }
                    }
                }
                if (!valid)
                    return(XML_ERR_REDECL_PREDEF_ENTITY);
            }
	    if (dtd->entities == NULL) {
		dtd->entities = xmlHashCreateDict(0, dict);
                if (dtd->entities == NULL)
                    return(XML_ERR_NO_MEMORY);
            }
	    table = dtd->entities;
	    break;
        case XML_INTERNAL_PARAMETER_ENTITY:
        case XML_EXTERNAL_PARAMETER_ENTITY:
	    if (dtd->pentities == NULL) {
		dtd->pentities = xmlHashCreateDict(0, dict);
                if (dtd->pentities == NULL)
                    return(XML_ERR_NO_MEMORY);
            }
	    table = dtd->pentities;
	    break;
        default:
	    return(XML_ERR_ARGUMENT);
    }
    ret = xmlCreateEntity(dtd->doc, name, type, ExternalID, SystemID, content);
    if (ret == NULL)
        return(XML_ERR_NO_MEMORY);

    res = xmlHashAdd(table, name, ret);
    if (res < 0) {
        xmlFreeEntity(ret);
        return(XML_ERR_NO_MEMORY);
    } else if (res == 0) {
	/*
	 * entity was already defined at another level.
	 */
        xmlFreeEntity(ret);
	return(XML_WAR_ENTITY_REDEFINED);
    }

    /*
     * Link it to the DTD
     */
    ret->parent = dtd;
    ret->doc = dtd->doc;
    if (dtd->last == NULL) {
