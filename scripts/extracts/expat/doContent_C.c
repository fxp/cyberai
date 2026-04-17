/* doContent C: char data, processing instruction, comment */
          return result;
      } else if (parser->m_externalEntityRefHandler) {
        const XML_Char *context;
        entity->open = XML_TRUE;
        context = getContext(parser);
        entity->open = XML_FALSE;
        if (! context)
          return XML_ERROR_NO_MEMORY;
        if (! parser->m_externalEntityRefHandler(
                parser->m_externalEntityRefHandlerArg, context, entity->base,
                entity->systemId, entity->publicId))
          return XML_ERROR_EXTERNAL_ENTITY_HANDLING;
        poolDiscard(&parser->m_tempPool);
      } else if (parser->m_defaultHandler)
        reportDefault(parser, enc, s, next);
      break;
    }
    case XML_TOK_START_TAG_NO_ATTS:
      /* fall through */
    case XML_TOK_START_TAG_WITH_ATTS: {
      TAG *tag;
      enum XML_Error result;
      XML_Char *toPtr;
      if (parser->m_freeTagList) {
        tag = parser->m_freeTagList;
        parser->m_freeTagList = parser->m_freeTagList->parent;
      } else {
        tag = (TAG *)MALLOC(parser, sizeof(TAG));
        if (! tag)
          return XML_ERROR_NO_MEMORY;
        tag->buf = (char *)MALLOC(parser, INIT_TAG_BUF_SIZE);
        if (! tag->buf) {
          FREE(parser, tag);
          return XML_ERROR_NO_MEMORY;
        }
        tag->bufEnd = tag->buf + INIT_TAG_BUF_SIZE;
      }
      tag->bindings = NULL;
      tag->parent = parser->m_tagStack;
      parser->m_tagStack = tag;
      tag->name.localPart = NULL;
      tag->name.prefix = NULL;
      tag->rawName = s + enc->minBytesPerChar;
      tag->rawNameLength = XmlNameLength(enc, tag->rawName);
      ++parser->m_tagLevel;
      {
        const char *rawNameEnd = tag->rawName + tag->rawNameLength;
        const char *fromPtr = tag->rawName;
        toPtr = (XML_Char *)tag->buf;
        for (;;) {
          int bufSize;
          int convLen;
          const enum XML_Convert_Result convert_res
              = XmlConvert(enc, &fromPtr, rawNameEnd, (ICHAR **)&toPtr,
                           (ICHAR *)tag->bufEnd - 1);
          convLen = (int)(toPtr - (XML_Char *)tag->buf);
          if ((fromPtr >= rawNameEnd)
              || (convert_res == XML_CONVERT_INPUT_INCOMPLETE)) {
            tag->name.strLen = convLen;
            break;
          }
          bufSize = (int)(tag->bufEnd - tag->buf) << 1;
          {
            char *temp = (char *)REALLOC(parser, tag->buf, bufSize);
            if (temp == NULL)
              return XML_ERROR_NO_MEMORY;
            tag->buf = temp;
            tag->bufEnd = temp + bufSize;
            toPtr = (XML_Char *)temp + convLen;
          }
        }
      }
      tag->name.str = (XML_Char *)tag->buf;
      *toPtr = XML_T('\0');
      result
          = storeAtts(parser, enc, s, &(tag->name), &(tag->bindings), account);
      if (result)
        return result;
      if (parser->m_startElementHandler)
        parser->m_startElementHandler(parser->m_handlerArg, tag->name.str,
                                      (const XML_Char **)parser->m_atts);
      else if (parser->m_defaultHandler)
        reportDefault(parser, enc, s, next);
      poolClear(&parser->m_tempPool);
      break;
    }
    case XML_TOK_EMPTY_ELEMENT_NO_ATTS:
      /* fall through */
    case XML_TOK_EMPTY_ELEMENT_WITH_ATTS: {
      const char *rawName = s + enc->minBytesPerChar;
