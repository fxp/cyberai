              {
                y = ADD_LONG( y, args[0] );
                cff_builder_add_point( builder, x, y, 0 );

                x = ADD_LONG( x, args[1] );
                y = ADD_LONG( y, args[2] );
                cff_builder_add_point( builder, x, y, 0 );

                x = ADD_LONG( x, args[3] );
                if ( nargs == 1 )
                  y = ADD_LONG( y, args[4] );
                cff_builder_add_point( builder, x, y, 1 );
              }
              args  += 4;
              phase ^= 1;
            }
            args = stack;
          }
          break;

        case cff_op_rlinecurve:
          {
            FT_Int  num_lines;
            FT_Int  nargs;


            FT_TRACE4(( " rlinecurve\n" ));

            if ( num_args < 8 )
              goto Stack_Underflow;

            nargs     = num_args & ~1;
            num_lines = ( nargs - 6 ) / 2;

            if ( cff_builder_start_point( builder, x, y )   ||
                 cff_check_points( builder, num_lines + 3 ) )
              goto Fail;

            args -= nargs;

            /* first, add the line segments */
            while ( num_lines > 0 )
            {
              x = ADD_LONG( x, args[0] );
              y = ADD_LONG( y, args[1] );
              cff_builder_add_point( builder, x, y, 1 );

              args += 2;
              num_lines--;
            }

            /* then the curve */
            x = ADD_LONG( x, args[0] );
            y = ADD_LONG( y, args[1] );
            cff_builder_add_point( builder, x, y, 0 );

            x = ADD_LONG( x, args[2] );
            y = ADD_LONG( y, args[3] );
            cff_builder_add_point( builder, x, y, 0 );

            x = ADD_LONG( x, args[4] );
            y = ADD_LONG( y, args[5] );
            cff_builder_add_point( builder, x, y, 1 );

            args = stack;
          }
          break;

        case cff_op_rcurveline:
          {
            FT_Int  num_curves;
            FT_Int  nargs;


            FT_TRACE4(( " rcurveline\n" ));

            if ( num_args < 8 )
              goto Stack_Underflow;

            nargs      = num_args - 2;
            nargs      = nargs - nargs % 6 + 2;
            num_curves = ( nargs - 2 ) / 6;

            if ( cff_builder_start_point( builder, x, y )        ||
                 cff_check_points( builder, num_curves * 3 + 2 ) )
              goto Fail;

            args -= nargs;

            /* first, add the curves */
            while ( num_curves > 0 )
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
              num_curves--;
            }

            /* then the final line */
            x = ADD_LONG( x, args[0] );
            y = ADD_LONG( y, args[1] );
            cff_builder_add_point( builder, x, y, 1 );

            args = stack;
          }
          break;

        case cff_op_hflex1:
          {
            FT_Pos  start_y;


            FT_TRACE4(( " hflex1\n" ));

            /* adding five more points: 4 control points, 1 on-curve point */
            /* -- make sure we have enough space for the start point if it */
            /* needs to be added                                           */
            if ( cff_builder_start_point( builder, x, y ) ||
                 cff_check_points( builder, 6 )           )
              goto Fail;

            /* record the starting point's y position for later use */
            start_y = y;

            /* first control point */
            x = ADD_LONG( x, args[0] );
            y = ADD_LONG( y, args[1] );
            cff_builder_add_point( builder, x, y, 0 );

            /* second control point */
            x = ADD_LONG( x, args[2] );
            y = ADD_LONG( y, args[3] );
            cff_builder_add_point( builder, x, y, 0 );

            /* join point; on curve, with y-value the same as the last */
            /* control point's y-value                                 */
            x = ADD_LONG( x, args[4] );
            cff_builder_add_point( builder, x, y, 1 );

            /* third control point, with y-value the same as the join */
