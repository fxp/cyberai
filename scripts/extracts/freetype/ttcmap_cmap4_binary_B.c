          }

          /* end, start, delta, and offset are for the i'th segment */
          if ( mid != i )
          {
            p      = cmap->data + 14 + mid * 2;
            end    = TT_PEEK_USHORT( p );
            p     += 2 + num_segs2;
            start  = TT_PEEK_USHORT( p );
            p     += num_segs2;
            delta  = TT_PEEK_SHORT( p );
            p     += num_segs2;
            offset = TT_PEEK_USHORT( p );
          }
        }
        else
        {
          if ( offset == 0xFFFFU )
            break;
        }

        if ( offset )
        {
          p += offset + ( charcode - start ) * 2;

          /* if p > limit, the whole segment is invalid */
          if ( next && p > limit )
            break;

          gindex = TT_PEEK_USHORT( p );
          if ( gindex )
          {
            gindex = (FT_UInt)( (FT_Int)gindex + delta ) & 0xFFFFU;
            if ( gindex >= (FT_UInt)face->root.num_glyphs )
              gindex = 0;
          }
        }
        else
        {
          gindex = (FT_UInt)( (FT_Int)charcode + delta ) & 0xFFFFU;

          if ( next && gindex >= (FT_UInt)face->root.num_glyphs )
          {
            /* we have an invalid glyph index; if there is an overflow, */
            /* we can adjust `charcode', otherwise the whole segment is */
            /* invalid                                                  */
            gindex = 0;

            if ( (FT_Int)charcode + delta < 0 &&
                 (FT_Int)end + delta >= 0     )
              charcode = (FT_UInt)( -delta );

            else if ( (FT_Int)charcode + delta < 0x10000L &&
                      (FT_Int)end + delta >= 0x10000L     )
              charcode = (FT_UInt)( 0x10000L - delta );
          }
        }

        break;
      }
    }
    while ( min < max );

    if ( next )
    {
      TT_CMap4  cmap4 = (TT_CMap4)cmap;


      /* if `charcode' is not in any segment, then `mid' is */
      /* the segment nearest to `charcode'                  */

      if ( charcode > end && ++mid == num_segs )
        return 0;

      if ( tt_cmap4_set_range( cmap4, mid ) )
      {
        if ( gindex )
          *pcharcode = charcode;
      }
      else
      {
        cmap4->cur_charcode = charcode;

        if ( gindex )
          cmap4->cur_gindex = gindex;
        else
        {
          tt_cmap4_next( cmap4 );
          gindex = cmap4->cur_gindex;
        }

        if ( gindex )
          *pcharcode = cmap4->cur_charcode;
      }
    }

    return gindex;
  }


  FT_CALLBACK_DEF( FT_UInt )
  tt_cmap4_char_index( FT_CMap    cmap,       /* TT_CMap */
                       FT_UInt32  char_code )
