        case cff_op_hstem:
        case cff_op_vstem:
        case cff_op_hstemhm:
        case cff_op_vstemhm:
          /* the number of arguments is always even here */
          FT_TRACE4(( "%s\n",
              op == cff_op_hstem   ? " hstem"   :
            ( op == cff_op_vstem   ? " vstem"   :
            ( op == cff_op_hstemhm ? " hstemhm" : " vstemhm" ) ) ));

          if ( hinter )
            hinter->stems( hinter->hints,
                           ( op == cff_op_hstem || op == cff_op_hstemhm ),
                           num_args / 2,
                           args - ( num_args & ~1 ) );

          decoder->num_hints += num_args / 2;
          args = stack;
          break;

        case cff_op_hintmask:
        case cff_op_cntrmask:
          FT_TRACE4(( "%s", op == cff_op_hintmask ? " hintmask"
                                                  : " cntrmask" ));

          /* implement vstem when needed --                        */
          /* the specification doesn't say it, but this also works */
          /* with the 'cntrmask' operator                          */
          /*                                                       */
          if ( num_args > 0 )
          {
            if ( hinter )
              hinter->stems( hinter->hints,
                             0,
                             num_args / 2,
                             args - ( num_args & ~1 ) );

            decoder->num_hints += num_args / 2;
          }

          /* In a valid charstring there must be at least one byte */
          /* after `hintmask' or `cntrmask' (e.g., for a `return'  */
          /* instruction).  Additionally, there must be space for  */
          /* `num_hints' bits.                                     */

          if ( ( ip + ( ( decoder->num_hints + 7 ) >> 3 ) ) >= limit )
            goto Syntax_Error;

          if ( hinter )
          {
            if ( op == cff_op_hintmask )
              hinter->hintmask( hinter->hints,
                                (FT_UInt)builder->current->n_points,
                                (FT_UInt)decoder->num_hints,
                                ip );
            else
              hinter->counter( hinter->hints,
                               (FT_UInt)decoder->num_hints,
                               ip );
          }

#ifdef FT_DEBUG_LEVEL_TRACE
          {
            FT_UInt  maskbyte;


            FT_TRACE4(( " (maskbytes:" ));

            for ( maskbyte = 0;
                  maskbyte < (FT_UInt)( ( decoder->num_hints + 7 ) >> 3 );
                  maskbyte++, ip++ )
              FT_TRACE4(( " 0x%02X", *ip ));

            FT_TRACE4(( ")\n" ));
          }
#else
          ip += ( decoder->num_hints + 7 ) >> 3;
#endif
          args = stack;
          break;

        case cff_op_rmoveto:
          FT_TRACE4(( " rmoveto\n" ));

          cff_builder_close_contour( builder );
          builder->path_begun = 0;
          x    = ADD_LONG( x, args[-2] );
          y    = ADD_LONG( y, args[-1] );
          args = stack;
          break;

        case cff_op_vmoveto:
          FT_TRACE4(( " vmoveto\n" ));

          cff_builder_close_contour( builder );
          builder->path_begun = 0;
          y    = ADD_LONG( y, args[-1] );
          args = stack;
          break;

        case cff_op_hmoveto:
          FT_TRACE4(( " hmoveto\n" ));

          cff_builder_close_contour( builder );
          builder->path_begun = 0;
          x    = ADD_LONG( x, args[-1] );
          args = stack;
          break;

        case cff_op_rlineto:
          FT_TRACE4(( " rlineto\n" ));

          if ( cff_builder_start_point( builder, x, y )  ||
               cff_check_points( builder, num_args / 2 ) )
            goto Fail;

          if ( num_args < 2 )
            goto Stack_Underflow;

          args -= num_args & ~1;
          while ( args < decoder->top )
          {
            x = ADD_LONG( x, args[0] );
            y = ADD_LONG( y, args[1] );
            cff_builder_add_point( builder, x, y, 1 );
            args += 2;
          }
          args = stack;
          break;

        case cff_op_hlineto:
        case cff_op_vlineto:
          {
            FT_Int  phase = ( op == cff_op_hlineto );


            FT_TRACE4(( "%s\n", op == cff_op_hlineto ? " hlineto"
                                                     : " vlineto" ));

            if ( num_args < 0 )
              goto Stack_Underflow;

            /* there exist subsetted fonts (found in PDFs) */
            /* which call `hlineto' without arguments      */
            if ( num_args == 0 )
              break;

            if ( cff_builder_start_point( builder, x, y ) ||
                 cff_check_points( builder, num_args )    )
              goto Fail;

            args = stack;
            while ( args < decoder->top )
            {
              if ( phase )
                x = ADD_LONG( x, args[0] );
              else
                y = ADD_LONG( y, args[0] );

              if ( cff_builder_add_point1( builder, x, y ) )
                goto Fail;

              args++;
              phase ^= 1;
            }
            args = stack;
          }
          break;

