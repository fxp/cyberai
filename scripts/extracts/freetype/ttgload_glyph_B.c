          glyph->linearHoriAdvance = loader.linear;
          glyph->linearVertAdvance = loader.vadvance;

          /* Bitmaps from the 'sbix' table need special treatment:  */
          /* if there is a glyph contour, the bitmap origin must be */
          /* shifted to be relative to the lower left corner of the */
          /* glyph bounding box, also taking the left-side bearing  */
          /* (or top bearing) into account.                         */
          if ( face->sbit_table_type == TT_SBIT_TABLE_TYPE_SBIX &&
               loader.n_contours > 0                            )
          {
            FT_Int  bitmap_left;
            FT_Int  bitmap_top;


            if ( load_flags & FT_LOAD_VERTICAL_LAYOUT )
            {
              /* This is a guess, since Apple's CoreText engine doesn't */
              /* really do vertical typesetting.                        */
              bitmap_left = loader.bbox.xMin;
              bitmap_top  = loader.top_bearing;
            }
            else
            {
              bitmap_left = loader.left_bearing;
              bitmap_top  = loader.bbox.yMin;
            }

            glyph->bitmap_left += FT_MulFix( bitmap_left, x_scale ) >> 6;
            glyph->bitmap_top  += FT_MulFix( bitmap_top,  y_scale ) >> 6;
          }

          /* sanity checks: if `xxxAdvance' in the sbit metric */
          /* structure isn't set, use `linearXXXAdvance'      */
          if ( !glyph->metrics.horiAdvance && glyph->linearHoriAdvance )
            glyph->metrics.horiAdvance = FT_MulFix( glyph->linearHoriAdvance,
                                                    x_scale );
          if ( !glyph->metrics.vertAdvance && glyph->linearVertAdvance )
            glyph->metrics.vertAdvance = FT_MulFix( glyph->linearVertAdvance,
                                                    y_scale );
        }

        return FT_Err_Ok;
      }
    }

    if ( load_flags & FT_LOAD_SBITS_ONLY )
    {
      error = FT_THROW( Invalid_Argument );
      goto Exit;
    }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    /* if FT_LOAD_NO_SCALE is not set, `ttmetrics' must be valid */
    if ( !( load_flags & FT_LOAD_NO_SCALE ) && !size->ttmetrics.valid )
    {
      error = FT_THROW( Invalid_Size_Handle );
      goto Exit;
    }

#ifdef FT_CONFIG_OPTION_SVG

    /* check for OT-SVG */
    if ( ( load_flags & FT_LOAD_NO_SVG ) == 0 &&
         ( load_flags & FT_LOAD_COLOR )       &&
         face->svg                            )
    {
      SFNT_Service  sfnt = (SFNT_Service)face->sfnt;


      FT_TRACE3(( "Trying to load SVG glyph\n" ));

      error = sfnt->load_svg_doc( glyph, glyph_index );
      if ( !error )
      {
        FT_Fixed  x_scale = size->root.metrics.x_scale;
        FT_Fixed  y_scale = size->root.metrics.y_scale;

        FT_Short   leftBearing;
        FT_Short   topBearing;
        FT_UShort  advanceX;
        FT_UShort  advanceY;


        FT_TRACE3(( "Successfully loaded SVG glyph\n" ));

        glyph->format = FT_GLYPH_FORMAT_SVG;

        sfnt->get_metrics( face,
