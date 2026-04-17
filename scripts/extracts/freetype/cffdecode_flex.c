            /* point's y-value                                        */
            x = ADD_LONG( x, args[5] );
            cff_builder_add_point( builder, x, y, 0 );

            /* fourth control point */
            x = ADD_LONG( x, args[6] );
            y = ADD_LONG( y, args[7] );
            cff_builder_add_point( builder, x, y, 0 );

            /* ending point, with y-value the same as the start   */
            x = ADD_LONG( x, args[8] );
            y = start_y;
            cff_builder_add_point( builder, x, y, 1 );

            args = stack;
            break;
          }

        case cff_op_hflex:
          {
            FT_Pos  start_y;


            FT_TRACE4(( " hflex\n" ));

            /* adding six more points; 4 control points, 2 on-curve points */
            if ( cff_builder_start_point( builder, x, y ) ||
                 cff_check_points( builder, 6 )           )
              goto Fail;

            /* record the starting point's y-position for later use */
            start_y = y;

            /* first control point */
            x = ADD_LONG( x, args[0] );
            cff_builder_add_point( builder, x, y, 0 );

            /* second control point */
            x = ADD_LONG( x, args[1] );
            y = ADD_LONG( y, args[2] );
            cff_builder_add_point( builder, x, y, 0 );

            /* join point; on curve, with y-value the same as the last */
            /* control point's y-value                                 */
            x = ADD_LONG( x, args[3] );
            cff_builder_add_point( builder, x, y, 1 );

            /* third control point, with y-value the same as the join */
            /* point's y-value                                        */
            x = ADD_LONG( x, args[4] );
            cff_builder_add_point( builder, x, y, 0 );

            /* fourth control point */
            x = ADD_LONG( x, args[5] );
            y = start_y;
            cff_builder_add_point( builder, x, y, 0 );

            /* ending point, with y-value the same as the start point's */
            /* y-value -- we don't add this point, though               */
            x = ADD_LONG( x, args[6] );
            cff_builder_add_point( builder, x, y, 1 );

            args = stack;
            break;
          }

        case cff_op_flex1:
          {
            FT_Pos     start_x, start_y; /* record start x, y values for */
                                         /* alter use                    */
            FT_Fixed   dx = 0, dy = 0;   /* used in horizontal/vertical  */
                                         /* algorithm below              */
            FT_Int     horizontal, count;
            FT_Fixed*  temp;


            FT_TRACE4(( " flex1\n" ));

            /* adding six more points; 4 control points, 2 on-curve points */
            if ( cff_builder_start_point( builder, x, y ) ||
                 cff_check_points( builder, 6 )           )
              goto Fail;

            /* record the starting point's x, y position for later use */
            start_x = x;
            start_y = y;

            /* XXX: figure out whether this is supposed to be a horizontal */
            /*      or vertical flex; the Type 2 specification is vague... */

            temp = args;

            /* grab up to the last argument */
            for ( count = 5; count > 0; count-- )
            {
              dx    = ADD_LONG( dx, temp[0] );
              dy    = ADD_LONG( dy, temp[1] );
              temp += 2;
            }

            if ( dx < 0 )
              dx = NEG_LONG( dx );
            if ( dy < 0 )
              dy = NEG_LONG( dy );

            /* strange test, but here it is... */
            horizontal = ( dx > dy );

            for ( count = 5; count > 0; count-- )
            {
              x = ADD_LONG( x, args[0] );
              y = ADD_LONG( y, args[1] );
              cff_builder_add_point( builder, x, y,
                                     FT_BOOL( count == 3 ) );
              args += 2;
            }

            /* is last operand an x- or y-delta? */
            if ( horizontal )
            {
              x = ADD_LONG( x, args[0] );
              y = start_y;
            }
            else
            {
              x = start_x;
              y = ADD_LONG( y, args[0] );
            }

            cff_builder_add_point( builder, x, y, 1 );

            args = stack;
            break;
           }

        case cff_op_flex:
          {
            FT_UInt  count;


            FT_TRACE4(( " flex\n" ));

            if ( cff_builder_start_point( builder, x, y ) ||
                 cff_check_points( builder, 6 )           )
              goto Fail;

            for ( count = 6; count > 0; count-- )
            {
              x = ADD_LONG( x, args[0] );
              y = ADD_LONG( y, args[1] );
              cff_builder_add_point( builder, x, y,
                                     FT_BOOL( count == 4 || count == 1 ) );
              args += 2;
            }

            args = stack;
          }
          break;

