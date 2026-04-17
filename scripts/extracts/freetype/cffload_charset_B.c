      /*                                                              */
      /* In order to use a predefined charset, the following must be  */
      /* true: The charset constructed for the glyphs in the font's   */
      /* charstrings dictionary must match the predefined charset in  */
      /* the first num_glyphs.                                        */

      charset->offset = offset;  /* record charset type */

      switch ( (FT_UInt)offset )
      {
      case 0:
        if ( num_glyphs > 229 )
        {
          FT_ERROR(( "cff_charset_load: implicit charset larger than\n" ));
          FT_ERROR(( "predefined charset (Adobe ISO-Latin)\n" ));
          error = FT_THROW( Invalid_File_Format );
          goto Exit;
        }

        /* Allocate memory for sids. */
        if ( FT_QNEW_ARRAY( charset->sids, num_glyphs ) )
          goto Exit;

        /* Copy the predefined charset into the allocated memory. */
        FT_ARRAY_COPY( charset->sids, cff_isoadobe_charset, num_glyphs );

        break;

      case 1:
        if ( num_glyphs > 166 )
        {
          FT_ERROR(( "cff_charset_load: implicit charset larger than\n" ));
          FT_ERROR(( "predefined charset (Adobe Expert)\n" ));
          error = FT_THROW( Invalid_File_Format );
          goto Exit;
        }

        /* Allocate memory for sids. */
        if ( FT_QNEW_ARRAY( charset->sids, num_glyphs ) )
          goto Exit;

        /* Copy the predefined charset into the allocated memory.     */
        FT_ARRAY_COPY( charset->sids, cff_expert_charset, num_glyphs );

        break;

      case 2:
        if ( num_glyphs > 87 )
        {
          FT_ERROR(( "cff_charset_load: implicit charset larger than\n" ));
          FT_ERROR(( "predefined charset (Adobe Expert Subset)\n" ));
          error = FT_THROW( Invalid_File_Format );
          goto Exit;
        }

        /* Allocate memory for sids. */
        if ( FT_QNEW_ARRAY( charset->sids, num_glyphs ) )
          goto Exit;

        /* Copy the predefined charset into the allocated memory.     */
        FT_ARRAY_COPY( charset->sids, cff_expertsubset_charset, num_glyphs );

        break;

      default:
        error = FT_THROW( Invalid_File_Format );
        goto Exit;
      }
    }

    /* we have to invert the `sids' array for subsetted CID-keyed fonts */
    if ( invert )
      error = cff_charset_compute_cids( charset, num_glyphs, memory );

  Exit:
    /* Clean up if there was an error. */
    if ( error )
    {
      FT_FREE( charset->sids );
      FT_FREE( charset->cids );
      charset->format = 0;
      charset->offset = 0;
    }

    return error;
  }


  static void
  cff_vstore_done( CFF_VStoreRec*  vstore,
                   FT_Memory       memory )
