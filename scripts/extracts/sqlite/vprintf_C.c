/* SQLite 3.49.1 — L31930-32035 — sqlite3_str_vappendf C: integer-to-string + buf sizing */
            x = 0;
          }
          *(--bufpt) = zOrd[x*2+1];
          *(--bufpt) = zOrd[x*2];
        }
        {
          const char *cset = &aDigits[infop->charset];
          u8 base = infop->base;
          do{                                           /* Convert to ascii */
            *(--bufpt) = cset[longvalue%base];
            longvalue = longvalue/base;
          }while( longvalue>0 );
        }
        length = (int)(&zOut[nOut-1]-bufpt);
        while( precision>length ){
          *(--bufpt) = '0';                             /* Zero pad */
          length++;
        }
        if( cThousand ){
          int nn = (length - 1)/3;  /* Number of "," to insert */
          int ix = (length - 1)%3 + 1;
          bufpt -= nn;
          for(idx=0; nn>0; idx++){
            bufpt[idx] = bufpt[idx+nn];
            ix--;
            if( ix==0 ){
              bufpt[++idx] = cThousand;
              nn--;
              ix = 3;
            }
          }
        }
        if( prefix ) *(--bufpt) = prefix;               /* Add sign */
        if( flag_alternateform && infop->prefix ){      /* Add "0" or "0x" */
          const char *pre;
          char x;
          pre = &aPrefix[infop->prefix];
          for(; (x=(*pre))!=0; pre++) *(--bufpt) = x;
        }
        length = (int)(&zOut[nOut-1]-bufpt);
        break;
      case etFLOAT:
      case etEXP:
      case etGENERIC: {
        FpDecode s;
        int iRound;
        int j;

        if( bArgList ){
          realvalue = getDoubleArg(pArgList);
        }else{
          realvalue = va_arg(ap,double);
        }
        if( precision<0 ) precision = 6;         /* Set default precision */
#ifdef SQLITE_FP_PRECISION_LIMIT
        if( precision>SQLITE_FP_PRECISION_LIMIT ){
          precision = SQLITE_FP_PRECISION_LIMIT;
        }
#endif
        if( xtype==etFLOAT ){
          iRound = -precision;
        }else if( xtype==etGENERIC ){
          if( precision==0 ) precision = 1;
          iRound = precision;
        }else{
          iRound = precision+1;
        }
        sqlite3FpDecode(&s, realvalue, iRound, flag_altform2 ? 26 : 16);
        if( s.isSpecial ){
          if( s.isSpecial==2 ){
            bufpt = flag_zeropad ? "null" : "NaN";
            length = sqlite3Strlen30(bufpt);
            break;
          }else if( flag_zeropad ){
            s.z[0] = '9';
            s.iDP = 1000;
            s.n = 1;
          }else{
            memcpy(buf, "-Inf", 5);
            bufpt = buf;
            if( s.sign=='-' ){
              /* no-op */
            }else if( flag_prefix ){
              buf[0] = flag_prefix;
            }else{
              bufpt++;
            }
            length = sqlite3Strlen30(bufpt);
            break;
          }
        }
        if( s.sign=='-' ){
          prefix = '-';
        }else{
          prefix = flag_prefix;
        }

        exp = s.iDP-1;

        /*
        ** If the field type is etGENERIC, then convert to either etEXP
        ** or etFLOAT, as appropriate.
        */
        if( xtype==etGENERIC ){
          assert( precision>0 );
          precision--;
