    {
      FT_ERROR(( "woff2_open_font: invalid WOFF2 header\n" ));
      return FT_THROW( Invalid_Table );
    }

    FT_TRACE2(( "woff2_open_font: WOFF2 Header is valid.\n" ));

    woff2.ttc_fonts = NULL;

    /* Read table directory. */
    if ( FT_QNEW_ARRAY( tables, woff2.num_tables )  ||
         FT_QNEW_ARRAY( indices, woff2.num_tables ) )
      goto Exit;

    FT_TRACE2(( "\n" ));
    FT_TRACE2(( "  tag    flags    transform  origLen   transformLen   offset\n" ));
    FT_TRACE2(( "  -----------------------------------------------------------\n" ));
             /* "  XXXX  XXXXXXXX  XXXXXXXX   XXXXXXXX    XXXXXXXX    XXXXXXXX" */

    for ( nn = 0; nn < woff2.num_tables; nn++ )
    {
      WOFF2_Table  table = tables + nn;


      if ( FT_READ_BYTE( table->FlagByte ) )
        goto Exit;

      if ( ( table->FlagByte & 0x3f ) == 0x3f )
      {
        if ( FT_READ_ULONG( table->Tag ) )
          goto Exit;
      }
      else
      {
        table->Tag = woff2_known_tags( table->FlagByte & 0x3f );
        if ( !table->Tag )
        {
          FT_ERROR(( "woff2_open_font: Unknown table tag." ));
          error = FT_THROW( Invalid_Table );
          goto Exit;
        }
      }

      flags = 0;
      xform_version = ( table->FlagByte >> 6 ) & 0x03;

      /* 0 means xform for glyph/loca, non-0 for others. */
      if ( table->Tag == TTAG_glyf || table->Tag == TTAG_loca )
      {
        if ( xform_version == 0 )
          flags |= WOFF2_FLAGS_TRANSFORM;
      }
      else if ( xform_version != 0 )
        flags |= WOFF2_FLAGS_TRANSFORM;

      flags |= xform_version;

      if ( READ_BASE128( table->dst_length ) )
        goto Exit;

      table->TransformLength = table->dst_length;

      if ( ( flags & WOFF2_FLAGS_TRANSFORM ) != 0 )
      {
        if ( READ_BASE128( table->TransformLength ) )
          goto Exit;

        if ( table->Tag == TTAG_loca && table->TransformLength )
        {
          FT_ERROR(( "woff2_open_font: Invalid loca `transformLength'.\n" ));
          error = FT_THROW( Invalid_Table );
          goto Exit;
        }
      }

      if ( src_offset + table->TransformLength < src_offset )
      {
        FT_ERROR(( "woff2_open_font: invalid WOFF2 table directory.\n" ));
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }

      table->flags      = flags;
      table->src_offset = src_offset;
      table->src_length = table->TransformLength;
      src_offset       += table->TransformLength;
      table->dst_offset = 0;

      FT_TRACE2(( "  %c%c%c%c  %08d  %08d   %08ld    %08ld    %08ld\n",
                  (FT_Char)( table->Tag >> 24 ),
                  (FT_Char)( table->Tag >> 16 ),
                  (FT_Char)( table->Tag >> 8  ),
                  (FT_Char)( table->Tag       ),
                  table->FlagByte & 0x3f,
                  ( table->FlagByte >> 6 ) & 0x03,
                  table->dst_length,
                  table->TransformLength,
                  table->src_offset ));

      indices[nn] = table;
    }

    /* End of last table is uncompressed size. */
    last_table = indices[woff2.num_tables - 1];

    woff2.uncompressed_size = last_table->src_offset +
                              last_table->src_length;
    if ( woff2.uncompressed_size < last_table->src_offset )
    {
      error = FT_THROW( Invalid_Table );
      goto Exit;
    }

    FT_TRACE2(( "Table directory parsed.\n" ));

    /* Check for and read collection directory. */
    woff2.num_fonts      = 1;
    woff2.header_version = 0;

    if ( woff2.flavor == TTAG_ttcf )
    {
      FT_TRACE2(( "Font is a TTC, reading collection directory.\n" ));

