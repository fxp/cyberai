/* lookup: hash table probe + growth with createSize */
lookup(XML_Parser parser, HASH_TABLE *table, KEY name, size_t createSize) {
  size_t i;
  if (table->size == 0) {
    size_t tsize;
    if (! createSize)
      return NULL;
    table->power = INIT_POWER;
    /* table->size is a power of 2 */
    table->size = (size_t)1 << INIT_POWER;
    tsize = table->size * sizeof(NAMED *);
    table->v = table->mem->malloc_fcn(tsize);
    if (! table->v) {
      table->size = 0;
      return NULL;
    }
    memset(table->v, 0, tsize);
    i = hash(parser, name) & ((unsigned long)table->size - 1);
  } else {
    unsigned long h = hash(parser, name);
    unsigned long mask = (unsigned long)table->size - 1;
    unsigned char step = 0;
    i = h & mask;
    while (table->v[i]) {
      if (keyeq(name, table->v[i]->name))
        return table->v[i];
      if (! step)
        step = PROBE_STEP(h, mask, table->power);
      i < step ? (i += table->size - step) : (i -= step);
    }
    if (! createSize)
      return NULL;

    /* check for overflow (table is half full) */
    if (table->used >> (table->power - 1)) {
      unsigned char newPower = table->power + 1;

      /* Detect and prevent invalid shift */
      if (newPower >= sizeof(unsigned long) * 8 /* bits per byte */) {
        return NULL;
      }

      size_t newSize = (size_t)1 << newPower;
      unsigned long newMask = (unsigned long)newSize - 1;

      /* Detect and prevent integer overflow */
      if (newSize > (size_t)(-1) / sizeof(NAMED *)) {
        return NULL;
      }

      size_t tsize = newSize * sizeof(NAMED *);
      NAMED **newV = table->mem->malloc_fcn(tsize);
      if (! newV)
        return NULL;
      memset(newV, 0, tsize);
      for (i = 0; i < table->size; i++)
        if (table->v[i]) {
          unsigned long newHash = hash(parser, table->v[i]->name);
          size_t j = newHash & newMask;
          step = 0;
          while (newV[j]) {
            if (! step)
              step = PROBE_STEP(newHash, newMask, newPower);
            j < step ? (j += newSize - step) : (j -= step);
          }
          newV[j] = table->v[i];
        }
      table->mem->free_fcn(table->v);
      table->v = newV;
      table->power = newPower;
      table->size = newSize;
      i = h & newMask;
      step = 0;
      while (table->v[i]) {
        if (! step)
          step = PROBE_STEP(h, newMask, newPower);
        i < step ? (i += newSize - step) : (i -= step);
      }
    }
  }
  table->v[i] = table->mem->malloc_fcn(createSize);
  if (! table->v[i])
    return NULL;
  memset(table->v[i], 0, createSize);
  table->v[i]->name = name;
  (table->used)++;
  return table->v[i];
}
