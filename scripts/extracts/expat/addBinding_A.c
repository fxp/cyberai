/* addBinding A: namespace URI validation, binding malloc */
addBinding(XML_Parser parser, PREFIX *prefix, const ATTRIBUTE_ID *attId,
           const XML_Char *uri, BINDING **bindingsPtr) {
  // "http://www.w3.org/XML/1998/namespace"
  static const XML_Char xmlNamespace[]
      = {ASCII_h,      ASCII_t,     ASCII_t,     ASCII_p,      ASCII_COLON,
         ASCII_SLASH,  ASCII_SLASH, ASCII_w,     ASCII_w,      ASCII_w,
         ASCII_PERIOD, ASCII_w,     ASCII_3,     ASCII_PERIOD, ASCII_o,
         ASCII_r,      ASCII_g,     ASCII_SLASH, ASCII_X,      ASCII_M,
         ASCII_L,      ASCII_SLASH, ASCII_1,     ASCII_9,      ASCII_9,
         ASCII_8,      ASCII_SLASH, ASCII_n,     ASCII_a,      ASCII_m,
         ASCII_e,      ASCII_s,     ASCII_p,     ASCII_a,      ASCII_c,
         ASCII_e,      '\0'};
  static const int xmlLen = (int)sizeof(xmlNamespace) / sizeof(XML_Char) - 1;
  // "http://www.w3.org/2000/xmlns/"
  static const XML_Char xmlnsNamespace[]
      = {ASCII_h,     ASCII_t,      ASCII_t, ASCII_p, ASCII_COLON,  ASCII_SLASH,
         ASCII_SLASH, ASCII_w,      ASCII_w, ASCII_w, ASCII_PERIOD, ASCII_w,
         ASCII_3,     ASCII_PERIOD, ASCII_o, ASCII_r, ASCII_g,      ASCII_SLASH,
         ASCII_2,     ASCII_0,      ASCII_0, ASCII_0, ASCII_SLASH,  ASCII_x,
         ASCII_m,     ASCII_l,      ASCII_n, ASCII_s, ASCII_SLASH,  '\0'};
  static const int xmlnsLen
      = (int)sizeof(xmlnsNamespace) / sizeof(XML_Char) - 1;

  XML_Bool mustBeXML = XML_FALSE;
  XML_Bool isXML = XML_TRUE;
  XML_Bool isXMLNS = XML_TRUE;

  BINDING *b;
  int len;

  /* empty URI is only valid for default namespace per XML NS 1.0 (not 1.1) */
  if (*uri == XML_T('\0') && prefix->name)
    return XML_ERROR_UNDECLARING_PREFIX;

  if (prefix->name && prefix->name[0] == XML_T(ASCII_x)
      && prefix->name[1] == XML_T(ASCII_m)
      && prefix->name[2] == XML_T(ASCII_l)) {
    /* Not allowed to bind xmlns */
    if (prefix->name[3] == XML_T(ASCII_n) && prefix->name[4] == XML_T(ASCII_s)
        && prefix->name[5] == XML_T('\0'))
      return XML_ERROR_RESERVED_PREFIX_XMLNS;

    if (prefix->name[3] == XML_T('\0'))
      mustBeXML = XML_TRUE;
  }

  for (len = 0; uri[len]; len++) {
    if (isXML && (len > xmlLen || uri[len] != xmlNamespace[len]))
      isXML = XML_FALSE;

    if (! mustBeXML && isXMLNS
        && (len > xmlnsLen || uri[len] != xmlnsNamespace[len]))
      isXMLNS = XML_FALSE;

    // NOTE: While Expat does not validate namespace URIs against RFC 3986
    //       today (and is not REQUIRED to do so with regard to the XML 1.0
    //       namespaces specification) we have to at least make sure, that
    //       the application on top of Expat (that is likely splitting expanded
    //       element names ("qualified names") of form
    //       "[uri sep] local [sep prefix] '\0'" back into 1, 2 or 3 pieces
    //       in its element handler code) cannot be confused by an attacker
    //       putting additional namespace separator characters into namespace
    //       declarations.  That would be ambiguous and not to be expected.
    //
    //       While the HTML API docs of function XML_ParserCreateNS have been
    //       advising against use of a namespace separator character that can
    //       appear in a URI for >20 years now, some widespread applications
    //       are using URI characters (':' (colon) in particular) for a
    //       namespace separator, in practice.  To keep these applications
    //       functional, we only reject namespaces URIs containing the
    //       application-chosen namespace separator if the chosen separator
    //       is a non-URI character with regard to RFC 3986.
    if (parser->m_ns && (uri[len] == parser->m_namespaceSeparator)
        && ! is_rfc3986_uri_char(uri[len])) {
      return XML_ERROR_SYNTAX;
    }
  }
  isXML = isXML && len == xmlLen;
  isXMLNS = isXMLNS && len == xmlnsLen;

  if (mustBeXML != isXML)
