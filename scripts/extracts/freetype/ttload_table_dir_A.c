  {
    TT_Table  entry;
    TT_Table  limit;
#ifdef FT_DEBUG_LEVEL_TRACE
    FT_Bool   zero_length = FALSE;
#endif


    FT_TRACE4(( "tt_face_lookup_table: %p, `%c%c%c%c' -- ",
                (void *)face,
                (FT_Char)( tag >> 24 ),
                (FT_Char)( tag >> 16 ),
                (FT_Char)( tag >> 8  ),
                (FT_Char)( tag       ) ));

    entry = face->dir_tables;
    limit = entry + face->num_tables;

    for ( ; entry < limit; entry++ )
    {
      /* For compatibility with Windows, we consider    */
      /* zero-length tables the same as missing tables. */
      if ( entry->Tag == tag )
      {
        if ( entry->Length != 0 )
        {
          FT_TRACE4(( "found table.\n" ));
          return entry;
        }
#ifdef FT_DEBUG_LEVEL_TRACE
        zero_length = TRUE;
#endif
      }
    }

#ifdef FT_DEBUG_LEVEL_TRACE
    if ( zero_length )
      FT_TRACE4(( "ignoring empty table\n" ));
    else
      FT_TRACE4(( "could not find table\n" ));
#endif

    return NULL;
  }


  /**************************************************************************
   *
   * @Function:
   *   tt_face_goto_table
   *
   * @Description:
   *   Looks for a TrueType table by name, then seek a stream to it.
   *
   * @Input:
   *   face ::
   *     A face object handle.
   *
   *   tag ::
   *     The searched tag.
   *
   *   stream ::
   *     The stream to seek when the table is found.
   *
   * @Output:
   *   length ::
   *     The length of the table if found, undefined otherwise.
   *
   * @Return:
   *   FreeType error code.  0 means success.
   */
  FT_LOCAL_DEF( FT_Error )
  tt_face_goto_table( TT_Face    face,
                      FT_ULong   tag,
                      FT_Stream  stream,
                      FT_ULong*  length )
  {
    TT_Table  table;
    FT_Error  error;


    table = tt_face_lookup_table( face, tag );
    if ( table )
    {
      if ( length )
        *length = table->Length;

      if ( FT_STREAM_SEEK( table->Offset ) )
        goto Exit;
    }
    else
      error = FT_THROW( Table_Missing );

  Exit:
    return error;
  }


  /* Here, we                                                         */
  /*                                                                  */
  /* - check that `num_tables' is valid (and adjust it if necessary); */
  /*   also return the number of valid table entries                  */
  /*                                                                  */
  /* - look for a `head' table, check its size, and parse it to check */
  /*   whether its `magic' field is correctly set                     */
  /*                                                                  */
  /* - errors (except errors returned by stream handling)             */
  /*                                                                  */
  /*     SFNT_Err_Unknown_File_Format:                                */
  /*       no table is defined in directory, it is not sfnt-wrapped   */
  /*       data                                                       */
  /*     SFNT_Err_Table_Missing:                                      */
