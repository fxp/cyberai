        case cff_op_seac:
          FT_TRACE4(( " seac\n" ));

          error = cff_operator_seac( decoder,
                                     args[0], args[1], args[2],
                                     (FT_Int)( args[3] >> 16 ),
                                     (FT_Int)( args[4] >> 16 ) );

          /* add current outline to the glyph slot */
          FT_GlyphLoader_Add( builder->loader );

          /* return now! */
          FT_TRACE4(( "\n" ));
          return error;

        case cff_op_endchar:
          /* in dictionaries, `endchar' simply indicates end of data */
          if ( in_dict )
            return error;

          FT_TRACE4(( " endchar\n" ));

          /* We are going to emulate the seac operator. */
          if ( num_args >= 4 )
          {
            /* Save glyph width so that the subglyphs don't overwrite it. */
            FT_Pos  glyph_width = decoder->glyph_width;


            error = cff_operator_seac( decoder,
                                       0L, args[-4], args[-3],
                                       (FT_Int)( args[-2] >> 16 ),
                                       (FT_Int)( args[-1] >> 16 ) );

            decoder->glyph_width = glyph_width;
          }
          else
          {
            cff_builder_close_contour( builder );

            /* close hints recording session */
            if ( hinter )
            {
              if ( hinter->close( hinter->hints,
                                  (FT_UInt)builder->current->n_points ) )
                goto Syntax_Error;

              /* apply hints to the loaded glyph outline now */
              error = hinter->apply( hinter->hints,
                                     builder->current,
                                     (PSH_Globals)builder->hints_globals,
                                     decoder->hint_mode );
              if ( error )
                goto Fail;
            }

            /* add current outline to the glyph slot */
            FT_GlyphLoader_Add( builder->loader );
          }

          /* return now! */
          FT_TRACE4(( "\n" ));
          return error;

        case cff_op_abs:
          FT_TRACE4(( " abs\n" ));

          if ( args[0] < 0 )
          {
            if ( args[0] == FT_LONG_MIN )
              args[0] = FT_LONG_MAX;
            else
              args[0] = -args[0];
          }
          args++;
          break;

        case cff_op_add:
          FT_TRACE4(( " add\n" ));

          args[0] = ADD_LONG( args[0], args[1] );
          args++;
          break;

        case cff_op_sub:
          FT_TRACE4(( " sub\n" ));

          args[0] = SUB_LONG( args[0], args[1] );
          args++;
          break;

        case cff_op_div:
          FT_TRACE4(( " div\n" ));

          args[0] = FT_DivFix( args[0], args[1] );
          args++;
          break;

        case cff_op_neg:
          FT_TRACE4(( " neg\n" ));

          if ( args[0] == FT_LONG_MIN )
            args[0] = FT_LONG_MAX;
          args[0] = -args[0];
          args++;
          break;

        case cff_op_random:
          {
            FT_UInt32*  randval = in_dict ? &decoder->cff->top_font.random
                                          : &decoder->current_subfont->random;


            FT_TRACE4(( " random\n" ));

            /* only use the lower 16 bits of `random'  */
            /* to generate a number in the range (0;1] */
            args[0] = (FT_Fixed)( ( *randval & 0xFFFF ) + 1 );
            args++;

            *randval = cff_random( *randval );
          }
          break;

        case cff_op_mul:
          FT_TRACE4(( " mul\n" ));

          args[0] = FT_MulFix( args[0], args[1] );
          args++;
          break;

        case cff_op_sqrt:
          FT_TRACE4(( " sqrt\n" ));

          /* without upper limit the loop below might not finish */
          if ( args[0] > 0x7FFFFFFFL )
            args[0] = 0xB504F4L;    /* sqrt( 32768.0044 ) */
          else if ( args[0] > 0 )
            args[0] = (FT_Fixed)FT_SqrtFixed( args[0] );
          else
            args[0] = 0;
