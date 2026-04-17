/* defineAttribute: uniqueness enforcement, type storage */
defineAttribute(ELEMENT_TYPE *type, ATTRIBUTE_ID *attId, XML_Bool isCdata,
                XML_Bool isId, const XML_Char *value, XML_Parser parser) {
  DEFAULT_ATTRIBUTE *att;
  if (value || isId) {
    /* The handling of default attributes gets messed up if we have
       a default which duplicates a non-default. */
    int i;
    for (i = 0; i < type->nDefaultAtts; i++)
      if (attId == type->defaultAtts[i].id)
        return 1;
    if (isId && ! type->idAtt && ! attId->xmlns)
      type->idAtt = attId;
  }
  if (type->nDefaultAtts == type->allocDefaultAtts) {
    if (type->allocDefaultAtts == 0) {
      type->allocDefaultAtts = 8;
      type->defaultAtts = (DEFAULT_ATTRIBUTE *)MALLOC(
          parser, type->allocDefaultAtts * sizeof(DEFAULT_ATTRIBUTE));
      if (! type->defaultAtts) {
        type->allocDefaultAtts = 0;
        return 0;
      }
    } else {
      DEFAULT_ATTRIBUTE *temp;

      /* Detect and prevent integer overflow */
      if (type->allocDefaultAtts > INT_MAX / 2) {
        return 0;
      }

      int count = type->allocDefaultAtts * 2;

      /* Detect and prevent integer overflow.
       * The preprocessor guard addresses the "always false" warning
       * from -Wtype-limits on platforms where
       * sizeof(unsigned int) < sizeof(size_t), e.g. on x86_64. */
#if UINT_MAX >= SIZE_MAX
      if ((unsigned)count > (size_t)(-1) / sizeof(DEFAULT_ATTRIBUTE)) {
        return 0;
      }
#endif

      temp = (DEFAULT_ATTRIBUTE *)REALLOC(parser, type->defaultAtts,
                                          (count * sizeof(DEFAULT_ATTRIBUTE)));
      if (temp == NULL)
        return 0;
      type->allocDefaultAtts = count;
      type->defaultAtts = temp;
    }
  }
  att = type->defaultAtts + type->nDefaultAtts;
  att->id = attId;
  att->value = value;
  att->isCdata = isCdata;
  if (! isCdata)
    attId->maybeTokenized = XML_TRUE;
  type->nDefaultAtts += 1;
  return 1;
}
