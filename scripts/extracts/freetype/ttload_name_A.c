  {
    FT_Error      error;
    FT_Memory     memory = stream->memory;
    FT_ULong      table_pos, table_len;
    FT_ULong      storage_start, storage_limit;
    TT_NameTable  table;
    TT_Name       names    = NULL;
    TT_LangTag    langTags = NULL;

    static const FT_Frame_Field  name_table_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_NameTableRec

      FT_FRAME_START( 6 ),
        FT_FRAME_USHORT( format ),
        FT_FRAME_USHORT( numNameRecords ),
        FT_FRAME_USHORT( storageOffset ),
      FT_FRAME_END
    };

    static const FT_Frame_Field  name_record_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_NameRec

      /* no FT_FRAME_START */
        FT_FRAME_USHORT( platformID ),
        FT_FRAME_USHORT( encodingID ),
        FT_FRAME_USHORT( languageID ),
        FT_FRAME_USHORT( nameID ),
        FT_FRAME_USHORT( stringLength ),
        FT_FRAME_USHORT( stringOffset ),
      FT_FRAME_END
    };

    static const FT_Frame_Field  langTag_record_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_LangTagRec

      /* no FT_FRAME_START */
        FT_FRAME_USHORT( stringLength ),
        FT_FRAME_USHORT( stringOffset ),
      FT_FRAME_END
    };


    table         = &face->name_table;
    table->stream = stream;

    error = face->goto_table( face, TTAG_name, stream, &table_len );
    if ( error )
      goto Exit;

    table_pos = FT_STREAM_POS();

    if ( FT_STREAM_READ_FIELDS( name_table_fields, table ) )
      goto Exit;

    /* Some popular Asian fonts have an invalid `storageOffset' value (it */
    /* should be at least `6 + 12*numNameRecords').  However, the string  */
    /* offsets, computed as `storageOffset + entry->stringOffset', are    */
    /* valid pointers within the name table...                            */
    /*                                                                    */
    /* We thus can't check `storageOffset' right now.                     */
    /*                                                                    */
    storage_start = table_pos + 6 + 12 * table->numNameRecords;
    storage_limit = table_pos + table_len;

    if ( storage_start > storage_limit )
    {
      FT_ERROR(( "tt_face_load_name: invalid `name' table\n" ));
      error = FT_THROW( Name_Table_Missing );
      goto Exit;
    }

    /* `name' format 1 contains additional language tag records, */
    /* which we load first                                       */
    if ( table->format == 1 )
    {
      if ( FT_STREAM_SEEK( storage_start )            ||
           FT_READ_USHORT( table->numLangTagRecords ) )
        goto Exit;

      storage_start += 2 + 4 * table->numLangTagRecords;

      /* allocate language tag records array */
      if ( FT_QNEW_ARRAY( langTags, table->numLangTagRecords ) ||
           FT_FRAME_ENTER( table->numLangTagRecords * 4 )      )
        goto Exit;

      /* load language tags */
      {
        TT_LangTag  entry = langTags;
        TT_LangTag  limit = FT_OFFSET( entry, table->numLangTagRecords );


        for ( ; entry < limit; entry++ )
        {
          (void)FT_STREAM_READ_FIELDS( langTag_record_fields, entry );

          /* check that the langTag string is within the table */
          entry->stringOffset += table_pos + table->storageOffset;
          if ( entry->stringOffset                       < storage_start ||
               entry->stringOffset + entry->stringLength > storage_limit )
