          break;
        case 11:
          op = cff_op_return;
          break;
        case 12:
          if ( ip >= limit )
            goto Syntax_Error;
          v = *ip++;

          switch ( v )
          {
          case 0:
            op = cff_op_dotsection;
            break;
          case 1: /* this is actually the Type1 vstem3 operator */
            op = cff_op_vstem;
            break;
          case 2: /* this is actually the Type1 hstem3 operator */
            op = cff_op_hstem;
            break;
          case 3:
            op = cff_op_and;
            break;
          case 4:
            op = cff_op_or;
            break;
          case 5:
            op = cff_op_not;
            break;
          case 6:
            op = cff_op_seac;
            break;
          case 7:
            op = cff_op_sbw;
            break;
          case 8:
            op = cff_op_store;
            break;
          case 9:
            op = cff_op_abs;
            break;
          case 10:
            op = cff_op_add;
            break;
          case 11:
            op = cff_op_sub;
            break;
          case 12:
            op = cff_op_div;
            break;
          case 13:
            op = cff_op_load;
            break;
          case 14:
            op = cff_op_neg;
            break;
          case 15:
            op = cff_op_eq;
            break;
          case 16:
            op = cff_op_callothersubr;
            break;
          case 17:
            op = cff_op_pop;
            break;
          case 18:
            op = cff_op_drop;
            break;
          case 20:
            op = cff_op_put;
            break;
          case 21:
            op = cff_op_get;
            break;
          case 22:
            op = cff_op_ifelse;
            break;
          case 23:
            op = cff_op_random;
            break;
          case 24:
            op = cff_op_mul;
            break;
          case 26:
            op = cff_op_sqrt;
            break;
          case 27:
            op = cff_op_dup;
            break;
          case 28:
            op = cff_op_exch;
            break;
          case 29:
            op = cff_op_index;
            break;
          case 30:
            op = cff_op_roll;
            break;
          case 33:
            op = cff_op_setcurrentpoint;
            break;
          case 34:
            op = cff_op_hflex;
            break;
          case 35:
            op = cff_op_flex;
            break;
          case 36:
            op = cff_op_hflex1;
            break;
          case 37:
            op = cff_op_flex1;
            break;
          default:
            FT_TRACE4(( " unknown op (12, %d)\n", v ));
            break;
          }
          break;
        case 13:
          op = cff_op_hsbw;
          break;
        case 14:
          op = cff_op_endchar;
          break;
        case 16:
          op = cff_op_blend;
          break;
        case 18:
          op = cff_op_hstemhm;
          break;
        case 19:
          op = cff_op_hintmask;
          break;
        case 20:
          op = cff_op_cntrmask;
          break;
        case 21:
          op = cff_op_rmoveto;
          break;
        case 22:
          op = cff_op_hmoveto;
          break;
        case 23:
          op = cff_op_vstemhm;
          break;
        case 24:
          op = cff_op_rcurveline;
          break;
        case 25:
          op = cff_op_rlinecurve;
          break;
        case 26:
          op = cff_op_vvcurveto;
          break;
        case 27:
          op = cff_op_hhcurveto;
          break;
        case 29:
          op = cff_op_callgsubr;
          break;
        case 30:
          op = cff_op_vhcurveto;
          break;
        case 31:
          op = cff_op_hvcurveto;
          break;
        default:
          FT_TRACE4(( " unknown op (%d)\n", v ));
          break;
        }

        if ( op == cff_op_unknown )
          continue;

        /* in Multiple Master CFFs, T2 charstrings can appear in */
        /* dictionaries, but some operators are prohibited       */
        if ( in_dict )
        {
          switch ( op )
          {
