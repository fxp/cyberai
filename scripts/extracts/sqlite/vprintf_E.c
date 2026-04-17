/* SQLite 3.49.1 — L32140-32270 — sqlite3_str_vappendf E: float+EXP+GENERIC */
      case etSIZE:
        if( !bArgList ){
          *(va_arg(ap,int*)) = pAccum->nChar;
        }
        length = width = 0;
        break;
      case etPERCENT:
        buf[0] = '%';
        bufpt = buf;
        length = 1;
        break;
      case etCHARX:
        if( bArgList ){
          bufpt = getTextArg(pArgList);
          length = 1;
          if( bufpt ){
            buf[0] = c = *(bufpt++);
            if( (c&0xc0)==0xc0 ){
              while( length<4 && (bufpt[0]&0xc0)==0x80 ){
                buf[length++] = *(bufpt++);
              }
            }
          }else{
            buf[0] = 0;
          }
        }else{
          unsigned int ch = va_arg(ap,unsigned int);
          if( ch<0x00080 ){
            buf[0] = ch & 0xff;
            length = 1;
          }else if( ch<0x00800 ){
            buf[0] = 0xc0 + (u8)((ch>>6)&0x1f);
            buf[1] = 0x80 + (u8)(ch & 0x3f);
            length = 2;
          }else if( ch<0x10000 ){
            buf[0] = 0xe0 + (u8)((ch>>12)&0x0f);
            buf[1] = 0x80 + (u8)((ch>>6) & 0x3f);
            buf[2] = 0x80 + (u8)(ch & 0x3f);
            length = 3;
          }else{
            buf[0] = 0xf0 + (u8)((ch>>18) & 0x07);
            buf[1] = 0x80 + (u8)((ch>>12) & 0x3f);
            buf[2] = 0x80 + (u8)((ch>>6) & 0x3f);
            buf[3] = 0x80 + (u8)(ch & 0x3f);
            length = 4;
          }
        }
        if( precision>1 ){
          i64 nPrior = 1;
          width -= precision-1;
          if( width>1 && !flag_leftjustify ){
            sqlite3_str_appendchar(pAccum, width-1, ' ');
            width = 0;
          }
          sqlite3_str_append(pAccum, buf, length);
          precision--;
          while( precision > 1 ){
            i64 nCopyBytes;
            if( nPrior > precision-1 ) nPrior = precision - 1;
            nCopyBytes = length*nPrior;
            if( nCopyBytes + pAccum->nChar >= pAccum->nAlloc ){
              sqlite3StrAccumEnlarge(pAccum, nCopyBytes);
            }
            if( pAccum->accError ) break;
            sqlite3_str_append(pAccum,
                 &pAccum->zText[pAccum->nChar-nCopyBytes], nCopyBytes);
            precision -= nPrior;
            nPrior *= 2;
          }
        }
        bufpt = buf;
        flag_altform2 = 1;
        goto adjust_width_for_utf8;
      case etSTRING:
      case etDYNSTRING:
        if( bArgList ){
          bufpt = getTextArg(pArgList);
          xtype = etSTRING;
        }else{
          bufpt = va_arg(ap,char*);
        }
        if( bufpt==0 ){
          bufpt = "";
        }else if( xtype==etDYNSTRING ){
          if( pAccum->nChar==0
           && pAccum->mxAlloc
           && width==0
           && precision<0
           && pAccum->accError==0
          ){
            /* Special optimization for sqlite3_mprintf("%z..."):
            ** Extend an existing memory allocation rather than creating
            ** a new one. */
            assert( (pAccum->printfFlags&SQLITE_PRINTF_MALLOCED)==0 );
            pAccum->zText = bufpt;
            pAccum->nAlloc = sqlite3DbMallocSize(pAccum->db, bufpt);
            pAccum->nChar = 0x7fffffff & (int)strlen(bufpt);
            pAccum->printfFlags |= SQLITE_PRINTF_MALLOCED;
            length = 0;
            break;
          }
          zExtra = bufpt;
        }
        if( precision>=0 ){
          if( flag_altform2 ){
            /* Set length to the number of bytes needed in order to display
            ** precision characters */
            unsigned char *z = (unsigned char*)bufpt;
            while( precision-- > 0 && z[0] ){
              SQLITE_SKIP_UTF8(z);
            }
            length = (int)(z - (unsigned char*)bufpt);
          }else{
            for(length=0; length<precision && bufpt[length]; length++){}
          }
        }else{
          length = 0x7fffffff & (int)strlen(bufpt);
        }
      adjust_width_for_utf8:
        if( flag_altform2 && width>0 ){
          /* Adjust width to account for extra bytes in UTF-8 characters */
          int ii = length - 1;
          while( ii>=0 ) if( (bufpt[ii--] & 0xc0)==0x80 ) width++;
        }
        break;
      case etSQLESCAPE:           /* %q: Escape ' characters */
      case etSQLESCAPE2:          /* %Q: Escape ' and enclose in '...' */
      case etSQLESCAPE3: {        /* %w: Escape " characters */
        i64 i, j, k, n;
        int needQuote, isnull;
        char ch;
