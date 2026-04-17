/*
 * entities.c : implementation for the XML entities handling
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

/* To avoid EBCDIC trouble when parsing on zOS */
#if defined(__MVS__)
#pragma convert("ISO8859-1")
#endif

#define IN_LIBXML
#include "libxml.h"

#include <string.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>
#include <libxml/hash.h>
#include <libxml/entities.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlerror.h>
#include <libxml/dict.h>
#include <libxml/xmlsave.h>

#include "private/entities.h"
#include "private/error.h"

/*
 * The XML predefined entities.
 */

static xmlEntity xmlEntityLt = {
    NULL, XML_ENTITY_DECL, BAD_CAST "lt",
    NULL, NULL, NULL, NULL, NULL, NULL,
    BAD_CAST "<", BAD_CAST "<", 1,
    XML_INTERNAL_PREDEFINED_ENTITY,
    NULL, NULL, NULL, NULL, 0, 0, 0
};
static xmlEntity xmlEntityGt = {
    NULL, XML_ENTITY_DECL, BAD_CAST "gt",
    NULL, NULL, NULL, NULL, NULL, NULL,
    BAD_CAST ">", BAD_CAST ">", 1,
    XML_INTERNAL_PREDEFINED_ENTITY,
    NULL, NULL, NULL, NULL, 0, 0, 0
};
static xmlEntity xmlEntityAmp = {
    NULL, XML_ENTITY_DECL, BAD_CAST "amp",
    NULL, NULL, NULL, NULL, NULL, NULL,
    BAD_CAST "&", BAD_CAST "&", 1,
    XML_INTERNAL_PREDEFINED_ENTITY,
    NULL, NULL, NULL, NULL, 0, 0, 0
};
static xmlEntity xmlEntityQuot = {
    NULL, XML_ENTITY_DECL, BAD_CAST "quot",
    NULL, NULL, NULL, NULL, NULL, NULL,
    BAD_CAST "\"", BAD_CAST "\"", 1,
    XML_INTERNAL_PREDEFINED_ENTITY,
    NULL, NULL, NULL, NULL, 0, 0, 0
};
static xmlEntity xmlEntityApos = {
    NULL, XML_ENTITY_DECL, BAD_CAST "apos",
    NULL, NULL, NULL, NULL, NULL, NULL,
    BAD_CAST "'", BAD_CAST "'", 1,
    XML_INTERNAL_PREDEFINED_ENTITY,
    NULL, NULL, NULL, NULL, 0, 0, 0
};

/*
 * xmlFreeEntity:
 * @entity:  an entity
 *
 * Frees the entity.
 */
void
xmlFreeEntity(xmlEntityPtr entity)
{
    xmlDictPtr dict = NULL;

    if (entity == NULL)
        return;

    if (entity->doc != NULL)
        dict = entity->doc->dict;


    if ((entity->children) &&
        (entity == (xmlEntityPtr) entity->children->parent))
        xmlFreeNodeList(entity->children);
    if ((entity->name != NULL) &&
        ((dict == NULL) || (!xmlDictOwns(dict, entity->name))))
        xmlFree((char *) entity->name);
    if (entity->ExternalID != NULL)
        xmlFree((char *) entity->ExternalID);
    if (entity->SystemID != NULL)
        xmlFree((char *) entity->SystemID);
    if (entity->URI != NULL)
        xmlFree((char *) entity->URI);
    if (entity->content != NULL)
        xmlFree((char *) entity->content);
    if (entity->orig != NULL)
        xmlFree((char *) entity->orig);
    xmlFree(entity);
}

/*
 * xmlCreateEntity:
 *
 * internal routine doing the entity node structures allocations
 */
static xmlEntityPtr
xmlCreateEntity(xmlDocPtr doc, const xmlChar *name, int type,
	        const xmlChar *ExternalID, const xmlChar *SystemID,
	        const xmlChar *content) {
    xmlEntityPtr ret;

    ret = (xmlEntityPtr) xmlMalloc(sizeof(xmlEntity));
    if (ret == NULL)
	return(NULL);
    memset(ret, 0, sizeof(xmlEntity));
    ret->doc = doc;
    ret->type = XML_ENTITY_DECL;

    /*
     * fill the structure.
     */
    ret->etype = (xmlEntityType) type;
    if ((doc == NULL) || (doc->dict == NULL))
	ret->name = xmlStrdup(name);
    else
        ret->name = xmlDictLookup(doc->dict, name, -1);
    if (ret->name == NULL)
        goto error;
    if (ExternalID != NULL) {
        ret->ExternalID = xmlStrdup(ExternalID);
        if (ret->ExternalID == NULL)
            goto error;
    }
    if (SystemID != NULL) {
        ret->SystemID = xmlStrdup(SystemID);
        if (ret->SystemID == NULL)
            goto error;
    }
    if (content != NULL) {
        ret->length = xmlStrlen(content);
	ret->content = xmlStrndup(content, ret->length);
        if (ret->content == NULL)
            goto error;
     } else {
        ret->length = 0;
        ret->content = NULL;
