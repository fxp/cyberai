      {
        xx = (FT_Fixed)FT_NEXT_SHORT( p ) * 4;
        yx = (FT_Fixed)FT_NEXT_SHORT( p ) * 4;
        xy = (FT_Fixed)FT_NEXT_SHORT( p ) * 4;
        yy = (FT_Fixed)FT_NEXT_SHORT( p ) * 4;
      }

      subglyph->transform.xx = xx;
      subglyph->transform.xy = xy;
      subglyph->transform.yx = yx;
      subglyph->transform.yy = yy;

      num_subglyphs++;

    } while ( subglyph->flags & MORE_COMPONENTS );

    gloader->current.num_subglyphs = num_subglyphs;
    FT_TRACE5(( "  %d component%s\n",
                num_subglyphs,
                num_subglyphs > 1 ? "s" : "" ));

#ifdef FT_DEBUG_LEVEL_TRACE
    {
      FT_UInt  i;


      subglyph = gloader->current.subglyphs;

      for ( i = 0; i < num_subglyphs; i++ )
      {
        if ( num_subglyphs > 1 )
          FT_TRACE7(( "    subglyph %d:\n", i ));

        FT_TRACE7(( "      glyph index: %d\n", subglyph->index ));

        if ( subglyph->flags & ARGS_ARE_XY_VALUES )
          FT_TRACE7(( "      offset: x=%d, y=%d\n",
                      subglyph->arg1,
                      subglyph->arg2 ));
        else
          FT_TRACE7(( "      matching points: base=%d, component=%d\n",
                      subglyph->arg1,
                      subglyph->arg2 ));

        if ( subglyph->flags & WE_HAVE_A_SCALE )
          FT_TRACE7(( "      scaling: %f\n",
                      (double)subglyph->transform.xx / 65536 ));
        else if ( subglyph->flags & WE_HAVE_AN_XY_SCALE )
          FT_TRACE7(( "      scaling: x=%f, y=%f\n",
                      (double)subglyph->transform.xx / 65536,
                      (double)subglyph->transform.yy / 65536 ));
        else if ( subglyph->flags & WE_HAVE_A_2X2 )
        {
          FT_TRACE7(( "      scaling: xx=%f, yx=%f\n",
                      (double)subglyph->transform.xx / 65536,
                      (double)subglyph->transform.yx / 65536 ));
          FT_TRACE7(( "               xy=%f, yy=%f\n",
                      (double)subglyph->transform.xy / 65536,
                      (double)subglyph->transform.yy / 65536 ));
        }

        subglyph++;
      }
    }
#endif /* FT_DEBUG_LEVEL_TRACE */

#ifdef TT_USE_BYTECODE_INTERPRETER

    {
      FT_Stream  stream = loader->stream;


      /* we must undo the FT_FRAME_ENTER in order to point */
      /* to the composite instructions, if we find some.   */
      /* We will process them later.                       */
      /*                                                   */
      loader->ins_pos = (FT_ULong)( FT_STREAM_POS() +
                                    p - limit );
    }

#endif

    loader->cursor = p;

  Fail:
    return error;

  Invalid_Composite:
    error = FT_THROW( Invalid_Composite );
    goto Fail;
  }

