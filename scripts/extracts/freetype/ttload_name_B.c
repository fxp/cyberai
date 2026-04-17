          {
            /* invalid entry; ignore it */
            entry->stringLength = 0;
          }

          /* mark the string as not yet loaded */
          entry->string = NULL;
        }

        table->langTags = langTags;
        langTags        = NULL;
      }

      FT_FRAME_EXIT();

      (void)FT_STREAM_SEEK( table_pos + 6 );
    }

    /* allocate name records array */
    if ( FT_QNEW_ARRAY( names, table->numNameRecords ) ||
         FT_FRAME_ENTER( table->numNameRecords * 12 )  )
      goto Exit;

    /* load name records */
    {
      TT_Name  entry = names;
      FT_UInt  count = table->numNameRecords;
      FT_UInt  valid = 0;


      for ( ; count > 0; count-- )
      {
        if ( FT_STREAM_READ_FIELDS( name_record_fields, entry ) )
          continue;

        /* check that the name is not empty */
        if ( entry->stringLength == 0 )
          continue;

        /* check that the name string is within the table */
        entry->stringOffset += table_pos + table->storageOffset;
        if ( entry->stringOffset                       < storage_start ||
             entry->stringOffset + entry->stringLength > storage_limit )
        {
          /* invalid entry; ignore it */
          continue;
        }

        /* assure that we have a valid language tag ID, and   */
        /* that the corresponding langTag entry is valid, too */
        if ( table->format == 1 && entry->languageID >= 0x8000U )
        {
          if ( entry->languageID - 0x8000U >= table->numLangTagRecords    ||
               !table->langTags[entry->languageID - 0x8000U].stringLength )
          {
            /* invalid entry; ignore it */
            continue;
          }
        }

        /* mark the string as not yet converted */
        entry->string = NULL;

        valid++;
        entry++;
      }

      /* reduce array size to the actually used elements */
      FT_MEM_QRENEW_ARRAY( names,
                           table->numNameRecords,
                           valid );
      table->names          = names;
      names                 = NULL;
      table->numNameRecords = valid;
    }

    FT_FRAME_EXIT();

    /* everything went well, update face->num_names */
    face->num_names = (FT_UShort)table->numNameRecords;

  Exit:
    FT_FREE( names );
    FT_FREE( langTags );
    return error;
  }


  /**************************************************************************
   *
   * @Function:
   *   tt_face_free_name
   *
   * @Description:
   *   Frees the name records.
   *
   * @Input:
   *   face ::
   *     A handle to the target face object.
   */
  FT_LOCAL_DEF( void )
