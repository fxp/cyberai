/* appendAttributeValue A: tokenizer loop, direct copy */
appendAttributeValue(XML_Parser parser, const ENCODING *enc, XML_Bool isCdata,
                     const char *ptr, const char *end, STRING_POOL *pool,
                     enum XML_Account account) {
  DTD *const dtd = parser->m_dtd; /* save one level of indirection */
#ifndef XML_DTD
  UNUSED_P(account);
#endif

  for (;;) {
    const char *next
        = ptr; /* XmlAttributeValueTok doesn't always set the last arg */
    int tok = XmlAttributeValueTok(enc, ptr, end, &next);
#if XML_GE == 1
    if (! accountingDiffTolerated(parser, tok, ptr, next, __LINE__, account)) {
      accountingOnAbort(parser);
      return XML_ERROR_AMPLIFICATION_LIMIT_BREACH;
    }
#endif
    switch (tok) {
    case XML_TOK_NONE:
      return XML_ERROR_NONE;
    case XML_TOK_INVALID:
      if (enc == parser->m_encoding)
        parser->m_eventPtr = next;
      return XML_ERROR_INVALID_TOKEN;
    case XML_TOK_PARTIAL:
      if (enc == parser->m_encoding)
        parser->m_eventPtr = ptr;
      return XML_ERROR_INVALID_TOKEN;
    case XML_TOK_CHAR_REF: {
      XML_Char buf[XML_ENCODE_MAX];
      int i;
      int n = XmlCharRefNumber(enc, ptr);
      if (n < 0) {
        if (enc == parser->m_encoding)
          parser->m_eventPtr = ptr;
        return XML_ERROR_BAD_CHAR_REF;
      }
      if (! isCdata && n == 0x20 /* space */
          && (poolLength(pool) == 0 || poolLastChar(pool) == 0x20))
        break;
      n = XmlEncode(n, (ICHAR *)buf);
      /* The XmlEncode() functions can never return 0 here.  That
       * error return happens if the code point passed in is either
       * negative or greater than or equal to 0x110000.  The
       * XmlCharRefNumber() functions will all return a number
       * strictly less than 0x110000 or a negative value if an error
       * occurred.  The negative value is intercepted above, so
       * XmlEncode() is never passed a value it might return an
       * error for.
       */
      for (i = 0; i < n; i++) {
        if (! poolAppendChar(pool, buf[i]))
          return XML_ERROR_NO_MEMORY;
      }
    } break;
    case XML_TOK_DATA_CHARS:
      if (! poolAppend(pool, enc, ptr, next))
        return XML_ERROR_NO_MEMORY;
      break;
    case XML_TOK_TRAILING_CR:
      next = ptr + enc->minBytesPerChar;
      /* fall through */
    case XML_TOK_ATTRIBUTE_VALUE_S:
    case XML_TOK_DATA_NEWLINE:
      if (! isCdata && (poolLength(pool) == 0 || poolLastChar(pool) == 0x20))
        break;
      if (! poolAppendChar(pool, 0x20))
        return XML_ERROR_NO_MEMORY;
      break;
    case XML_TOK_ENTITY_REF: {
      const XML_Char *name;
      ENTITY *entity;
      char checkEntityDecl;
      XML_Char ch = (XML_Char)XmlPredefinedEntityName(
          enc, ptr + enc->minBytesPerChar, next - enc->minBytesPerChar);
      if (ch) {
#if XML_GE == 1
        /* NOTE: We are replacing 4-6 characters original input for 1 character
         *       so there is no amplification and hence recording without
         *       protection. */
        accountingDiffTolerated(parser, tok, (char *)&ch,
                                ((char *)&ch) + sizeof(XML_Char), __LINE__,
                                XML_ACCOUNT_ENTITY_EXPANSION);
#endif /* XML_GE == 1 */
        if (! poolAppendChar(pool, ch))
          return XML_ERROR_NO_MEMORY;
        break;
      }
      name = poolStoreString(&parser->m_temp2Pool, enc,
                             ptr + enc->minBytesPerChar,
                             next - enc->minBytesPerChar);
      if (! name)
        return XML_ERROR_NO_MEMORY;
