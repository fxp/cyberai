  {
    FT_Error  error;


    error = face->goto_table( face, TTAG_cmap, stream, &face->cmap_size );
    if ( error )
      goto Exit;

    if ( FT_FRAME_EXTRACT( face->cmap_size, face->cmap_table ) )
      face->cmap_size = 0;

  Exit:
    return error;
  }



  /**************************************************************************
   *
   * @Function:
   *   tt_face_load_os2
   *
   * @Description:
   *   Loads the OS2 table.
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
  tt_face_load_os2( TT_Face    face,
                    FT_Stream  stream )
  {
    FT_Error  error;
    TT_OS2*   os2;

    static const FT_Frame_Field  os2_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_OS2

      FT_FRAME_START( 78 ),
        FT_FRAME_USHORT( version ),
        FT_FRAME_SHORT ( xAvgCharWidth ),
        FT_FRAME_USHORT( usWeightClass ),
        FT_FRAME_USHORT( usWidthClass ),
        FT_FRAME_SHORT ( fsType ),
        FT_FRAME_SHORT ( ySubscriptXSize ),
        FT_FRAME_SHORT ( ySubscriptYSize ),
        FT_FRAME_SHORT ( ySubscriptXOffset ),
        FT_FRAME_SHORT ( ySubscriptYOffset ),
        FT_FRAME_SHORT ( ySuperscriptXSize ),
        FT_FRAME_SHORT ( ySuperscriptYSize ),
        FT_FRAME_SHORT ( ySuperscriptXOffset ),
        FT_FRAME_SHORT ( ySuperscriptYOffset ),
        FT_FRAME_SHORT ( yStrikeoutSize ),
        FT_FRAME_SHORT ( yStrikeoutPosition ),
        FT_FRAME_SHORT ( sFamilyClass ),
        FT_FRAME_BYTE  ( panose[0] ),
        FT_FRAME_BYTE  ( panose[1] ),
        FT_FRAME_BYTE  ( panose[2] ),
        FT_FRAME_BYTE  ( panose[3] ),
        FT_FRAME_BYTE  ( panose[4] ),
        FT_FRAME_BYTE  ( panose[5] ),
        FT_FRAME_BYTE  ( panose[6] ),
        FT_FRAME_BYTE  ( panose[7] ),
        FT_FRAME_BYTE  ( panose[8] ),
        FT_FRAME_BYTE  ( panose[9] ),
        FT_FRAME_ULONG ( ulUnicodeRange1 ),
        FT_FRAME_ULONG ( ulUnicodeRange2 ),
        FT_FRAME_ULONG ( ulUnicodeRange3 ),
        FT_FRAME_ULONG ( ulUnicodeRange4 ),
        FT_FRAME_BYTE  ( achVendID[0] ),
        FT_FRAME_BYTE  ( achVendID[1] ),
        FT_FRAME_BYTE  ( achVendID[2] ),
        FT_FRAME_BYTE  ( achVendID[3] ),

        FT_FRAME_USHORT( fsSelection ),
        FT_FRAME_USHORT( usFirstCharIndex ),
        FT_FRAME_USHORT( usLastCharIndex ),
        FT_FRAME_SHORT ( sTypoAscender ),
        FT_FRAME_SHORT ( sTypoDescender ),
        FT_FRAME_SHORT ( sTypoLineGap ),
        FT_FRAME_USHORT( usWinAscent ),
        FT_FRAME_USHORT( usWinDescent ),
      FT_FRAME_END
    };

    /* `OS/2' version 1 and newer */
    static const FT_Frame_Field  os2_fields_extra1[] =
    {
      FT_FRAME_START( 8 ),
        FT_FRAME_ULONG( ulCodePageRange1 ),
        FT_FRAME_ULONG( ulCodePageRange2 ),
      FT_FRAME_END
    };

    /* `OS/2' version 2 and newer */
    static const FT_Frame_Field  os2_fields_extra2[] =
    {
      FT_FRAME_START( 10 ),
        FT_FRAME_SHORT ( sxHeight ),
        FT_FRAME_SHORT ( sCapHeight ),
