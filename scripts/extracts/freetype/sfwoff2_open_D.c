              sizeof ( WOFF2_Table ),
              compare_tags );

    /* reject fonts that have multiple tables with the same tag */
    for ( nn = 1; nn < woff2.num_tables; nn++ )
    {
      FT_Tag  tag = indices[nn]->Tag;


      if ( tag == indices[nn - 1]->Tag )
      {
        FT_ERROR(( "woff2_open_font:"
                   " multiple tables with tag `%c%c%c%c'.\n",
                   (FT_Char)( tag >> 24 ),
                   (FT_Char)( tag >> 16 ),
                   (FT_Char)( tag >> 8  ),
                   (FT_Char)( tag       ) ));
        error = FT_THROW( Invalid_Table );
        goto Exit;
      }
    }

    if ( woff2.uncompressed_size < 1 )
    {
      error = FT_THROW( Invalid_Table );
      goto Exit;
    }

    /* We must not blindly trust `uncompressed_size` since its   */
    /* value might be corrupted.  If it is too large, reject the */
    /* font.  In other words, we don't accept a WOFF2 font that  */
    /* expands to something larger than MAX_SFNT_SIZE.  If ever  */
    /* necessary, this limit can be easily adjusted.             */
    if ( woff2.uncompressed_size > MAX_SFNT_SIZE )
    {
      FT_ERROR(( "Uncompressed font too large.\n" ));
      error = FT_THROW( Array_Too_Large );
      goto Exit;
    }

    /* Allocate memory for uncompressed table data. */
    if ( FT_QALLOC( uncompressed_buf, woff2.uncompressed_size ) ||
         FT_FRAME_ENTER( woff2.totalCompressedSize )            )
      goto Exit;

    /* Uncompress the stream. */
    error = woff2_decompress( uncompressed_buf,
                              woff2.uncompressed_size,
                              stream->cursor,
                              woff2.totalCompressedSize );

    FT_FRAME_EXIT();

    if ( error )
      goto Exit;

    error = reconstruct_font( uncompressed_buf,
                              woff2.uncompressed_size,
                              indices,
                              &woff2,
                              &info,
                              &sfnt,
                              &sfnt_size,
                              memory );

    if ( error )
      goto Exit;

    /* Resize `sfnt' to actual size of sfnt stream. */
    if ( woff2.actual_sfnt_size < sfnt_size )
    {
      FT_TRACE5(( "Trimming sfnt stream from %lu to %lu.\n",
                  sfnt_size, woff2.actual_sfnt_size ));
      if ( FT_QREALLOC( sfnt,
                        (FT_ULong)( sfnt_size ),
                        (FT_ULong)( woff2.actual_sfnt_size ) ) )
        goto Exit;
    }

    /* `reconstruct_font' has done all the work. */
    /* Swap out stream and return.               */
    FT_Stream_OpenMemory( sfnt_stream, sfnt, woff2.actual_sfnt_size );
    sfnt_stream->memory = stream->memory;
    sfnt_stream->close  = stream_close;

    FT_Stream_Free(
      face->root.stream,
      ( face->root.face_flags & FT_FACE_FLAG_EXTERNAL_STREAM ) != 0 );

    face->root.stream      = sfnt_stream;
    face->root.face_flags &= ~FT_FACE_FLAG_EXTERNAL_STREAM;

    /* Set face_index to 0 or -1. */
    if ( *face_instance_index >= 0 )
      *face_instance_index = 0;
    else
      *face_instance_index = -1;

    FT_TRACE2(( "woff2_open_font: SFNT synthesized.\n" ));

  Exit:
    FT_FREE( tables );
    FT_FREE( indices );
    FT_FREE( uncompressed_buf );
    FT_FREE( info.x_mins );

    if ( woff2.ttc_fonts )
    {
      WOFF2_TtcFont  ttc_font = woff2.ttc_fonts;


      for ( nn = 0; nn < woff2.num_fonts; nn++ )
      {
        FT_FREE( ttc_font->table_indices );
        ttc_font++;
      }

      FT_FREE( woff2.ttc_fonts );
    }

    if ( error )
    {
      FT_FREE( sfnt );
      if ( sfnt_stream )
      {
        FT_Stream_Close( sfnt_stream );
        FT_FREE( sfnt_stream );
      }
    }

    return error;
  }


#undef READ_255USHORT
#undef READ_BASE128
#undef ROUND4
#undef WRITE_USHORT
#undef WRITE_ULONG
#undef WRITE_SHORT
#undef WRITE_SFNT_BUF
#undef WRITE_SFNT_BUF_AT

#undef N_CONTOUR_STREAM
#undef N_POINTS_STREAM
#undef FLAG_STREAM
#undef GLYPH_STREAM
#undef COMPOSITE_STREAM
#undef BBOX_STREAM
#undef INSTRUCTION_STREAM

#else /* !FT_CONFIG_OPTION_USE_BROTLI */

  /* ANSI C doesn't like empty source files */
  typedef int  sfwoff2_dummy_;

#endif /* !FT_CONFIG_OPTION_USE_BROTLI */


/* END */
