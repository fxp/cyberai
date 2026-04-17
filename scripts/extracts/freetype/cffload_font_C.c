    /* now, check for a CID or CFF2 font */
    if ( dict->cid_registry != 0xFFFFU ||
         cff2                          )
    {
      CFF_IndexRec  fd_index;
      CFF_SubFont   sub = NULL;
      FT_UInt       idx;


      /* for CFF2, read the Variation Store if available;                 */
      /* this must follow the Top DICT parse and precede any Private DICT */
      error = cff_vstore_load( &font->vstore,
                               stream,
                               base_offset,
                               dict->vstore_offset );
      if ( error )
        goto Exit;

      /* this is a CID-keyed font, we must now allocate a table of */
      /* sub-fonts, then load each of them separately              */
      if ( FT_STREAM_SEEK( base_offset + dict->cid_fd_array_offset ) )
        goto Exit;

      error = cff_index_init( &fd_index, stream, 0, cff2 );
      if ( error )
        goto Exit;

      /* Font Dicts are not limited to 256 for CFF2. */
      /* TODO: support this for CFF2                 */
      if ( fd_index.count > CFF_MAX_CID_FONTS )
      {
        FT_TRACE0(( "cff_font_load: FD array too large in CID font\n" ));
        goto Fail_CID;
      }

      /* allocate & read each font dict independently */
      font->num_subfonts = fd_index.count;
      if ( FT_NEW_ARRAY( sub, fd_index.count ) )
        goto Fail_CID;

      /* set up pointer table */
      for ( idx = 0; idx < fd_index.count; idx++ )
        font->subfonts[idx] = sub + idx;

      /* now load each subfont independently */
      for ( idx = 0; idx < fd_index.count; idx++ )
      {
        sub = font->subfonts[idx];
        FT_TRACE4(( "parsing subfont %u\n", idx ));
        error = cff_subfont_load( sub,
                                  &fd_index,
                                  idx,
                                  stream,
                                  base_offset,
                                  cff2 ? CFF2_CODE_FONTDICT
                                       : CFF_CODE_TOPDICT,
                                  font,
                                  face );
        if ( error )
          goto Fail_CID;
      }

      /* now load the FD Select array;               */
      /* CFF2 omits FDSelect if there is only one FD */
      if ( !cff2 || fd_index.count > 1 )
        error = CFF_Load_FD_Select( &font->fd_select,
                                    font->charstrings_index.count,
                                    stream,
                                    base_offset + dict->cid_fd_select_offset );

    Fail_CID:
      cff_index_done( &fd_index );

      if ( error )
        goto Exit;
    }
    else
      font->num_subfonts = 0;

    /* read the charstrings index now */
    if ( dict->charstrings_offset == 0 )
    {
      FT_ERROR(( "cff_font_load: no charstrings offset\n" ));
      error = FT_THROW( Invalid_File_Format );
      goto Exit;
    }

    font->num_glyphs = font->charstrings_index.count;

    error = cff_index_get_pointers( &font->global_subrs_index,
                                    &font->global_subrs, NULL, NULL );

    if ( error )
      goto Exit;

    /* read the Charset and Encoding tables if available */
    if ( !cff2 && font->num_glyphs > 0 )
    {
      FT_Bool  invert = FT_BOOL( dict->cid_registry != 0xFFFFU && pure_cff );


      error = cff_charset_load( &font->charset, font->num_glyphs, stream,
                                base_offset, dict->charset_offset, invert );
      if ( error )
        goto Exit;

      /* CID-keyed CFFs don't have an encoding */
      if ( dict->cid_registry == 0xFFFFU )
      {
        error = cff_encoding_load( &font->encoding,
                                   &font->charset,
                                   font->num_glyphs,
                                   stream,
                                   base_offset,
                                   dict->encoding_offset );
        if ( error )
          goto Exit;
      }
    }

    /* get the font name (/CIDFontName for CID-keyed fonts, */
    /* /FontName otherwise)                                 */
    font->font_name = cff_index_get_name( font, subfont_index );

  Exit:
    cff_index_done( &string_index );

    return error;
  }


  FT_LOCAL_DEF( void )
  cff_font_done( CFF_Font  font )
  {
