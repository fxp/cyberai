          args++;
          break;

        case cff_op_drop:
          /* nothing */
          FT_TRACE4(( " drop\n" ));

          break;

        case cff_op_exch:
          {
            FT_Fixed  tmp;


            FT_TRACE4(( " exch\n" ));

            tmp     = args[0];
            args[0] = args[1];
            args[1] = tmp;
            args   += 2;
          }
          break;

        case cff_op_index:
          {
            FT_Int  idx = (FT_Int)( args[0] >> 16 );


            FT_TRACE4(( " index\n" ));

            if ( idx < 0 )
              idx = 0;
            else if ( idx > num_args - 2 )
              idx = num_args - 2;
            args[0] = args[-( idx + 1 )];
            args++;
          }
          break;

        case cff_op_roll:
          {
            FT_Int  count = (FT_Int)( args[0] >> 16 );
            FT_Int  idx   = (FT_Int)( args[1] >> 16 );


            FT_TRACE4(( " roll\n" ));

            if ( count <= 0 )
              count = 1;

            args -= count;
            if ( args < stack )
              goto Stack_Underflow;

            if ( idx >= 0 )
            {
              idx = idx % count;
              while ( idx > 0 )
              {
                FT_Fixed  tmp = args[count - 1];
                FT_Int    i;


                for ( i = count - 2; i >= 0; i-- )
                  args[i + 1] = args[i];
                args[0] = tmp;
                idx--;
              }
            }
            else
            {
              /* before C99 it is implementation-defined whether    */
              /* the result of `%' is negative if the first operand */
              /* is negative                                        */
              idx = -( NEG_INT( idx ) % count );
              while ( idx < 0 )
              {
                FT_Fixed  tmp = args[0];
                FT_Int    i;


                for ( i = 0; i < count - 1; i++ )
                  args[i] = args[i + 1];
                args[count - 1] = tmp;
                idx++;
              }
            }
            args += count;
          }
          break;

        case cff_op_dup:
          FT_TRACE4(( " dup\n" ));

          args[1] = args[0];
          args   += 2;
          break;

        case cff_op_put:
          {
            FT_Fixed  val = args[0];
            FT_UInt   idx = (FT_UInt)( args[1] >> 16 );


            FT_TRACE4(( " put\n" ));

            /* the Type2 specification before version 16-March-2000 */
            /* didn't give a hard-coded size limit of the temporary */
            /* storage array; instead, an argument of the           */
            /* `MultipleMaster' operator set the size               */
            if ( idx < CFF_MAX_TRANS_ELEMENTS )
              decoder->buildchar[idx] = val;
          }
          break;

        case cff_op_get:
          {
            FT_UInt   idx = (FT_UInt)( args[0] >> 16 );
            FT_Fixed  val = 0;


            FT_TRACE4(( " get\n" ));

            if ( idx < CFF_MAX_TRANS_ELEMENTS )
              val = decoder->buildchar[idx];

            args[0] = val;
            args++;
          }
          break;

        case cff_op_store:
          /* this operator was removed from the Type2 specification */
          /* in version 16-March-2000                               */

          /* since we currently don't handle interpolation of multiple */
          /* master fonts, this is a no-op                             */
          FT_TRACE4(( " store\n" ));
          break;

        case cff_op_load:
          /* this operator was removed from the Type2 specification */
          /* in version 16-March-2000                               */
          {
            FT_UInt  reg_idx = (FT_UInt)args[0];
            FT_UInt  idx     = (FT_UInt)args[1];
            FT_UInt  count   = (FT_UInt)args[2];


            FT_TRACE4(( " load\n" ));

            /* since we currently don't handle interpolation of multiple */
            /* master fonts, we store a vector [1 0 0 ...] in the        */
            /* temporary storage array regardless of the Registry index  */
            if ( reg_idx <= 2                 &&
                 idx < CFF_MAX_TRANS_ELEMENTS &&
                 count <= num_axes            )
            {
