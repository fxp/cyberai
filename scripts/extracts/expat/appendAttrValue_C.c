/* appendAttributeValue C: pool append completion */
      if (! entity->textPtr) {
        if (enc == parser->m_encoding)
          parser->m_eventPtr = ptr;
        return XML_ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF;
      } else {
        enum XML_Error result;
        const XML_Char *textEnd = entity->textPtr + entity->textLen;
        entity->open = XML_TRUE;
#if XML_GE == 1
        entityTrackingOnOpen(parser, entity, __LINE__);
#endif
        result = appendAttributeValue(parser, parser->m_internalEncoding,
                                      isCdata, (const char *)entity->textPtr,
                                      (const char *)textEnd, pool,
                                      XML_ACCOUNT_ENTITY_EXPANSION);
#if XML_GE == 1
        entityTrackingOnClose(parser, entity, __LINE__);
#endif
        entity->open = XML_FALSE;
        if (result)
          return result;
      }
    } break;
    default:
      /* The only token returned by XmlAttributeValueTok() that does
       * not have an explicit case here is XML_TOK_PARTIAL_CHAR.
       * Getting that would require an entity name to contain an
       * incomplete XML character (e.g. \xE2\x82); however previous
       * tokenisers will have already recognised and rejected such
       * names before XmlAttributeValueTok() gets a look-in.  This
       * default case should be retained as a safety net, but the code
       * excluded from coverage tests.
       *
       * LCOV_EXCL_START
       */
      if (enc == parser->m_encoding)
        parser->m_eventPtr = ptr;
      return XML_ERROR_UNEXPECTED_STATE;
      /* LCOV_EXCL_STOP */
    }
    ptr = next;
  }
  /* not reached */
}
