        FT_FRAME_USHORT( usDefaultChar ),
        FT_FRAME_USHORT( usBreakChar ),
        FT_FRAME_USHORT( usMaxContext ),
      FT_FRAME_END
    };

    /* `OS/2' version 5 and newer */
    static const FT_Frame_Field  os2_fields_extra5[] =
    {
      FT_FRAME_START( 4 ),
        FT_FRAME_USHORT( usLowerOpticalPointSize ),
        FT_FRAME_USHORT( usUpperOpticalPointSize ),
      FT_FRAME_END
    };


    /* We now support old Mac fonts where the OS/2 table doesn't  */
    /* exist.  Simply put, we set the `version' field to 0xFFFF   */
    /* and test this value each time we need to access the table. */
    error = face->goto_table( face, TTAG_OS2, stream, 0 );
    if ( error )
      goto Exit;

    os2 = &face->os2;

    if ( FT_STREAM_READ_FIELDS( os2_fields, os2 ) )
      goto Exit;

    os2->ulCodePageRange1        = 0;
    os2->ulCodePageRange2        = 0;
    os2->sxHeight                = 0;
    os2->sCapHeight              = 0;
    os2->usDefaultChar           = 0;
    os2->usBreakChar             = 0;
    os2->usMaxContext            = 0;
    os2->usLowerOpticalPointSize = 0;
    os2->usUpperOpticalPointSize = 0xFFFF;

    if ( os2->version >= 0x0001 )
    {
      /* only version 1 tables */
      if ( FT_STREAM_READ_FIELDS( os2_fields_extra1, os2 ) )
        goto Exit;

      if ( os2->version >= 0x0002 )
      {
        /* only version 2 tables */
        if ( FT_STREAM_READ_FIELDS( os2_fields_extra2, os2 ) )
          goto Exit;

        if ( os2->version >= 0x0005 )
        {
          /* only version 5 tables */
          if ( FT_STREAM_READ_FIELDS( os2_fields_extra5, os2 ) )
            goto Exit;
        }
      }
    }

    FT_TRACE3(( "sTypoAscender:  %4hd\n",   os2->sTypoAscender ));
    FT_TRACE3(( "sTypoDescender: %4hd\n",   os2->sTypoDescender ));
    FT_TRACE3(( "usWinAscent:    %4hu\n",   os2->usWinAscent ));
    FT_TRACE3(( "usWinDescent:   %4hu\n",   os2->usWinDescent ));
    FT_TRACE3(( "fsSelection:    0x%2hx\n", os2->fsSelection ));

  Exit:
    return error;
  }


  /**************************************************************************
   *
   * @Function:
   *   tt_face_load_postscript
   *
   * @Description:
   *   Loads the Postscript table.
   *
   * @Input:
   *   face ::
   *     A handle to the target face object.
   *
   *   stream ::
   *     A handle to the input stream.
   *
   * @Return:
   *   FreeType error code.  0 means success.
   */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_post( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error        error;
    TT_Postscript*  post = &face->postscript;

    static const FT_Frame_Field  post_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_Postscript

      FT_FRAME_START( 32 ),
        FT_FRAME_LONG ( FormatType ),
        FT_FRAME_LONG ( italicAngle ),
        FT_FRAME_SHORT( underlinePosition ),
        FT_FRAME_SHORT( underlineThickness ),
        FT_FRAME_ULONG( isFixedPitch ),
        FT_FRAME_ULONG( minMemType42 ),
        FT_FRAME_ULONG( maxMemType42 ),
        FT_FRAME_ULONG( minMemType1 ),
        FT_FRAME_ULONG( maxMemType1 ),
      FT_FRAME_END
    };


    error = face->goto_table( face, TTAG_post, stream, 0 );
    if ( error )
      return error;

    if ( FT_STREAM_READ_FIELDS( post_fields, post ) )
      return error;

    if ( post->FormatType != 0x00030000L &&
         post->FormatType != 0x00025000L &&
         post->FormatType != 0x00020000L &&
         post->FormatType != 0x00010000L )
      return FT_THROW( Invalid_Post_Table_Format );

    /* we don't load the glyph names, we do that in another */
    /* module (ttpost).                                     */

    FT_TRACE3(( "FormatType:   0x%lx\n", post->FormatType ));
    FT_TRACE3(( "isFixedPitch:   %s\n", post->isFixedPitch
                                        ? "  yes" : "   no" ));

    return FT_Err_Ok;
  }


  /**************************************************************************
   *
   * @Function:
   *   tt_face_load_pclt
   *
   * @Description:
   *   Loads the PCL 5 Table.
   *
   * @Input:
   *   face ::
   *     A handle to the target face object.
   *
   *   stream ::
   *     A handle to the input stream.
   *
   * @Return:
   *   FreeType error code.  0 means success.
   */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_pclt( TT_Face    face,
