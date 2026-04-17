/* build_model B: content model tree traversal/population */
   *         [3] first child of 1, does not have children
   *         [4] second child of 1, does not have children
   *         [5] third child of 1, does not have children
   *       [2] second child of 0, does not have children
   *
   *   Or (the same data) presented in flat array view:
   *
   *     [0] root, has two children
   *
   *     [1] first child of 0, has three children
   *     [2] second child of 0, does not have children
   *
   *     [3] first child of 1, does not have children
   *     [4] second child of 1, does not have children
   *     [5] third child of 1, does not have children
   *
   * - The algorithm repeats until all target array indices have been processed.
   */
  XML_Content *dest = ret; /* tree node writing location, moves upwards */
  XML_Content *const destLimit = &ret[dtd->scaffCount];
  XML_Content *jobDest = ret; /* next free writing location in target array */
  str = (XML_Char *)&ret[dtd->scaffCount];

  /* Add the starting job, the root node (index 0) of the source tree  */
  (jobDest++)->numchildren = 0;

  for (; dest < destLimit; dest++) {
    /* Retrieve source tree array index from job storage */
    const int src_node = (int)dest->numchildren;

    /* Convert item */
    dest->type = dtd->scaffold[src_node].type;
    dest->quant = dtd->scaffold[src_node].quant;
    if (dest->type == XML_CTYPE_NAME) {
      const XML_Char *src;
      dest->name = str;
      src = dtd->scaffold[src_node].name;
      for (;;) {
        *str++ = *src;
        if (! *src)
          break;
        src++;
      }
      dest->numchildren = 0;
      dest->children = NULL;
    } else {
      unsigned int i;
      int cn;
      dest->name = NULL;
      dest->numchildren = dtd->scaffold[src_node].childcnt;
      dest->children = jobDest;

      /* Append scaffold indices of children to array */
      for (i = 0, cn = dtd->scaffold[src_node].firstchild;
           i < dest->numchildren; i++, cn = dtd->scaffold[cn].nextsib)
        (jobDest++)->numchildren = (unsigned int)cn;
    }
  }

  return ret;
}
