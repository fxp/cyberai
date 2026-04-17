  {
    FT_Error        error;
    FT_Byte*        p          = load->cursor;
    FT_Byte*        limit      = load->limit;
    FT_GlyphLoader  gloader    = load->gloader;
    FT_Outline*     outline    = &gloader->current.outline;
    FT_Int          n_contours = load->n_contours;
    FT_Int          n_points;
    FT_UShort       n_ins;

    FT_Byte         *flag, *flag_limit;
    FT_Byte         c, count;
    FT_Vector       *vec, *vec_limit;
    FT_Pos          x, y;
    FT_UShort       *cont, *cont_limit;
    FT_Int          last;


    /* check that we can add the contours to the glyph */
    error = FT_GLYPHLOADER_CHECK_POINTS( gloader, 0, n_contours );
    if ( error )
      goto Fail;

    /* check space for contours array + instructions count */
    if ( n_contours >= 0xFFF || p + 2 * n_contours + 2 > limit )
      goto Invalid_Outline;

    /* reading the contours' endpoints & number of points */
    cont       = outline->contours;
    cont_limit = cont + n_contours;

    last = -1;
    for ( ; cont < cont_limit; cont++ )
    {
      *cont = FT_NEXT_USHORT( p );

      if ( *cont <= last )
        goto Invalid_Outline;

      last = *cont;
    }

    n_points = last + 1;

    FT_TRACE5(( "  # of points: %d\n", n_points ));

    /* note that we will add four phantom points later */
    error = FT_GLYPHLOADER_CHECK_POINTS( gloader, n_points + 4, 0 );
    if ( error )
      goto Fail;

    /* space checked above */
    n_ins = FT_NEXT_USHORT( p );

    FT_TRACE5(( "  Instructions size: %u\n", n_ins ));

    /* check instructions size */
    if ( p + n_ins > limit )
    {
      FT_TRACE1(( "TT_Load_Simple_Glyph: excessive instruction count\n" ));
      error = FT_THROW( Too_Many_Hints );
      goto Fail;
    }

#ifdef TT_USE_BYTECODE_INTERPRETER

    if ( IS_HINTED( load->load_flags ) )
    {
      TT_ExecContext  exec = load->exec;
      FT_Memory       memory = exec->memory;


      if ( exec->glyphSize )
        FT_FREE( exec->glyphIns );
      exec->glyphSize = 0;

      /* we don't trust `maxSizeOfInstructions' in the `maxp' table */
      /* and thus allocate the bytecode array size by ourselves     */
      if ( n_ins )
      {
        if ( FT_DUP( exec->glyphIns, p, n_ins ) )
          return error;

        exec->glyphSize  = n_ins;
      }
    }

#endif /* TT_USE_BYTECODE_INTERPRETER */

    p += n_ins;

    /* reading the point tags */
    flag       = outline->tags;
    flag_limit = flag + n_points;

    FT_ASSERT( flag );

    while ( flag < flag_limit )
    {
      if ( p + 1 > limit )
        goto Invalid_Outline;

      *flag++ = c = FT_NEXT_BYTE( p );
      if ( c & REPEAT_FLAG )
      {
        if ( p + 1 > limit )
          goto Invalid_Outline;

        count = FT_NEXT_BYTE( p );
        if ( flag + (FT_Int)count > flag_limit )
          goto Invalid_Outline;

        for ( ; count > 0; count-- )
          *flag++ = c;
      }
    }

    /* retain the overlap flag */
    if ( n_points && outline->tags[0] & OVERLAP_SIMPLE )
      gloader->base.outline.flags |= FT_OUTLINE_OVERLAP;

    /* reading the X coordinates */

    vec       = outline->points;
    vec_limit = vec + n_points;
    flag      = outline->tags;
    x         = 0;

    for ( ; vec < vec_limit; vec++, flag++ )
    {
      FT_Pos   delta = 0;
      FT_Byte  f     = *flag;


      if ( f & X_SHORT_VECTOR )
      {
        if ( p + 1 > limit )
          goto Invalid_Outline;

        delta = (FT_Pos)FT_NEXT_BYTE( p );
        if ( !( f & X_POSITIVE ) )
          delta = -delta;
      }
      else if ( !( f & SAME_X ) )
      {
        if ( p + 2 > limit )
          goto Invalid_Outline;

        delta = (FT_Pos)FT_NEXT_SHORT( p );
      }

      x     += delta;
      vec->x = x;
    }

    /* reading the Y coordinates */

    vec       = outline->points;
    vec_limit = vec + n_points;
    flag      = outline->tags;
    y         = 0;

    for ( ; vec < vec_limit; vec++, flag++ )
    {
      FT_Pos   delta = 0;
      FT_Byte  f     = *flag;


      if ( f & Y_SHORT_VECTOR )
      {
        if ( p + 1 > limit )
          goto Invalid_Outline;

        delta = (FT_Pos)FT_NEXT_BYTE( p );
        if ( !( f & Y_POSITIVE ) )
          delta = -delta;
      }
      else if ( !( f & SAME_Y ) )
      {
        if ( p + 2 > limit )
          goto Invalid_Outline;

        delta = (FT_Pos)FT_NEXT_SHORT( p );
      }

      y     += delta;
      vec->y = y;

      /* the cast is for stupid compilers */
      *flag  = (FT_Byte)( f & ON_CURVE_POINT );
    }

    outline->n_points   = (FT_UShort)n_points;
    outline->n_contours = (FT_UShort)n_contours;

    load->cursor = p;

  Fail:
    return error;

  Invalid_Outline:
    error = FT_THROW( Invalid_Outline );
    goto Fail;
  }

