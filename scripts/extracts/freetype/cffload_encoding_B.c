    {
      /* We take into account the fact a CFF font can use a predefined */
      /* encoding without containing all of the glyphs encoded by this */
      /* encoding (see the note at the end of section 12 in the CFF    */
      /* specification).                                               */

      switch ( (FT_UInt)offset )
      {
      case 0:
        /* First, copy the code to SID mapping. */
        FT_ARRAY_COPY( encoding->sids, cff_standard_encoding, 256 );
        goto Populate;

      case 1:
        /* First, copy the code to SID mapping. */
        FT_ARRAY_COPY( encoding->sids, cff_expert_encoding, 256 );

      Populate:
        /* Construct code to GID mapping from code to SID mapping */
        /* and charset.                                           */

        encoding->offset = offset; /* used in cff_face_init */
        encoding->count  = 0;

        error = cff_charset_compute_cids( charset, num_glyphs,
                                          stream->memory );
        if ( error )
          goto Exit;

        for ( j = 0; j < 256; j++ )
        {
          FT_UInt  sid = encoding->sids[j];
          FT_UInt  gid = 0;


          if ( sid )
            gid = cff_charset_cid_to_gindex( charset, sid );

          if ( gid != 0 )
          {
            encoding->codes[j] = (FT_UShort)gid;
            encoding->count    = j + 1;
          }
          else
          {
            encoding->codes[j] = 0;
            encoding->sids [j] = 0;
          }
        }
        break;

      default:
        FT_ERROR(( "cff_encoding_load: invalid table format\n" ));
        error = FT_THROW( Invalid_File_Format );
        goto Exit;
      }
    }

  Exit:

    /* Clean up if there was an error. */
    return error;
  }


  /* Parse private dictionary; first call is always from `cff_face_init', */
  /* so NDV has not been set for CFF2 variation.                          */
  /*                                                                      */
  /* `cff_slot_load' must call this function each time NDV changes.       */
  FT_LOCAL_DEF( FT_Error )
  cff_load_private_dict( CFF_Font     font,
                         CFF_SubFont  subfont,
                         FT_UInt      lenNDV,
                         FT_Fixed*    NDV )
  {
    FT_Error         error  = FT_Err_Ok;
    CFF_ParserRec    parser;
    CFF_FontRecDict  top    = &subfont->font_dict;
    CFF_Private      priv   = &subfont->private_dict;
    FT_Stream        stream = font->stream;
    FT_UInt          stackSize;


    /* store handle needed to access memory, vstore for blend;    */
    /* we need this for clean-up even if there is no private DICT */
    subfont->blend.font   = font;
    subfont->blend.usedBV = FALSE;  /* clear state */

    if ( !top->private_offset || !top->private_size )
      goto Exit2;       /* no private DICT, do nothing */

    /* set defaults */
    FT_ZERO( priv );

    priv->blue_shift       = 7;
    priv->blue_fuzz        = 1;
    priv->lenIV            = -1;
    priv->expansion_factor = (FT_Fixed)( 0.06 * 0x10000L );
    priv->blue_scale       = (FT_Fixed)( 0.039625 * 0x10000L * 1000 );

