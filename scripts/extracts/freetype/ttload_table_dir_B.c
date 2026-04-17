  /*       table directory is valid, but essential tables             */
  /*       (head/bhed/SING) are missing                               */
  /*                                                                  */
  static FT_Error
  check_table_dir( SFNT_Header  sfnt,
                   FT_Stream    stream,
                   FT_UShort*   valid )
  {
    FT_Error   error;
    FT_UShort  nn, valid_entries = 0;
    FT_UInt    has_head = 0, has_sing = 0, has_meta = 0;
    FT_ULong   offset = sfnt->offset + 12;

    static const FT_Frame_Field  table_dir_entry_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_TableRec

      FT_FRAME_START( 16 ),
        FT_FRAME_ULONG( Tag ),
        FT_FRAME_ULONG( CheckSum ),
        FT_FRAME_ULONG( Offset ),
        FT_FRAME_ULONG( Length ),
      FT_FRAME_END
    };


    if ( FT_STREAM_SEEK( offset ) )
      goto Exit;

    for ( nn = 0; nn < sfnt->num_tables; nn++ )
    {
      TT_TableRec  table;


      if ( FT_STREAM_READ_FIELDS( table_dir_entry_fields, &table ) )
      {
        FT_TRACE2(( "check_table_dir:"
                    " can read only %hu table%s in font (instead of %hu)\n",
                    nn, nn == 1 ? "" : "s", sfnt->num_tables ));
        sfnt->num_tables = nn;
        break;
      }

      /* we ignore invalid tables */

      if ( table.Offset > stream->size )
      {
        FT_TRACE2(( "check_table_dir: table entry %hu invalid\n", nn ));
        continue;
      }
      else if ( table.Length > stream->size - table.Offset )
      {
        /* Some tables have such a simple structure that clipping its     */
        /* contents is harmless.  This also makes FreeType less sensitive */
        /* to invalid table lengths (which programs like Acroread seem to */
        /* ignore in general).                                            */

        if ( table.Tag == TTAG_hmtx ||
             table.Tag == TTAG_vmtx )
          valid_entries++;
        else
        {
          FT_TRACE2(( "check_table_dir: table entry %hu invalid\n", nn ));
          continue;
        }
      }
      else
        valid_entries++;

      if ( table.Tag == TTAG_head || table.Tag == TTAG_bhed )
      {
        FT_UInt32  magic;


#ifndef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
        if ( table.Tag == TTAG_head )
#endif
          has_head = 1;

        /*
         * The table length should be 0x36, but certain font tools make it
         * 0x38, so we will just check that it is greater.
         *
         * Note that according to the specification, the table must be
         * padded to 32-bit lengths, but this doesn't apply to the value of
         * its `Length' field!
         *
         */
        if ( table.Length < 0x36 )
        {
          FT_TRACE2(( "check_table_dir:"
                      " `head' or `bhed' table too small\n" ));
          error = FT_THROW( Table_Missing );
          goto Exit;
        }

        if ( FT_STREAM_SEEK( table.Offset + 12 ) ||
             FT_READ_ULONG( magic )              )
          goto Exit;

        if ( magic != 0x5F0F3CF5UL )
          FT_TRACE2(( "check_table_dir:"
                      " invalid magic number in `head' or `bhed' table\n"));

        if ( FT_STREAM_SEEK( offset + ( nn + 1 ) * 16 ) )
          goto Exit;
      }
      else if ( table.Tag == TTAG_SING )
        has_sing = 1;
      else if ( table.Tag == TTAG_META )
        has_meta = 1;
    }

    *valid = valid_entries;

    if ( !valid_entries )
    {
      FT_TRACE2(( "check_table_dir: no valid tables found\n" ));
      error = FT_THROW( Unknown_File_Format );
      goto Exit;
    }

    /* if `sing' and `meta' tables are present, there is no `head' table */
    if ( has_head || ( has_sing && has_meta ) )
    {
      error = FT_Err_Ok;
      goto Exit;
    }
    else
    {
      FT_TRACE2(( "check_table_dir:" ));
#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
      FT_TRACE2(( " neither `head', `bhed', nor `sing' table found\n" ));
#else
      FT_TRACE2(( " neither `head' nor `sing' table found\n" ));
#endif
      error = FT_THROW( Table_Missing );
    }

  Exit:
    return error;
  }


  /**************************************************************************
   *
   * @Function:
   *   tt_face_load_font_dir
   *
   * @Description:
   *   Loads the header of a SFNT font file.
   *
   * @Input:
   *   face ::
   *     A handle to the target face object.
   *
   *   stream ::
   *     The input stream.
   *
   * @Output:
   *   sfnt ::
   *     The SFNT header.
   *
   * @Return:
   *   FreeType error code.  0 means success.
   *
   * @Note:
   *   The stream cursor must be at the beginning of the font directory.
   */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_font_dir( TT_Face    face,
