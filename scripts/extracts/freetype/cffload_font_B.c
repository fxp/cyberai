    else
    {
      /* for CFF, read the name, top dict, string and global subrs index */
      if ( FT_SET_ERROR( cff_index_init( &font->name_index,
                                         stream, 0, cff2 ) ) )
      {
        if ( pure_cff )
        {
          FT_TRACE2(( "  not a CFF file\n" ));
          error = FT_THROW( Unknown_File_Format );
        }
        goto Exit;
      }

      /* if we have an empty font name,      */
      /* it must be the only font in the CFF */
      if ( font->name_index.count > 1                          &&
           font->name_index.data_size < font->name_index.count )
      {
        /* for pure CFFs, we still haven't checked enough bytes */
        /* to be sure that it is a CFF at all                   */
        error = pure_cff ? FT_THROW( Unknown_File_Format )
                         : FT_THROW( Invalid_File_Format );
        goto Exit;
      }

      if ( FT_SET_ERROR( cff_index_init( &font->font_dict_index,
                                         stream, 0, cff2 ) )                 ||
           FT_SET_ERROR( cff_index_init( &string_index,
                                         stream, 1, cff2 ) )                 ||
           FT_SET_ERROR( cff_index_init( &font->global_subrs_index,
                                         stream, 1, cff2 ) )                 ||
           FT_SET_ERROR( cff_index_get_pointers( &string_index,
                                                 &font->strings,
                                                 &font->string_pool,
                                                 &font->string_pool_size ) ) )
        goto Exit;

      /* there must be a Top DICT index entry for each name index entry */
      if ( font->name_index.count > font->font_dict_index.count )
      {
        FT_ERROR(( "cff_font_load:"
                   " not enough entries in Top DICT index\n" ));
        error = FT_THROW( Invalid_File_Format );
        goto Exit;
      }
    }

    font->num_strings = string_index.count;

    if ( pure_cff )
    {
      /* well, we don't really forget the `disabled' fonts... */
      subfont_index = (FT_UInt)( face_index & 0xFFFF );

      if ( face_index > 0 && subfont_index >= font->name_index.count )
      {
        FT_ERROR(( "cff_font_load:"
                   " invalid subfont index for pure CFF font (%d)\n",
                   subfont_index ));
        error = FT_THROW( Invalid_Argument );
        goto Exit;
      }

      font->num_faces = font->name_index.count;
    }
    else
    {
      subfont_index = 0;

      if ( font->name_index.count > 1 )
      {
        FT_ERROR(( "cff_font_load:"
                   " invalid CFF font with multiple subfonts\n" ));
        FT_ERROR(( "              "
                   " in SFNT wrapper\n" ));
        error = FT_THROW( Invalid_File_Format );
        goto Exit;
      }
    }

    /* in case of a font format check, simply exit now */
    if ( face_index < 0 )
      goto Exit;

    /* now, parse the top-level font dictionary */
    FT_TRACE4(( "parsing top-level\n" ));
    error = cff_subfont_load( &font->top_font,
                              &font->font_dict_index,
                              subfont_index,
                              stream,
                              base_offset,
                              cff2 ? CFF2_CODE_TOPDICT : CFF_CODE_TOPDICT,
                              font,
                              face );
    if ( error )
      goto Exit;

    if ( FT_STREAM_SEEK( base_offset + dict->charstrings_offset ) )
      goto Exit;

    error = cff_index_init( &font->charstrings_index, stream, 0, cff2 );
    if ( error )
      goto Exit;

