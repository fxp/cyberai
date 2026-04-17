        case cff_op_rrcurveto:
          {
            FT_Int  nargs;


            FT_TRACE4(( " rrcurveto\n" ));

            if ( num_args < 6 )
              goto Stack_Underflow;

            nargs = num_args - num_args % 6;

            if ( cff_builder_start_point( builder, x, y ) ||
                 cff_check_points( builder, nargs / 2 )   )
              goto Fail;

            args -= nargs;
            while ( args < decoder->top )
            {
              x = ADD_LONG( x, args[0] );
              y = ADD_LONG( y, args[1] );
              cff_builder_add_point( builder, x, y, 0 );

              x = ADD_LONG( x, args[2] );
              y = ADD_LONG( y, args[3] );
              cff_builder_add_point( builder, x, y, 0 );

              x = ADD_LONG( x, args[4] );
              y = ADD_LONG( y, args[5] );
              cff_builder_add_point( builder, x, y, 1 );

              args += 6;
            }
            args = stack;
          }
          break;

        case cff_op_vvcurveto:
          {
            FT_Int  nargs;


            FT_TRACE4(( " vvcurveto\n" ));

            if ( num_args < 4 )
              goto Stack_Underflow;

            /* if num_args isn't of the form 4n or 4n+1, */
            /* we enforce it by clearing the second bit  */

            nargs = num_args & ~2;

            if ( cff_builder_start_point( builder, x, y ) )
              goto Fail;

            args -= nargs;

            if ( nargs & 1 )
            {
              x = ADD_LONG( x, args[0] );
              args++;
              nargs--;
            }

            if ( cff_check_points( builder, 3 * ( nargs / 4 ) ) )
              goto Fail;

            while ( args < decoder->top )
            {
              y = ADD_LONG( y, args[0] );
              cff_builder_add_point( builder, x, y, 0 );

              x = ADD_LONG( x, args[1] );
              y = ADD_LONG( y, args[2] );
              cff_builder_add_point( builder, x, y, 0 );

              y = ADD_LONG( y, args[3] );
              cff_builder_add_point( builder, x, y, 1 );

              args += 4;
            }
            args = stack;
          }
          break;

        case cff_op_hhcurveto:
          {
            FT_Int  nargs;


            FT_TRACE4(( " hhcurveto\n" ));

            if ( num_args < 4 )
              goto Stack_Underflow;

            /* if num_args isn't of the form 4n or 4n+1, */
            /* we enforce it by clearing the second bit  */

            nargs = num_args & ~2;

            if ( cff_builder_start_point( builder, x, y ) )
              goto Fail;

            args -= nargs;
            if ( nargs & 1 )
            {
              y = ADD_LONG( y, args[0] );
              args++;
              nargs--;
            }

            if ( cff_check_points( builder, 3 * ( nargs / 4 ) ) )
              goto Fail;

            while ( args < decoder->top )
            {
              x = ADD_LONG( x, args[0] );
              cff_builder_add_point( builder, x, y, 0 );

              x = ADD_LONG( x, args[1] );
              y = ADD_LONG( y, args[2] );
              cff_builder_add_point( builder, x, y, 0 );

              x = ADD_LONG( x, args[3] );
              cff_builder_add_point( builder, x, y, 1 );

              args += 4;
            }
            args = stack;
          }
          break;

        case cff_op_vhcurveto:
        case cff_op_hvcurveto:
          {
            FT_Int  phase;
            FT_Int  nargs;


            FT_TRACE4(( "%s\n", op == cff_op_vhcurveto ? " vhcurveto"
                                                       : " hvcurveto" ));

            if ( cff_builder_start_point( builder, x, y ) )
              goto Fail;

            if ( num_args < 4 )
              goto Stack_Underflow;

            /* if num_args isn't of the form 8n, 8n+1, 8n+4, or 8n+5, */
            /* we enforce it by clearing the second bit               */

            nargs = num_args & ~2;

            args -= nargs;
            if ( cff_check_points( builder, ( nargs / 4 ) * 3 ) )
              goto Stack_Underflow;

            phase = ( op == cff_op_hvcurveto );

            while ( nargs >= 4 )
            {
              nargs -= 4;
              if ( phase )
              {
                x = ADD_LONG( x, args[0] );
                cff_builder_add_point( builder, x, y, 0 );

                x = ADD_LONG( x, args[1] );
                y = ADD_LONG( y, args[2] );
                cff_builder_add_point( builder, x, y, 0 );

                y = ADD_LONG( y, args[3] );
                if ( nargs == 1 )
                  x = ADD_LONG( x, args[4] );
                cff_builder_add_point( builder, x, y, 1 );
              }
              else
