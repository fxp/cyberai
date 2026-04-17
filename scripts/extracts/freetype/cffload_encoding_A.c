  {
    FT_Error   error = FT_Err_Ok;
    FT_UInt    count;
    FT_UInt    j;
    FT_UShort  glyph_sid;
    FT_UInt    glyph_code;


    /* Check for charset->sids.  If we do not have this, we fail. */
    if ( !charset->sids )
    {
      error = FT_THROW( Invalid_File_Format );
      goto Exit;
    }

    /* Note: The encoding table in a CFF font is indexed by glyph index;  */
    /* the first encoded glyph index is 1.  Hence, we read the character  */
    /* code (`glyph_code') at index j and make the assignment:            */
    /*                                                                    */
    /*    encoding->codes[glyph_code] = j + 1                             */
    /*                                                                    */
    /* We also make the assignment:                                       */
    /*                                                                    */
    /*    encoding->sids[glyph_code] = charset->sids[j + 1]               */
    /*                                                                    */
    /* This gives us both a code to GID and a code to SID mapping.        */

    if ( offset > 1 )
    {
      /* Zero out the code to gid/sid mappings. */
      FT_ARRAY_ZERO( encoding->sids,  256 );
      FT_ARRAY_ZERO( encoding->codes, 256 );

      encoding->offset = base_offset + offset;

      /* we need to parse the table to determine its size */
      if ( FT_STREAM_SEEK( encoding->offset ) ||
           FT_READ_BYTE( encoding->format )   ||
           FT_READ_BYTE( count )              )
        goto Exit;

      switch ( encoding->format & 0x7F )
      {
      case 0:
        {
          FT_Byte*  p;


          /* By convention, GID 0 is always ".notdef" and is never */
          /* coded in the font.  Hence, the number of codes found  */
          /* in the table is `count+1'.                            */
          /*                                                       */
          encoding->count = count + 1;

          if ( FT_FRAME_ENTER( count ) )
            goto Exit;

          p = (FT_Byte*)stream->cursor;

          for ( j = 1; j <= count; j++ )
          {
            glyph_code = *p++;

            /* Make sure j is not too big. */
            if ( j < num_glyphs )
            {
              /* Assign code to GID mapping. */
              encoding->codes[glyph_code] = (FT_UShort)j;

              /* Assign code to SID mapping. */
              encoding->sids[glyph_code] = charset->sids[j];
            }
          }

          FT_FRAME_EXIT();
        }
        break;

      case 1:
        {
          FT_UInt  nleft;
          FT_UInt  i = 1;
          FT_UInt  k;


          encoding->count = 0;

          /* Parse the Format1 ranges. */
          for ( j = 0;  j < count; j++, i += nleft )
          {
            /* Read the first glyph code of the range. */
            if ( FT_READ_BYTE( glyph_code ) )
              goto Exit;

            /* Read the number of codes in the range. */
            if ( FT_READ_BYTE( nleft ) )
              goto Exit;

            /* Increment nleft, so we read `nleft + 1' codes/sids. */
            nleft++;

            /* compute max number of character codes */
            if ( (FT_UInt)nleft > encoding->count )
              encoding->count = nleft;

            /* Fill in the range of codes/sids. */
            for ( k = i; k < nleft + i; k++, glyph_code++ )
            {
              /* Make sure k is not too big. */
              if ( k < num_glyphs && glyph_code < 256 )
              {
                /* Assign code to GID mapping. */
                encoding->codes[glyph_code] = (FT_UShort)k;

                /* Assign code to SID mapping. */
                encoding->sids[glyph_code] = charset->sids[k];
              }
            }
          }

          /* simple check; one never knows what can be found in a font */
          if ( encoding->count > 256 )
            encoding->count = 256;
        }
        break;

      default:
        FT_ERROR(( "cff_encoding_load: invalid table format\n" ));
        error = FT_THROW( Invalid_File_Format );
        goto Exit;
      }

      /* Parse supplemental encodings, if any. */
      if ( encoding->format & 0x80 )
      {
        FT_UInt  gindex;


        /* count supplements */
        if ( FT_READ_BYTE( count ) )
          goto Exit;

        for ( j = 0; j < count; j++ )
        {
          /* Read supplemental glyph code. */
          if ( FT_READ_BYTE( glyph_code ) )
            goto Exit;

          /* Read the SID associated with this glyph code. */
          if ( FT_READ_USHORT( glyph_sid ) )
            goto Exit;

          /* Assign code to SID mapping. */
          encoding->sids[glyph_code] = glyph_sid;

          /* First, look up GID which has been assigned to */
          /* SID glyph_sid.                                */
          for ( gindex = 0; gindex < num_glyphs; gindex++ )
          {
            if ( charset->sids[gindex] == glyph_sid )
            {
              encoding->codes[glyph_code] = (FT_UShort)gindex;
              break;
            }
          }
        }
      }
    }
    else
