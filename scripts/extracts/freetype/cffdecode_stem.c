          case cff_op_hstem:
          case cff_op_vstem:
          case cff_op_vmoveto:
          case cff_op_rlineto:
          case cff_op_hlineto:
          case cff_op_vlineto:
          case cff_op_rrcurveto:
          case cff_op_hstemhm:
          case cff_op_hintmask:
          case cff_op_cntrmask:
          case cff_op_rmoveto:
          case cff_op_hmoveto:
          case cff_op_vstemhm:
          case cff_op_rcurveline:
          case cff_op_rlinecurve:
          case cff_op_vvcurveto:
          case cff_op_hhcurveto:
          case cff_op_vhcurveto:
          case cff_op_hvcurveto:
          case cff_op_hflex:
          case cff_op_flex:
          case cff_op_hflex1:
          case cff_op_flex1:
          case cff_op_callsubr:
          case cff_op_callgsubr:
            /* deprecated opcodes */
          case cff_op_dotsection:
            /* invalid Type 1 opcodes */
          case cff_op_hsbw:
          case cff_op_closepath:
          case cff_op_callothersubr:
          case cff_op_seac:
          case cff_op_sbw:
          case cff_op_setcurrentpoint:
            goto MM_Error;

          default:
            break;
          }
        }

        /* check arguments */
        req_args = cff_argument_counts[op];
        if ( req_args & CFF_COUNT_CHECK_WIDTH )
        {
          if ( num_args > 0 && decoder->read_width )
          {
            /* If `nominal_width' is non-zero, the number is really a      */
            /* difference against `nominal_width'.  Else, the number here  */
            /* is truly a width, not a difference against `nominal_width'. */
            /* If the font does not set `nominal_width', then              */
            /* `nominal_width' defaults to zero, and so we can set         */
            /* `glyph_width' to `nominal_width' plus number on the stack   */
            /* -- for either case.                                         */

            FT_Int  set_width_ok;


            switch ( op )
            {
            case cff_op_hmoveto:
            case cff_op_vmoveto:
              set_width_ok = num_args & 2;
              break;

            case cff_op_hstem:
            case cff_op_vstem:
            case cff_op_hstemhm:
            case cff_op_vstemhm:
            case cff_op_rmoveto:
            case cff_op_hintmask:
            case cff_op_cntrmask:
              set_width_ok = num_args & 1;
              break;

            case cff_op_endchar:
              /* If there is a width specified for endchar, we either have */
              /* 1 argument or 5 arguments.  We like to argue.             */
              set_width_ok = in_dict
                               ? 0
                               : ( ( num_args == 5 ) || ( num_args == 1 ) );
              break;

            default:
              set_width_ok = 0;
              break;
            }

            if ( set_width_ok )
            {
              decoder->glyph_width = decoder->nominal_width +
                                       ( stack[0] >> 16 );

              if ( decoder->width_only )
              {
                /* we only want the advance width; stop here */
                break;
              }

              /* Consumed an argument. */
              num_args--;
            }
          }

          decoder->read_width = 0;
          req_args            = 0;
        }

        req_args &= 0x000F;
        if ( num_args < req_args )
          goto Stack_Underflow;
        args     -= req_args;
        num_args -= req_args;

        /* At this point, `args' points to the first argument of the  */
        /* operand in case `req_args' isn't zero.  Otherwise, we have */
        /* to adjust `args' manually.                                 */

        /* Note that we only pop arguments from the stack which we    */
        /* really need and can digest so that we can continue in case */
        /* of superfluous stack elements.                             */

        switch ( op )
        {
