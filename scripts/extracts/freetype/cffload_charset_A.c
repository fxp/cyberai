  {
    FT_Memory  memory = stream->memory;
    FT_Error   error  = FT_Err_Ok;
    FT_UShort  glyph_sid;


    /* If the offset is greater than 2, we have to parse the charset */
    /* table.                                                        */
    if ( offset > 2 )
    {
      FT_UInt  j;


      charset->offset = base_offset + offset;

      /* Get the format of the table. */
      if ( FT_STREAM_SEEK( charset->offset ) ||
           FT_READ_BYTE( charset->format )   )
        goto Exit;

      /* Allocate memory for sids. */
      if ( FT_QNEW_ARRAY( charset->sids, num_glyphs ) )
        goto Exit;

      /* assign the .notdef glyph */
      charset->sids[0] = 0;

      switch ( charset->format )
      {
      case 0:
        if ( num_glyphs > 0 )
        {
          if ( FT_FRAME_ENTER( ( num_glyphs - 1 ) * 2 ) )
            goto Exit;

          for ( j = 1; j < num_glyphs; j++ )
            charset->sids[j] = FT_GET_USHORT();

          FT_FRAME_EXIT();
        }
        break;

      case 1:
      case 2:
        {
          FT_UInt  nleft;
          FT_UInt  i;


          j = 1;

          while ( j < num_glyphs )
          {
            /* Read the first glyph sid of the range. */
            if ( FT_READ_USHORT( glyph_sid ) )
              goto Exit;

            /* Read the number of glyphs in the range.  */
            if ( charset->format == 2 )
            {
              if ( FT_READ_USHORT( nleft ) )
                goto Exit;
            }
            else
            {
              if ( FT_READ_BYTE( nleft ) )
                goto Exit;
            }

            /* try to rescue some of the SIDs if `nleft' is too large */
            if ( glyph_sid > 0xFFFFL - nleft )
            {
              FT_ERROR(( "cff_charset_load: invalid SID range trimmed"
                         " nleft=%d -> %ld\n", nleft, 0xFFFFL - glyph_sid ));
              nleft = ( FT_UInt )( 0xFFFFL - glyph_sid );
            }

            /* Fill in the range of sids -- `nleft + 1' glyphs. */
            for ( i = 0; j < num_glyphs && i <= nleft; i++, j++, glyph_sid++ )
              charset->sids[j] = glyph_sid;
          }
        }
        break;

      default:
        FT_ERROR(( "cff_charset_load: invalid table format\n" ));
        error = FT_THROW( Invalid_File_Format );
        goto Exit;
      }
    }
    else
    {
      /* Parse default tables corresponding to offset == 0, 1, or 2.  */
      /* CFF specification intimates the following:                   */
