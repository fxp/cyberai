/* appendAttributeValue B: entity ref, char ref expansion */
      entity = (ENTITY *)lookup(parser, &dtd->generalEntities, name, 0);
      poolDiscard(&parser->m_temp2Pool);
      /* First, determine if a check for an existing declaration is needed;
         if yes, check that the entity exists, and that it is internal.
      */
      if (pool == &dtd->pool) /* are we called from prolog? */
        checkEntityDecl =
#ifdef XML_DTD
            parser->m_prologState.documentEntity &&
#endif /* XML_DTD */
            (dtd->standalone ? ! parser->m_openInternalEntities
                             : ! dtd->hasParamEntityRefs);
      else /* if (pool == &parser->m_tempPool): we are called from content */
        checkEntityDecl = ! dtd->hasParamEntityRefs || dtd->standalone;
      if (checkEntityDecl) {
        if (! entity)
          return XML_ERROR_UNDEFINED_ENTITY;
        else if (! entity->is_internal)
          return XML_ERROR_ENTITY_DECLARED_IN_PE;
      } else if (! entity) {
        /* Cannot report skipped entity here - see comments on
           parser->m_skippedEntityHandler.
        if (parser->m_skippedEntityHandler)
          parser->m_skippedEntityHandler(parser->m_handlerArg, name, 0);
        */
        /* Cannot call the default handler because this would be
           out of sync with the call to the startElementHandler.
        if ((pool == &parser->m_tempPool) && parser->m_defaultHandler)
          reportDefault(parser, enc, ptr, next);
        */
        break;
      }
      if (entity->open) {
        if (enc == parser->m_encoding) {
          /* It does not appear that this line can be executed.
           *
           * The "if (entity->open)" check catches recursive entity
           * definitions.  In order to be called with an open
           * entity, it must have gone through this code before and
           * been through the recursive call to
           * appendAttributeValue() some lines below.  That call
           * sets the local encoding ("enc") to the parser's
           * internal encoding (internal_utf8 or internal_utf16),
           * which can never be the same as the principle encoding.
           * It doesn't appear there is another code path that gets
           * here with entity->open being TRUE.
           *
           * Since it is not certain that this logic is watertight,
           * we keep the line and merely exclude it from coverage
           * tests.
           */
          parser->m_eventPtr = ptr; /* LCOV_EXCL_LINE */
        }
        return XML_ERROR_RECURSIVE_ENTITY_REF;
      }
      if (entity->notation) {
        if (enc == parser->m_encoding)
          parser->m_eventPtr = ptr;
        return XML_ERROR_BINARY_ENTITY_REF;
      }
