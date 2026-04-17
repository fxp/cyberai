  {
    FT_Error        error   = FT_Err_Ok;
    FT_Fixed        x_scale, y_scale;
    FT_ULong        offset;
    TT_Face         face    = loader->face;
    FT_GlyphLoader  gloader = loader->gloader;

    FT_Bool  opened_frame = 0;

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    FT_StreamRec    inc_stream;
    FT_Data         glyph_data;
    FT_Bool         glyph_data_loaded = 0;
#endif


#ifdef FT_DEBUG_LEVEL_TRACE
    if ( recurse_count )
      FT_TRACE5(( "  nesting level: %d\n", recurse_count ));
#endif

    /* some fonts have an incorrect value of `maxComponentDepth' */
    if ( recurse_count > face->max_profile.maxComponentDepth )
    {
      FT_TRACE1(( "load_truetype_glyph: maxComponentDepth set to %d\n",
                  recurse_count ));
      face->max_profile.maxComponentDepth = (FT_UShort)recurse_count;
    }

#ifndef FT_CONFIG_OPTION_INCREMENTAL
    /* check glyph index */
    if ( glyph_index >= (FT_UInt)face->root.num_glyphs )
    {
      error = FT_THROW( Invalid_Glyph_Index );
      goto Exit;
    }
#endif

    loader->glyph_index = glyph_index;

    if ( loader->load_flags & FT_LOAD_NO_SCALE )
    {
      x_scale = 0x10000L;
      y_scale = 0x10000L;
    }
    else
    {
      x_scale = loader->size->metrics->x_scale;
      y_scale = loader->size->metrics->y_scale;
    }

    /* Set `offset' to the start of the glyph relative to the start of */
    /* the `glyf' table, and `byte_len' to the length of the glyph in  */
    /* bytes.                                                          */

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    /* If we are loading glyph data via the incremental interface, set */
    /* the loader stream to a memory stream reading the data returned  */
    /* by the interface.                                               */
    if ( face->root.internal->incremental_interface )
    {
      error = face->root.internal->incremental_interface->funcs->get_glyph_data(
                face->root.internal->incremental_interface->object,
                glyph_index, &glyph_data );
      if ( error )
        goto Exit;

      glyph_data_loaded = 1;
      offset            = 0;
      loader->byte_len  = glyph_data.length;

      FT_ZERO( &inc_stream );
      FT_Stream_OpenMemory( &inc_stream,
                            glyph_data.pointer,
                            glyph_data.length );

      loader->stream = &inc_stream;
    }
    else

#endif /* FT_CONFIG_OPTION_INCREMENTAL */
    {
      FT_ULong  len;


      offset = tt_face_get_location( FT_FACE( face ), glyph_index, &len );

      loader->byte_len = (FT_UInt)len;
    }

    if ( loader->byte_len > 0 )
    {
#ifdef FT_CONFIG_OPTION_INCREMENTAL
      /* for the incremental interface, `glyf_offset' is always zero */
      if ( !face->glyf_offset                          &&
           !face->root.internal->incremental_interface )
#else
      if ( !face->glyf_offset )
#endif /* FT_CONFIG_OPTION_INCREMENTAL */
      {
        FT_TRACE2(( "no `glyf' table but non-zero `loca' entry\n" ));
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }

      error = face->access_glyph_frame( loader, glyph_index,
                                        face->glyf_offset + offset,
                                        loader->byte_len );
      if ( error )
        goto Exit;

      /* read glyph header first */
      error = face->read_glyph_header( loader );

      face->forget_glyph_frame( loader );

      if ( error )
        goto Exit;
    }

    /* a space glyph */
    if ( loader->byte_len == 0 || loader->n_contours == 0 )
    {
      loader->bbox.xMin = 0;
      loader->bbox.xMax = 0;
      loader->bbox.yMin = 0;
      loader->bbox.yMax = 0;
    }

    /* the metrics must be computed after loading the glyph header */
    /* since we need the glyph's `yMax' value in case the vertical */
    /* metrics must be emulated                                    */
    error = tt_get_metrics( loader, glyph_index );
    if ( error )
      goto Exit;

    if ( header_only )
      goto Exit;

    if ( loader->byte_len == 0 || loader->n_contours == 0 )
    {
#ifdef FT_CONFIG_OPTION_INCREMENTAL
      tt_get_metrics_incremental( loader, glyph_index );
#endif
      tt_loader_set_pp( loader );


#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
