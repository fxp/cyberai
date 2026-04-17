                                            : -subglyph->transform.yx;
        int  c = subglyph->transform.xy > 0 ?  subglyph->transform.xy
                                            : -subglyph->transform.xy;
        int  d = subglyph->transform.yy > 0 ?  subglyph->transform.yy
                                            : -subglyph->transform.yy;
        int  m = a > b ? a : b;
        int  n = c > d ? c : d;


        if ( a - b <= 33 && a - b >= -33 )
          m *= 2;
        if ( c - d <= 33 && c - d >= -33 )
          n *= 2;
        x = FT_MulFix( x, m );
        y = FT_MulFix( y, n );

#else /* 1 */

        /********************************************************************
         *
         * This algorithm is a guess and works much better than the above.
         */
        FT_Fixed  mac_xscale = FT_Hypot( subglyph->transform.xx,
                                         subglyph->transform.xy );
        FT_Fixed  mac_yscale = FT_Hypot( subglyph->transform.yy,
                                         subglyph->transform.yx );


        x = FT_MulFix( x, mac_xscale );
        y = FT_MulFix( y, mac_yscale );

#endif /* 1 */

      }

      if ( !( loader->load_flags & FT_LOAD_NO_SCALE ) )
      {
        FT_Fixed  x_scale = loader->size->metrics->x_scale;
        FT_Fixed  y_scale = loader->size->metrics->y_scale;


        x = FT_MulFix( x, x_scale );
        y = FT_MulFix( y, y_scale );

        if ( subglyph->flags & ROUND_XY_TO_GRID )
        {
          TT_Face    face   = loader->face;
          TT_Driver  driver = (TT_Driver)FT_FACE_DRIVER( face );


          if ( IS_HINTED( loader->load_flags ) )
          {
            /*
             * We round the horizontal offset only if there is hinting along
             * the x axis; this corresponds to integer advance width values.
             *
             * Theoretically, a glyph's bytecode can toggle ClearType's
             * `backward compatibility' mode, which would allow modification
             * of the advance width.  In reality, however, applications
             * neither allow nor expect modified advance widths if subpixel
             * rendering is active.
             *
             */
            if ( driver->interpreter_version == TT_INTERPRETER_VERSION_35 )
              x = FT_PIX_ROUND( x );

            y = FT_PIX_ROUND( y );
          }
        }
      }
    }

    if ( x || y )
      FT_Outline_Translate( &current, x, y );

    return FT_Err_Ok;
  }


  /**************************************************************************
   *
   * @Function:
   *   TT_Process_Composite_Glyph
   *
   * @Description:
   *   This is slightly different from TT_Process_Simple_Glyph, in that
   *   its sole purpose is to hint the glyph.  Thus this function is
   *   only available when bytecode interpreter is enabled.
   */
  static FT_Error
