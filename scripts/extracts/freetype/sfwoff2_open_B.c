      if ( FT_READ_ULONG( woff2.header_version ) )
        goto Exit;

      if ( woff2.header_version != 0x00010000 &&
           woff2.header_version != 0x00020000 )
      {
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }

      if ( READ_255USHORT( woff2.num_fonts ) )
        goto Exit;

      if ( !woff2.num_fonts )
      {
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }

      FT_TRACE4(( "Number of fonts in TTC: %d\n", woff2.num_fonts ));

      /* pre-zero pointers within in case of failure */
      if ( FT_NEW_ARRAY( woff2.ttc_fonts, woff2.num_fonts ) )
        goto Exit;

      for ( nn = 0; nn < woff2.num_fonts; nn++ )
      {
        WOFF2_TtcFont  ttc_font = woff2.ttc_fonts + nn;


        if ( READ_255USHORT( ttc_font->num_tables ) )
          goto Exit;
        if ( FT_READ_ULONG( ttc_font->flavor ) )
          goto Exit;

        if ( FT_QNEW_ARRAY( ttc_font->table_indices, ttc_font->num_tables ) )
          goto Exit;

        FT_TRACE5(( "Number of tables in font %d: %d\n",
                    nn, ttc_font->num_tables ));

#ifdef FT_DEBUG_LEVEL_TRACE
        if ( ttc_font->num_tables )
          FT_TRACE6(( "  Indices: " ));
#endif

        glyf_index = 0;
        loca_index = 0;

        for ( j = 0; j < ttc_font->num_tables; j++ )
        {
          FT_UShort    table_index;
          WOFF2_Table  table;


          if ( READ_255USHORT( table_index ) )
            goto Exit;

          FT_TRACE6(( "%hu ", table_index ));
          if ( table_index >= woff2.num_tables )
          {
            FT_ERROR(( "woff2_open_font: invalid table index\n" ));
            error = FT_THROW( Invalid_Table );
            goto Exit;
          }

          ttc_font->table_indices[j] = table_index;

          table = indices[table_index];
          if ( table->Tag == TTAG_loca )
            loca_index = table_index;
          if ( table->Tag == TTAG_glyf )
            glyf_index = table_index;
        }

#ifdef FT_DEBUG_LEVEL_TRACE
        if ( ttc_font->num_tables )
          FT_TRACE6(( "\n" ));
#endif

        /* glyf and loca must be consecutive */
        if ( glyf_index > 0 || loca_index > 0 )
        {
          if ( glyf_index > loca_index      ||
               loca_index - glyf_index != 1 )
          {
            error = FT_THROW( Invalid_Table );
            goto Exit;
          }
        }
      }

      /* Collection directory reading complete. */
      FT_TRACE2(( "WOFF2 collection directory is valid.\n" ));
    }
    else
      woff2.ttc_fonts = NULL;

    woff2.compressed_offset = FT_STREAM_POS();
    file_offset             = ROUND4( woff2.compressed_offset +
                                      woff2.totalCompressedSize );

    /* Some more checks before we start reading the tables. */
    if ( file_offset > woff2.length )
    {
      error = FT_THROW( Invalid_Table );
      goto Exit;
    }

    if ( woff2.metaOffset )
