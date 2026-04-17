/* build_model A: XML_Content node allocation */
build_model(XML_Parser parser) {
  /* Function build_model transforms the existing parser->m_dtd->scaffold
   * array of CONTENT_SCAFFOLD tree nodes into a new array of
   * XML_Content tree nodes followed by a gapless list of zero-terminated
   * strings. */
  DTD *const dtd = parser->m_dtd; /* save one level of indirection */
  XML_Content *ret;
  XML_Char *str; /* the current string writing location */

  /* Detect and prevent integer overflow.
   * The preprocessor guard addresses the "always false" warning
   * from -Wtype-limits on platforms where
   * sizeof(unsigned int) < sizeof(size_t), e.g. on x86_64. */
#if UINT_MAX >= SIZE_MAX
  if (dtd->scaffCount > (size_t)(-1) / sizeof(XML_Content)) {
    return NULL;
  }
  if (dtd->contentStringLen > (size_t)(-1) / sizeof(XML_Char)) {
    return NULL;
  }
#endif
  if (dtd->scaffCount * sizeof(XML_Content)
      > (size_t)(-1) - dtd->contentStringLen * sizeof(XML_Char)) {
    return NULL;
  }

  const size_t allocsize = (dtd->scaffCount * sizeof(XML_Content)
                            + (dtd->contentStringLen * sizeof(XML_Char)));

  ret = (XML_Content *)MALLOC(parser, allocsize);
  if (! ret)
    return NULL;

  /* What follows is an iterative implementation (of what was previously done
   * recursively in a dedicated function called "build_node".  The old recursive
   * build_node could be forced into stack exhaustion from input as small as a
   * few megabyte, and so that was a security issue.  Hence, a function call
   * stack is avoided now by resolving recursion.)
   *
   * The iterative approach works as follows:
   *
   * - We have two writing pointers, both walking up the result array; one does
   *   the work, the other creates "jobs" for its colleague to do, and leads
   *   the way:
   *
   *   - The faster one, pointer jobDest, always leads and writes "what job
   *     to do" by the other, once they reach that place in the
   *     array: leader "jobDest" stores the source node array index (relative
   *     to array dtd->scaffold) in field "numchildren".
   *
   *   - The slower one, pointer dest, looks at the value stored in the
   *     "numchildren" field (which actually holds a source node array index
   *     at that time) and puts the real data from dtd->scaffold in.
   *
   * - Before the loop starts, jobDest writes source array index 0
   *   (where the root node is located) so that dest will have something to do
   *   when it starts operation.
   *
   * - Whenever nodes with children are encountered, jobDest appends
   *   them as new jobs, in order.  As a result, tree node siblings are
   *   adjacent in the resulting array, for example:
   *
   *     [0] root, has two children
   *       [1] first child of 0, has three children
