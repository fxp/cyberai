  {
    SFNT_HeaderRec  sfnt;
    FT_Error        error;
    FT_Memory       memory = stream->memory;
    FT_UShort       nn, valid_entries = 0;

    static const FT_Frame_Field  offset_table_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  SFNT_HeaderRec

      FT_FRAME_START( 8 ),
        FT_FRAME_USHORT( num_tables ),
        FT_FRAME_USHORT( search_range ),
        FT_FRAME_USHORT( entry_selector ),
        FT_FRAME_USHORT( range_shift ),
      FT_FRAME_END
    };


    FT_TRACE2(( "tt_face_load_font_dir: %p\n", (void *)face ));

    /* read the offset table */

    sfnt.offset = FT_STREAM_POS();

    if ( FT_READ_ULONG( sfnt.format_tag )                    ||
         FT_STREAM_READ_FIELDS( offset_table_fields, &sfnt ) )
      goto Exit;

    /* many fonts don't have these fields set correctly */
#if 0
    if ( sfnt.search_range != 1 << ( sfnt.entry_selector + 4 )        ||
         sfnt.search_range + sfnt.range_shift != sfnt.num_tables << 4 )
      return FT_THROW( Unknown_File_Format );
#endif

    /* load the table directory */

    FT_TRACE2(( "-- Number of tables: %10hu\n",   sfnt.num_tables ));
    FT_TRACE2(( "-- Format version:   0x%08lx\n", sfnt.format_tag ));

    if ( sfnt.format_tag != TTAG_OTTO )
    {
      /* check first */
      error = check_table_dir( &sfnt, stream, &valid_entries );
      if ( error )
      {
        FT_TRACE2(( "tt_face_load_font_dir:"
                    " invalid table directory for TrueType\n" ));
        goto Exit;
      }
    }
    else
    {
      valid_entries = sfnt.num_tables;
      if ( !valid_entries )
      {
        FT_TRACE2(( "tt_face_load_font_dir: no valid tables found\n" ));
        error = FT_THROW( Unknown_File_Format );
        goto Exit;
      }
    }

    face->num_tables = valid_entries;
    face->format_tag = sfnt.format_tag;

    if ( FT_QNEW_ARRAY( face->dir_tables, face->num_tables ) )
      goto Exit;

    if ( FT_STREAM_SEEK( sfnt.offset + 12 )      ||
         FT_FRAME_ENTER( sfnt.num_tables * 16L ) )
      goto Exit;

    FT_TRACE2(( "\n" ));
    FT_TRACE2(( "  tag    offset    length   checksum\n" ));
    FT_TRACE2(( "  ----------------------------------\n" ));

    valid_entries = 0;
    for ( nn = 0; nn < sfnt.num_tables; nn++ )
    {
      TT_TableRec  entry;
      FT_UShort    i;
      FT_Bool      duplicate;


      entry.Tag      = FT_GET_TAG4();
      entry.CheckSum = FT_GET_ULONG();
      entry.Offset   = FT_GET_ULONG();
      entry.Length   = FT_GET_ULONG();

      /* ignore invalid tables that can't be sanitized */

      if ( entry.Offset > stream->size )
        continue;
      else if ( entry.Length > stream->size - entry.Offset )
      {
        if ( entry.Tag == TTAG_hmtx ||
             entry.Tag == TTAG_vmtx )
        {
#ifdef FT_DEBUG_LEVEL_TRACE
          FT_ULong  old_length = entry.Length;
#endif


          /* make metrics table length a multiple of 4 */
          entry.Length = ( stream->size - entry.Offset ) & ~3U;

          FT_TRACE2(( "  %c%c%c%c  %08lx  %08lx  %08lx"
                      " (sanitized; original length %08lx)",
                      (FT_Char)( entry.Tag >> 24 ),
                      (FT_Char)( entry.Tag >> 16 ),
                      (FT_Char)( entry.Tag >> 8  ),
                      (FT_Char)( entry.Tag       ),
                      entry.Offset,
                      entry.Length,
                      entry.CheckSum,
                      old_length ));
        }
        else
          continue;
      }
#ifdef FT_DEBUG_LEVEL_TRACE
      else
        FT_TRACE2(( "  %c%c%c%c  %08lx  %08lx  %08lx",
                    (FT_Char)( entry.Tag >> 24 ),
                    (FT_Char)( entry.Tag >> 16 ),
