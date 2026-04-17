/* doContent D: CDATA section, entity reference dispatch */
      enum XML_Error result;
      BINDING *bindings = NULL;
      XML_Bool noElmHandlers = XML_TRUE;
      TAG_NAME name;
      name.str = poolStoreString(&parser->m_tempPool, enc, rawName,
                                 rawName + XmlNameLength(enc, rawName));
      if (! name.str)
        return XML_ERROR_NO_MEMORY;
      poolFinish(&parser->m_tempPool);
      result = storeAtts(parser, enc, s, &name, &bindings,
                         XML_ACCOUNT_NONE /* token spans whole start tag */);
      if (result != XML_ERROR_NONE) {
        freeBindings(parser, bindings);
        return result;
      }
      poolFinish(&parser->m_tempPool);
      if (parser->m_startElementHandler) {
        parser->m_startElementHandler(parser->m_handlerArg, name.str,
                                      (const XML_Char **)parser->m_atts);
        noElmHandlers = XML_FALSE;
      }
      if (parser->m_endElementHandler) {
        if (parser->m_startElementHandler)
          *eventPP = *eventEndPP;
        parser->m_endElementHandler(parser->m_handlerArg, name.str);
        noElmHandlers = XML_FALSE;
      }
      if (noElmHandlers && parser->m_defaultHandler)
        reportDefault(parser, enc, s, next);
      poolClear(&parser->m_tempPool);
      freeBindings(parser, bindings);
    }
      if ((parser->m_tagLevel == 0)
          && (parser->m_parsingStatus.parsing != XML_FINISHED)) {
        if (parser->m_parsingStatus.parsing == XML_SUSPENDED)
          parser->m_processor = epilogProcessor;
        else
          return epilogProcessor(parser, next, end, nextPtr);
      }
      break;
    case XML_TOK_END_TAG:
      if (parser->m_tagLevel == startTagLevel)
        return XML_ERROR_ASYNC_ENTITY;
      else {
        int len;
        const char *rawName;
        TAG *tag = parser->m_tagStack;
        rawName = s + enc->minBytesPerChar * 2;
        len = XmlNameLength(enc, rawName);
        if (len != tag->rawNameLength
            || memcmp(tag->rawName, rawName, len) != 0) {
          *eventPP = rawName;
          return XML_ERROR_TAG_MISMATCH;
        }
        parser->m_tagStack = tag->parent;
        tag->parent = parser->m_freeTagList;
        parser->m_freeTagList = tag;
        --parser->m_tagLevel;
        if (parser->m_endElementHandler) {
          const XML_Char *localPart;
          const XML_Char *prefix;
          XML_Char *uri;
          localPart = tag->name.localPart;
          if (parser->m_ns && localPart) {
            /* localPart and prefix may have been overwritten in
               tag->name.str, since this points to the binding->uri
               buffer which gets reused; so we have to add them again
            */
            uri = (XML_Char *)tag->name.str + tag->name.uriLen;
            /* don't need to check for space - already done in storeAtts() */
            while (*localPart)
              *uri++ = *localPart++;
            prefix = tag->name.prefix;
            if (parser->m_ns_triplets && prefix) {
              *uri++ = parser->m_namespaceSeparator;
              while (*prefix)
                *uri++ = *prefix++;
            }
            *uri = XML_T('\0');
          }
          parser->m_endElementHandler(parser->m_handlerArg, tag->name.str);
        } else if (parser->m_defaultHandler)
          reportDefault(parser, enc, s, next);
        while (tag->bindings) {
          BINDING *b = tag->bindings;
          if (parser->m_endNamespaceDeclHandler)
            parser->m_endNamespaceDeclHandler(parser->m_handlerArg,
                                              b->prefix->name);
          tag->bindings = tag->bindings->nextTagBinding;
          b->nextTagBinding = parser->m_freeBindingList;
