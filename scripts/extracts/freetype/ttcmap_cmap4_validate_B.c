          {
            if ( p < glyph_ids                                ||
                 p + ( end - start + 1 ) * 2 > table + length )
              FT_INVALID_DATA;
          }
          /* Some fonts handle the last segment incorrectly.  In */
          /* theory, 0xFFFF might point to an ordinary glyph --  */
          /* a cmap 4 is versatile and could be used for any     */
          /* encoding, not only Unicode.  However, reality shows */
          /* that far too many fonts are sloppy and incorrectly  */
          /* set all fields but `start' and `end' for the last   */
          /* segment if it contains only a single character.     */
          /*                                                     */
          /* We thus omit the test here, delaying it to the      */
          /* routines that actually access the cmap.             */
          else if ( n != num_segs - 1                       ||
                    !( start == 0xFFFFU && end == 0xFFFFU ) )
          {
            if ( p < glyph_ids                              ||
                 p + ( end - start + 1 ) * 2 > valid->limit )
              FT_INVALID_DATA;
          }

          /* check glyph indices within the segment range */
          if ( valid->level >= FT_VALIDATE_TIGHT )
          {
            FT_UInt  i, idx;


            for ( i = start; i < end; i++ )
            {
              idx = FT_NEXT_USHORT( p );
              if ( idx != 0 )
              {
                idx = (FT_UInt)( (FT_Int)idx + delta ) & 0xFFFFU;

                if ( idx >= TT_VALID_GLYPH_COUNT( valid ) )
                  FT_INVALID_GLYPH_ID;
              }
            }
          }
        }
        else if ( offset == 0xFFFFU )
        {
          /* some fonts (erroneously?) use a range offset of 0xFFFF */
          /* to mean missing glyph in cmap table                    */
          /*                                                        */
          if ( valid->level >= FT_VALIDATE_PARANOID    ||
               n != num_segs - 1                       ||
               !( start == 0xFFFFU && end == 0xFFFFU ) )
            FT_INVALID_DATA;
        }

        last_start = start;
        last_end   = end;
      }
    }

    return error;
  }


  static FT_UInt
  tt_cmap4_char_map_linear( TT_CMap     cmap,
                            FT_UInt32*  pcharcode,
                            FT_Bool     next )
  {
    TT_Face   face  = (TT_Face)FT_CMAP_FACE( cmap );
    FT_Byte*  limit = face->cmap_table + face->cmap_size;


    FT_UInt    num_segs2, start, end, offset;
    FT_Int     delta;
    FT_UInt    i, num_segs;
    FT_UInt32  charcode = *pcharcode + next;
    FT_UInt    gindex   = 0;
    FT_Byte*   p;
    FT_Byte*   q;


    p = cmap->data + 6;
    num_segs = TT_PEEK_USHORT( p ) >> 1;

    if ( !num_segs )
      return 0;

    num_segs2 = num_segs << 1;

    /* linear search */
    p = cmap->data + 14;               /* ends table   */
    q = cmap->data + 16 + num_segs2;   /* starts table */

    for ( i = 0; i < num_segs; i++ )
    {
      end   = TT_NEXT_USHORT( p );
      start = TT_NEXT_USHORT( q );

      if ( charcode < start )
      {
        if ( next )
          charcode = start;
        else
          break;
      }

    Again:
      if ( charcode <= end )
      {
        FT_Byte*  r;


        r       = q - 2 + num_segs2;
        delta   = TT_PEEK_SHORT( r );
        r      += num_segs2;
        offset  = TT_PEEK_USHORT( r );

        /* some fonts have an incorrect last segment; */
        /* we have to catch it                        */
        if ( i >= num_segs - 1                  &&
             start == 0xFFFFU && end == 0xFFFFU )
