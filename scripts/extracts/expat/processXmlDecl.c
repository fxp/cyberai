/* processXmlDecl: XML version/encoding/standalone attrs */
processXmlDecl(XML_Parser parser, int isGeneralTextEntity, const char *s,
               const char *next) {
  const char *encodingName = NULL;
  const XML_Char *storedEncName = NULL;
  const ENCODING *newEncoding = NULL;
  const char *version = NULL;
  const char *versionend = NULL;
  const XML_Char *storedversion = NULL;
  int standalone = -1;

#if XML_GE == 1
  if (! accountingDiffTolerated(parser, XML_TOK_XML_DECL, s, next, __LINE__,
                                XML_ACCOUNT_DIRECT)) {
    accountingOnAbort(parser);
    return XML_ERROR_AMPLIFICATION_LIMIT_BREACH;
  }
#endif

  if (! (parser->m_ns ? XmlParseXmlDeclNS : XmlParseXmlDecl)(
          isGeneralTextEntity, parser->m_encoding, s, next, &parser->m_eventPtr,
          &version, &versionend, &encodingName, &newEncoding, &standalone)) {
    if (isGeneralTextEntity)
      return XML_ERROR_TEXT_DECL;
    else
      return XML_ERROR_XML_DECL;
  }
  if (! isGeneralTextEntity && standalone == 1) {
    parser->m_dtd->standalone = XML_TRUE;
#ifdef XML_DTD
    if (parser->m_paramEntityParsing
        == XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE)
      parser->m_paramEntityParsing = XML_PARAM_ENTITY_PARSING_NEVER;
#endif /* XML_DTD */
  }
  if (parser->m_xmlDeclHandler) {
    if (encodingName != NULL) {
      storedEncName = poolStoreString(
          &parser->m_temp2Pool, parser->m_encoding, encodingName,
          encodingName + XmlNameLength(parser->m_encoding, encodingName));
      if (! storedEncName)
        return XML_ERROR_NO_MEMORY;
      poolFinish(&parser->m_temp2Pool);
    }
    if (version) {
      storedversion
          = poolStoreString(&parser->m_temp2Pool, parser->m_encoding, version,
                            versionend - parser->m_encoding->minBytesPerChar);
      if (! storedversion)
        return XML_ERROR_NO_MEMORY;
    }
    parser->m_xmlDeclHandler(parser->m_handlerArg, storedversion, storedEncName,
                             standalone);
  } else if (parser->m_defaultHandler)
    reportDefault(parser, parser->m_encoding, s, next);
  if (parser->m_protocolEncodingName == NULL) {
    if (newEncoding) {
      /* Check that the specified encoding does not conflict with what
       * the parser has already deduced.  Do we have the same number
       * of bytes in the smallest representation of a character?  If
       * this is UTF-16, is it the same endianness?
       */
      if (newEncoding->minBytesPerChar != parser->m_encoding->minBytesPerChar
          || (newEncoding->minBytesPerChar == 2
              && newEncoding != parser->m_encoding)) {
        parser->m_eventPtr = encodingName;
        return XML_ERROR_INCORRECT_ENCODING;
      }
      parser->m_encoding = newEncoding;
    } else if (encodingName) {
      enum XML_Error result;
      if (! storedEncName) {
        storedEncName = poolStoreString(
            &parser->m_temp2Pool, parser->m_encoding, encodingName,
            encodingName + XmlNameLength(parser->m_encoding, encodingName));
        if (! storedEncName)
          return XML_ERROR_NO_MEMORY;
      }
      result = handleUnknownEncoding(parser, storedEncName);
      poolClear(&parser->m_temp2Pool);
      if (result == XML_ERROR_UNKNOWN_ENCODING)
        parser->m_eventPtr = encodingName;
      return result;
    }
  }

  if (storedEncName || storedversion)
    poolClear(&parser->m_temp2Pool);

  return XML_ERROR_NONE;
}
