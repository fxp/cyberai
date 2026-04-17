                    (FT_Char)( entry.Tag >> 8  ),
                    (FT_Char)( entry.Tag       ),
                    entry.Offset,
                    entry.Length,
                    entry.CheckSum ));
#endif

      /* ignore duplicate tables – the first one wins */
      duplicate = 0;
      for ( i = 0; i < valid_entries; i++ )
      {
        if ( face->dir_tables[i].Tag == entry.Tag )
        {
          duplicate = 1;
          break;
        }
      }
      if ( duplicate )
      {
        FT_TRACE2(( "  (duplicate, ignored)\n" ));
        continue;
      }
      else
      {
        FT_TRACE2(( "\n" ));

        /* we finally have a valid entry */
        face->dir_tables[valid_entries++] = entry;
      }
    }

    /* final adjustment to number of tables */
    face->num_tables = valid_entries;

    FT_FRAME_EXIT();

    if ( !valid_entries )
    {
      FT_TRACE2(( "tt_face_load_font_dir: no valid tables found\n" ));
      error = FT_THROW( Unknown_File_Format );
      goto Exit;
    }

    FT_TRACE2(( "table directory loaded\n" ));
    FT_TRACE2(( "\n" ));

  Exit:
    return error;
  }


  /**************************************************************************
   *
   * @Function:
   *   tt_face_load_any
   *
   * @Description:
   *   Loads any font table into client memory.
   *
   * @Input:
   *   face ::
   *     The face object to look for.
   *
   *   tag ::
   *     The tag of table to load.  Use the value 0 if you want
   *     to access the whole font file, else set this parameter
   *     to a valid TrueType table tag that you can forge with
   *     the MAKE_TT_TAG macro.
   *
   *   offset ::
   *     The starting offset in the table (or the file if
   *     tag == 0).
   *
   *   length ::
   *     The address of the decision variable:
   *
   *     If length == NULL:
   *       Loads the whole table.  Returns an error if
   *       `offset' == 0!
   *
   *     If *length == 0:
   *       Exits immediately; returning the length of the given
   *       table or of the font file, depending on the value of
   *       `tag'.
   *
   *     If *length != 0:
   *       Loads the next `length' bytes of table or font,
   *       starting at offset `offset' (in table or font too).
   *
   * @Output:
   *   buffer ::
   *     The address of target buffer.
   *
   * @Return:
   *   FreeType error code.  0 means success.
   */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_any( TT_Face    face,
                    FT_ULong   tag,
                    FT_Long    offset,
                    FT_Byte*   buffer,
                    FT_ULong*  length )
  {
    FT_Error   error;
    FT_Stream  stream;
    TT_Table   table;
    FT_ULong   size;


    if ( tag != 0 )
    {
      /* look for tag in font directory */
      table = tt_face_lookup_table( face, tag );
      if ( !table )
      {
        error = FT_THROW( Table_Missing );
        goto Exit;
      }

      offset += table->Offset;
      size    = table->Length;
    }
    else
      /* tag == 0 -- the user wants to access the font file directly */
      size = face->root.stream->size;

    if ( length && *length == 0 )
    {
      *length = size;

      return FT_Err_Ok;
    }

    if ( length )
      size = *length;

    stream = face->root.stream;
    /* the `if' is syntactic sugar for picky compilers */
    if ( FT_STREAM_READ_AT( offset, buffer, size ) )
      goto Exit;

  Exit:
    return error;
  }


  /**************************************************************************
   *
   * @Function:
   *   tt_face_load_generic_header
   *
   * @Description:
   *   Loads the TrueType table `head' or `bhed'.
   *
   * @Input:
   *   face ::
   *     A handle to the target face object.
   *
   *   stream ::
   *     The input stream.
   *
   * @Return:
   *   FreeType error code.  0 means success.
   */
  static FT_Error
  tt_face_load_generic_header( TT_Face    face,
                               FT_Stream  stream,
                               FT_ULong   tag )
