    {
      if ( file_offset != woff2.metaOffset )
      {
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }
      file_offset = ROUND4( woff2.metaOffset + woff2.metaLength );
    }

    if ( woff2.privOffset )
    {
      if ( file_offset != woff2.privOffset )
      {
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }
      file_offset = ROUND4( woff2.privOffset + woff2.privLength );
    }

    if ( file_offset != ( ROUND4( woff2.length ) ) )
    {
      error = FT_THROW( Invalid_Table );
      goto Exit;
    }

    /* Validate requested face index. */
    *num_faces = woff2.num_fonts;
    /* value -(N+1) requests information on index N */
    if ( *face_instance_index < 0 && face_index > 0 )
      face_index--;

    if ( face_index >= woff2.num_fonts )
    {
      if ( *face_instance_index >= 0 )
      {
        error = FT_THROW( Invalid_Argument );
        goto Exit;
      }
      else
        face_index = 0;
    }

    /* Only retain tables of the requested face in a TTC. */
    if ( woff2.header_version )
    {
      WOFF2_TtcFont  ttc_font = woff2.ttc_fonts + face_index;


      if ( ttc_font->num_tables == 0 || ttc_font->num_tables > 0xFFFU )
      {
        FT_ERROR(( "woff2_open_font: invalid WOFF2 CollectionFontEntry\n" ));
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }

      /* Create a temporary array. */
      if ( FT_QNEW_ARRAY( temp_indices,
                          ttc_font->num_tables ) )
        goto Exit;

      FT_TRACE4(( "Storing tables for TTC face index %d.\n", face_index ));
      for ( nn = 0; nn < ttc_font->num_tables; nn++ )
        temp_indices[nn] = indices[ttc_font->table_indices[nn]];

      /* Resize array to required size. */
      if ( FT_QRENEW_ARRAY( indices,
                            woff2.num_tables,
                            ttc_font->num_tables ) )
        goto Exit;

      for ( nn = 0; nn < ttc_font->num_tables; nn++ )
        indices[nn] = temp_indices[nn];

      FT_FREE( temp_indices );

      /* Change header values. */
      woff2.flavor     = ttc_font->flavor;
      woff2.num_tables = ttc_font->num_tables;
    }

    /* We need to allocate this much at the minimum. */
    sfnt_size = 12 + woff2.num_tables * 16UL;
    /* This is what we normally expect.                              */
    /* Initially trust `totalSfntSize' and change later as required. */
    if ( woff2.totalSfntSize > sfnt_size )
    {
      /* However, adjust the value to something reasonable. */

      /* Factor 64 is heuristic. */
      if ( ( woff2.totalSfntSize >> 6 ) > woff2.length )
        sfnt_size = woff2.length << 6;
      else
        sfnt_size = woff2.totalSfntSize;

      if ( sfnt_size >= MAX_SFNT_SIZE )
        sfnt_size = MAX_SFNT_SIZE;

#ifdef FT_DEBUG_LEVEL_TRACE
      if ( sfnt_size != woff2.totalSfntSize )
        FT_TRACE4(( "adjusting estimate of uncompressed font size"
                    " to %lu bytes\n",
                    sfnt_size ));
#endif
    }

    /* Write sfnt header. */
    if ( FT_QALLOC( sfnt, sfnt_size ) ||
         FT_NEW( sfnt_stream )        )
      goto Exit;

    {
      FT_Byte*  sfnt_header = sfnt;

      FT_Int  entrySelector = FT_MSB( woff2.num_tables );
      FT_Int  searchRange   = ( 1 << entrySelector ) * 16;
      FT_Int  rangeShift    = woff2.num_tables * 16 - searchRange;


      WRITE_ULONG ( sfnt_header, woff2.flavor );
      WRITE_USHORT( sfnt_header, woff2.num_tables );
      WRITE_USHORT( sfnt_header, searchRange );
      WRITE_USHORT( sfnt_header, entrySelector );
      WRITE_USHORT( sfnt_header, rangeShift );
    }

    info.header_checksum = compute_ULong_sum( sfnt, 12 );

    /* Sort tables by tag. */
    ft_qsort( indices,
              woff2.num_tables,
