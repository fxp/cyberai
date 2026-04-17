  TT_Load_Composite_Glyph( TT_Loader  loader )
  {
    FT_Error        error;
    FT_Byte*        p          = loader->cursor;
    FT_Byte*        limit      = loader->limit;
    FT_GlyphLoader  gloader    = loader->gloader;
    FT_Long         num_glyphs = loader->face->root.num_glyphs;
    FT_SubGlyph     subglyph;
    FT_UInt         num_subglyphs;


    num_subglyphs = 0;

    do
    {
      FT_Fixed  xx, xy, yy, yx;
      FT_UInt   count;


      /* check that we can load a new subglyph */
      error = FT_GlyphLoader_CheckSubGlyphs( gloader, num_subglyphs + 1 );
      if ( error )
        goto Fail;

      /* check space */
      if ( p + 4 > limit )
        goto Invalid_Composite;

      subglyph = gloader->current.subglyphs + num_subglyphs;

      subglyph->arg1 = subglyph->arg2 = 0;

      subglyph->flags = FT_NEXT_USHORT( p );
      subglyph->index = FT_NEXT_USHORT( p );

      /* we reject composites that have components */
      /* with invalid glyph indices                */
      if ( subglyph->index >= num_glyphs )
        goto Invalid_Composite;

      /* check space */
      count = 2;
      if ( subglyph->flags & ARGS_ARE_WORDS )
        count += 2;
      if ( subglyph->flags & WE_HAVE_A_SCALE )
        count += 2;
      else if ( subglyph->flags & WE_HAVE_AN_XY_SCALE )
        count += 4;
      else if ( subglyph->flags & WE_HAVE_A_2X2 )
        count += 8;

      if ( p + count > limit )
        goto Invalid_Composite;

      /* read arguments */
      if ( subglyph->flags & ARGS_ARE_XY_VALUES )
      {
        if ( subglyph->flags & ARGS_ARE_WORDS )
        {
          subglyph->arg1 = FT_NEXT_SHORT( p );
          subglyph->arg2 = FT_NEXT_SHORT( p );
        }
        else
        {
          subglyph->arg1 = FT_NEXT_CHAR( p );
          subglyph->arg2 = FT_NEXT_CHAR( p );
        }
      }
      else
      {
        if ( subglyph->flags & ARGS_ARE_WORDS )
        {
          subglyph->arg1 = (FT_Int)FT_NEXT_USHORT( p );
          subglyph->arg2 = (FT_Int)FT_NEXT_USHORT( p );
        }
        else
        {
          subglyph->arg1 = (FT_Int)FT_NEXT_BYTE( p );
          subglyph->arg2 = (FT_Int)FT_NEXT_BYTE( p );
        }
      }

      /* read transform */
      xx = yy = 0x10000L;
      xy = yx = 0;

      if ( subglyph->flags & WE_HAVE_A_SCALE )
      {
        xx = (FT_Fixed)FT_NEXT_SHORT( p ) * 4;
        yy = xx;
      }
      else if ( subglyph->flags & WE_HAVE_AN_XY_SCALE )
      {
        xx = (FT_Fixed)FT_NEXT_SHORT( p ) * 4;
        yy = (FT_Fixed)FT_NEXT_SHORT( p ) * 4;
      }
      else if ( subglyph->flags & WE_HAVE_A_2X2 )
