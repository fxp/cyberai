/* doContent B: end tag handling, level tracking */
    case XML_TOK_PARTIAL:
      if (haveMore) {
        *nextPtr = s;
        return XML_ERROR_NONE;
      }
      return XML_ERROR_UNCLOSED_TOKEN;
    case XML_TOK_PARTIAL_CHAR:
      if (haveMore) {
        *nextPtr = s;
        return XML_ERROR_NONE;
      }
      return XML_ERROR_PARTIAL_CHAR;
    case XML_TOK_ENTITY_REF: {
      const XML_Char *name;
      ENTITY *entity;
      XML_Char ch = (XML_Char)XmlPredefinedEntityName(
          enc, s + enc->minBytesPerChar, next - enc->minBytesPerChar);
      if (ch) {
#if XML_GE == 1
        /* NOTE: We are replacing 4-6 characters original input for 1 character
         *       so there is no amplification and hence recording without
         *       protection. */
        accountingDiffTolerated(parser, tok, (char *)&ch,
                                ((char *)&ch) + sizeof(XML_Char), __LINE__,
                                XML_ACCOUNT_ENTITY_EXPANSION);
#endif /* XML_GE == 1 */
        if (parser->m_characterDataHandler)
          parser->m_characterDataHandler(parser->m_handlerArg, &ch, 1);
        else if (parser->m_defaultHandler)
          reportDefault(parser, enc, s, next);
        break;
      }
      name = poolStoreString(&dtd->pool, enc, s + enc->minBytesPerChar,
                             next - enc->minBytesPerChar);
      if (! name)
        return XML_ERROR_NO_MEMORY;
      entity = (ENTITY *)lookup(parser, &dtd->generalEntities, name, 0);
      poolDiscard(&dtd->pool);
      /* First, determine if a check for an existing declaration is needed;
         if yes, check that the entity exists, and that it is internal,
         otherwise call the skipped entity or default handler.
      */
      if (! dtd->hasParamEntityRefs || dtd->standalone) {
        if (! entity)
          return XML_ERROR_UNDEFINED_ENTITY;
        else if (! entity->is_internal)
          return XML_ERROR_ENTITY_DECLARED_IN_PE;
      } else if (! entity) {
        if (parser->m_skippedEntityHandler)
          parser->m_skippedEntityHandler(parser->m_handlerArg, name, 0);
        else if (parser->m_defaultHandler)
          reportDefault(parser, enc, s, next);
        break;
      }
      if (entity->open)
        return XML_ERROR_RECURSIVE_ENTITY_REF;
      if (entity->notation)
        return XML_ERROR_BINARY_ENTITY_REF;
      if (entity->textPtr) {
        enum XML_Error result;
        if (! parser->m_defaultExpandInternalEntities) {
          if (parser->m_skippedEntityHandler)
            parser->m_skippedEntityHandler(parser->m_handlerArg, entity->name,
                                           0);
          else if (parser->m_defaultHandler)
            reportDefault(parser, enc, s, next);
          break;
        }
        result = processInternalEntity(parser, entity, XML_FALSE);
        if (result != XML_ERROR_NONE)
