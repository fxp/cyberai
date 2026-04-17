/* storeEntityValue A: pool grow loop, char data copy */
storeEntityValue(XML_Parser parser, const ENCODING *enc,
                 const char *entityTextPtr, const char *entityTextEnd,
                 enum XML_Account account) {
  DTD *const dtd = parser->m_dtd; /* save one level of indirection */
  STRING_POOL *pool = &(dtd->entityValuePool);
  enum XML_Error result = XML_ERROR_NONE;
#  ifdef XML_DTD
  int oldInEntityValue = parser->m_prologState.inEntityValue;
  parser->m_prologState.inEntityValue = 1;
#  else
  UNUSED_P(account);
#  endif /* XML_DTD */
  /* never return Null for the value argument in EntityDeclHandler,
     since this would indicate an external entity; therefore we
     have to make sure that entityValuePool.start is not null */
  if (! pool->blocks) {
    if (! poolGrow(pool))
      return XML_ERROR_NO_MEMORY;
  }

  for (;;) {
    const char *next
        = entityTextPtr; /* XmlEntityValueTok doesn't always set the last arg */
    int tok = XmlEntityValueTok(enc, entityTextPtr, entityTextEnd, &next);

    if (! accountingDiffTolerated(parser, tok, entityTextPtr, next, __LINE__,
                                  account)) {
      accountingOnAbort(parser);
      result = XML_ERROR_AMPLIFICATION_LIMIT_BREACH;
      goto endEntityValue;
    }

    switch (tok) {
    case XML_TOK_PARAM_ENTITY_REF:
#  ifdef XML_DTD
      if (parser->m_isParamEntity || enc != parser->m_encoding) {
        const XML_Char *name;
        ENTITY *entity;
        name = poolStoreString(&parser->m_tempPool, enc,
                               entityTextPtr + enc->minBytesPerChar,
                               next - enc->minBytesPerChar);
        if (! name) {
          result = XML_ERROR_NO_MEMORY;
          goto endEntityValue;
        }
        entity = (ENTITY *)lookup(parser, &dtd->paramEntities, name, 0);
        poolDiscard(&parser->m_tempPool);
        if (! entity) {
          /* not a well-formedness error - see XML 1.0: WFC Entity Declared */
          /* cannot report skipped entity here - see comments on
             parser->m_skippedEntityHandler
          if (parser->m_skippedEntityHandler)
            parser->m_skippedEntityHandler(parser->m_handlerArg, name, 0);
          */
          dtd->keepProcessing = dtd->standalone;
          goto endEntityValue;
        }
        if (entity->open || (entity == parser->m_declEntity)) {
          if (enc == parser->m_encoding)
            parser->m_eventPtr = entityTextPtr;
          result = XML_ERROR_RECURSIVE_ENTITY_REF;
          goto endEntityValue;
        }
        if (entity->systemId) {
          if (parser->m_externalEntityRefHandler) {
            dtd->paramEntityRead = XML_FALSE;
            entity->open = XML_TRUE;
            entityTrackingOnOpen(parser, entity, __LINE__);
            if (! parser->m_externalEntityRefHandler(
                    parser->m_externalEntityRefHandlerArg, 0, entity->base,
                    entity->systemId, entity->publicId)) {
              entityTrackingOnClose(parser, entity, __LINE__);
              entity->open = XML_FALSE;
              result = XML_ERROR_EXTERNAL_ENTITY_HANDLING;
              goto endEntityValue;
            }
            entityTrackingOnClose(parser, entity, __LINE__);
            entity->open = XML_FALSE;
            if (! dtd->paramEntityRead)
              dtd->keepProcessing = dtd->standalone;
          } else
            dtd->keepProcessing = dtd->standalone;
        } else {
          entity->open = XML_TRUE;
          entityTrackingOnOpen(parser, entity, __LINE__);
          result = storeEntityValue(
              parser, parser->m_internalEncoding, (const char *)entity->textPtr,
              (const char *)(entity->textPtr + entity->textLen),
              XML_ACCOUNT_ENTITY_EXPANSION);
          entityTrackingOnClose(parser, entity, __LINE__);
          entity->open = XML_FALSE;
