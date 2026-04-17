  {
    subFont->blend_top  = subFont->blend_stack;
    subFont->blend_used = 0;
  }


  /* Blend numOperands on the stack,                       */
  /* store results into the first numBlends values,        */
  /* then pop remaining arguments.                         */
  /*                                                       */
  /* This is comparable to `cf2_doBlend' but               */
  /* the cffparse stack is different and can't be written. */
  /* Blended values are written to a different buffer,     */
  /* using reserved operator 255.                          */
  /*                                                       */
  /* Blend calculation is done in 16.16 fixed-point.       */
  FT_LOCAL_DEF( FT_Error )
  cff_blend_doBlend( CFF_SubFont  subFont,
                     CFF_Parser   parser,
                     FT_UInt      numBlends )
  {
    FT_UInt  delta;
    FT_UInt  base;
    FT_UInt  i, j;
    FT_UInt  size;

    CFF_Blend  blend = &subFont->blend;

    FT_Memory  memory = subFont->blend.font->memory; /* for FT_REALLOC */
    FT_Error   error  = FT_Err_Ok;                   /* for FT_REALLOC */

    /* compute expected number of operands for this blend */
    FT_UInt  numOperands = (FT_UInt)( numBlends * blend->lenBV );
    FT_UInt  count       = (FT_UInt)( parser->top - 1 - parser->stack );


    if ( numOperands > count )
    {
      FT_TRACE4(( " cff_blend_doBlend: Stack underflow %d argument%s\n",
                  count,
                  count == 1 ? "" : "s" ));

      error = FT_THROW( Stack_Underflow );
      goto Exit;
    }

    /* check whether we have room for `numBlends' values at `blend_top' */
    size = 5 * numBlends;           /* add 5 bytes per entry    */
    if ( subFont->blend_used + size > subFont->blend_alloc )
    {
      FT_Byte*  blend_stack_old = subFont->blend_stack;
      FT_Byte*  blend_top_old   = subFont->blend_top;


      /* increase or allocate `blend_stack' and reset `blend_top'; */
      /* prepare to append `numBlends' values to the buffer        */
      if ( FT_QREALLOC( subFont->blend_stack,
                        subFont->blend_alloc,
                        subFont->blend_alloc + size ) )
        goto Exit;

      subFont->blend_top    = subFont->blend_stack + subFont->blend_used;
      subFont->blend_alloc += size;

      /* iterate over the parser stack and adjust pointers */
      /* if the reallocated buffer has a different address */
      if ( blend_stack_old                         &&
           subFont->blend_stack != blend_stack_old )
      {
        FT_PtrDist  offset = subFont->blend_stack - blend_stack_old;
        FT_Byte**   p;


        for ( p = parser->stack; p < parser->top; p++ )
        {
          if ( *p >= blend_stack_old && *p < blend_top_old )
            *p += offset;
        }
      }
    }
    subFont->blend_used += size;

    base  = count - numOperands;     /* index of first blend arg */
    delta = base + numBlends;        /* index of first delta arg */

    for ( i = 0; i < numBlends; i++ )
    {
      const FT_Int32*  weight = &blend->BV[1];
      FT_Fixed         sum;


      /* convert inputs to 16.16 fixed point */
      sum = cff_parse_fixed( parser, &parser->stack[i + base] );

      for ( j = 1; j < blend->lenBV; j++ )
        sum += FT_MulFix( cff_parse_fixed( parser, &parser->stack[delta++] ),
                          *weight++ );

      /* point parser stack to new value on blend_stack */
      parser->stack[i + base] = subFont->blend_top;

