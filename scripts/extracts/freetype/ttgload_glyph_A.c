  {
    TT_Face       face = (TT_Face)glyph->face;
    FT_Error      error;
    TT_LoaderRec  loader;


    FT_TRACE1(( "TT_Load_Glyph: glyph index %d\n", glyph_index ));

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    /* try to load embedded bitmap (if any) */
    if ( size->strike_index != 0xFFFFFFFFUL      &&
         ( load_flags & FT_LOAD_NO_BITMAP ) == 0 &&
         IS_DEFAULT_INSTANCE( glyph->face )      )
    {
      FT_Fixed  x_scale = size->root.metrics.x_scale;
      FT_Fixed  y_scale = size->root.metrics.y_scale;


      error = load_sbit_image( size, glyph, glyph_index, load_flags );
      if ( FT_ERR_EQ( error, Missing_Bitmap ) )
      {
        /* the bitmap strike is incomplete and misses the requested glyph; */
        /* if we have a bitmap-only font, return an empty glyph            */
        if ( !FT_IS_SCALABLE( glyph->face ) )
        {
          FT_Short  left_bearing = 0;
          FT_Short  top_bearing  = 0;

          FT_UShort  advance_width  = 0;
          FT_UShort  advance_height = 0;


          /* to return an empty glyph, however, we need metrics data   */
          /* from the `hmtx' (or `vmtx') table; the assumption is that */
          /* empty glyphs are missing intentionally, representing      */
          /* whitespace - not having at least horizontal metrics is    */
          /* thus considered an error                                  */
          if ( !face->horz_metrics_size )
            return error;

          /* we now construct an empty bitmap glyph */
          TT_Get_HMetrics( face, glyph_index,
                           &left_bearing,
                           &advance_width );
          TT_Get_VMetrics( face, glyph_index,
                           0,
                           &top_bearing,
                           &advance_height );

          glyph->outline.n_points   = 0;
          glyph->outline.n_contours = 0;

          glyph->metrics.width  = 0;
          glyph->metrics.height = 0;

          glyph->metrics.horiBearingX = FT_MulFix( left_bearing, x_scale );
          glyph->metrics.horiBearingY = 0;
          glyph->metrics.horiAdvance  = FT_MulFix( advance_width, x_scale );

          glyph->metrics.vertBearingX = 0;
          glyph->metrics.vertBearingY = FT_MulFix( top_bearing, y_scale );
          glyph->metrics.vertAdvance  = FT_MulFix( advance_height, y_scale );

          glyph->format            = FT_GLYPH_FORMAT_BITMAP;
          glyph->bitmap.pixel_mode = FT_PIXEL_MODE_MONO;

          glyph->bitmap_left = 0;
          glyph->bitmap_top  = 0;

          return FT_Err_Ok;
        }
      }
      else if ( error )
      {
        /* return error if font is not scalable */
        if ( !FT_IS_SCALABLE( glyph->face ) )
          return error;
      }
      else
      {
        if ( FT_IS_SCALABLE( glyph->face ) ||
             FT_HAS_SBIX( glyph->face )    )
        {
          /* for the bbox we need the header only */
          (void)tt_loader_init( &loader, size, glyph, load_flags, TRUE );
          (void)load_truetype_glyph( &loader, glyph_index, 0, TRUE );
          tt_loader_done( &loader );
