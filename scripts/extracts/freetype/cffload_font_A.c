  {
    static const FT_Frame_Field  cff_header_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  CFF_FontRec

      FT_FRAME_START( 3 ),
        FT_FRAME_BYTE( version_major ),
        FT_FRAME_BYTE( version_minor ),
        FT_FRAME_BYTE( header_size ),
      FT_FRAME_END
    };

    FT_Error         error;
    FT_Memory        memory = stream->memory;
    FT_ULong         base_offset;
    CFF_FontRecDict  dict;
    CFF_IndexRec     string_index;
    FT_UInt          subfont_index;


    FT_ZERO( font );
    FT_ZERO( &string_index );

    dict        = &font->top_font.font_dict;
    base_offset = FT_STREAM_POS();

    font->library     = library;
    font->stream      = stream;
    font->memory      = memory;
    font->cff2        = cff2;
    font->base_offset = base_offset;

    /* read CFF font header */
    if ( FT_STREAM_READ_FIELDS( cff_header_fields, font ) )
      goto Exit;

    if ( cff2 )
    {
      if ( font->version_major != 2 ||
           font->header_size < 5    )
      {
        FT_TRACE2(( "  not a CFF2 font header\n" ));
        error = FT_THROW( Unknown_File_Format );
        goto Exit;
      }

      if ( FT_READ_USHORT( font->top_dict_length ) )
        goto Exit;
    }
    else
    {
      FT_Byte  absolute_offset;


      if ( FT_READ_BYTE( absolute_offset ) )
        goto Exit;

      if ( font->version_major != 1 ||
           font->header_size < 4    ||
           absolute_offset > 4      )
      {
        FT_TRACE2(( "  not a CFF font header\n" ));
        error = FT_THROW( Unknown_File_Format );
        goto Exit;
      }
    }

    /* skip the rest of the header */
    if ( FT_STREAM_SEEK( base_offset + font->header_size ) )
    {
      /* For pure CFFs we have read only four bytes so far.  Contrary to */
      /* other formats like SFNT those bytes doesn't define a signature; */
      /* it is thus possible that the font isn't a CFF at all.           */
      if ( pure_cff )
      {
        FT_TRACE2(( "  not a CFF file\n" ));
        error = FT_THROW( Unknown_File_Format );
      }
      goto Exit;
    }

    if ( cff2 )
    {
      /* For CFF2, the top dict data immediately follow the header    */
      /* and the length is stored in the header `offSize' field;      */
      /* there is no index for it.                                    */
      /*                                                              */
      /* Use the `font_dict_index' to save the current position       */
      /* and length of data, but leave count at zero as an indicator. */
      FT_ZERO( &font->font_dict_index );

      font->font_dict_index.data_offset = FT_STREAM_POS();
      font->font_dict_index.data_size   = font->top_dict_length;

      /* skip the top dict data for now, we will parse it later */
      if ( FT_STREAM_SKIP( font->top_dict_length ) )
        goto Exit;

      /* next, read the global subrs index */
      if ( FT_SET_ERROR( cff_index_init( &font->global_subrs_index,
                                         stream, 1, cff2 ) ) )
        goto Exit;
    }
