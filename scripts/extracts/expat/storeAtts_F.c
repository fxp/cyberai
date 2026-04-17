/* storeAtts F: id attribute tracking, final cleanup */
  /* clear flags for the remaining attributes */
  for (; i < attIndex; i += 2)
    ((XML_Char *)(appAtts[i]))[-1] = 0;
  for (binding = *bindingsPtr; binding; binding = binding->nextTagBinding)
    binding->attId->name[-1] = 0;

  if (! parser->m_ns)
    return XML_ERROR_NONE;

  /* expand the element type name */
  if (elementType->prefix) {
    binding = elementType->prefix->binding;
    if (! binding)
      return XML_ERROR_UNBOUND_PREFIX;
    localPart = tagNamePtr->str;
    while (*localPart++ != XML_T(ASCII_COLON))
      ;
  } else if (dtd->defaultPrefix.binding) {
    binding = dtd->defaultPrefix.binding;
    localPart = tagNamePtr->str;
  } else
    return XML_ERROR_NONE;
  prefixLen = 0;
  if (parser->m_ns_triplets && binding->prefix->name) {
    for (; binding->prefix->name[prefixLen++];)
      ; /* prefixLen includes null terminator */
  }
  tagNamePtr->localPart = localPart;
  tagNamePtr->uriLen = binding->uriLen;
  tagNamePtr->prefix = binding->prefix->name;
  tagNamePtr->prefixLen = prefixLen;
  for (i = 0; localPart[i++];)
    ; /* i includes null terminator */

  /* Detect and prevent integer overflow */
  if (binding->uriLen > INT_MAX - prefixLen
      || i > INT_MAX - (binding->uriLen + prefixLen)) {
    return XML_ERROR_NO_MEMORY;
  }

  n = i + binding->uriLen + prefixLen;
  if (n > binding->uriAlloc) {
    TAG *p;

    /* Detect and prevent integer overflow */
    if (n > INT_MAX - EXPAND_SPARE) {
      return XML_ERROR_NO_MEMORY;
    }
    /* Detect and prevent integer overflow.
     * The preprocessor guard addresses the "always false" warning
     * from -Wtype-limits on platforms where
     * sizeof(unsigned int) < sizeof(size_t), e.g. on x86_64. */
#if UINT_MAX >= SIZE_MAX
    if ((unsigned)(n + EXPAND_SPARE) > (size_t)(-1) / sizeof(XML_Char)) {
      return XML_ERROR_NO_MEMORY;
    }
#endif

    uri = (XML_Char *)MALLOC(parser, (n + EXPAND_SPARE) * sizeof(XML_Char));
    if (! uri)
      return XML_ERROR_NO_MEMORY;
    binding->uriAlloc = n + EXPAND_SPARE;
    memcpy(uri, binding->uri, binding->uriLen * sizeof(XML_Char));
    for (p = parser->m_tagStack; p; p = p->parent)
      if (p->name.str == binding->uri)
        p->name.str = uri;
    FREE(parser, binding->uri);
    binding->uri = uri;
  }
  /* if m_namespaceSeparator != '\0' then uri includes it already */
  uri = binding->uri + binding->uriLen;
  memcpy(uri, localPart, i * sizeof(XML_Char));
  /* we always have a namespace separator between localPart and prefix */
  if (prefixLen) {
    uri += i - 1;
    *uri = parser->m_namespaceSeparator; /* replace null terminator */
    memcpy(uri + 1, binding->prefix->name, prefixLen * sizeof(XML_Char));
  }
  tagNamePtr->str = binding->uri;
  return XML_ERROR_NONE;
}
