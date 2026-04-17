/* SQLite 3.49.1 — L32270-32395 — sqlite3_str_vappendf F: string+SQL-escape+TOKEN+SRCITEM */
        char ch;
        char q = ((xtype==etSQLESCAPE3)?'"':'\'');   /* Quote character */
        char *escarg;

        if( bArgList ){
          escarg = getTextArg(pArgList);
        }else{
          escarg = va_arg(ap,char*);
        }
        isnull = escarg==0;
        if( isnull ) escarg = (xtype==etSQLESCAPE2 ? "NULL" : "(NULL)");
        /* For %q, %Q, and %w, the precision is the number of bytes (or
        ** characters if the ! flags is present) to use from the input.
        ** Because of the extra quoting characters inserted, the number
        ** of output characters may be larger than the precision.
        */
        k = precision;
        for(i=n=0; k!=0 && (ch=escarg[i])!=0; i++, k--){
          if( ch==q )  n++;
          if( flag_altform2 && (ch&0xc0)==0xc0 ){
            while( (escarg[i+1]&0xc0)==0x80 ){ i++; }
          }
        }
        needQuote = !isnull && xtype==etSQLESCAPE2;
        n += i + 3;
        if( n>etBUFSIZE ){
          bufpt = zExtra = printfTempBuf(pAccum, n);
          if( bufpt==0 ) return;
        }else{
          bufpt = buf;
        }
        j = 0;
        if( needQuote ) bufpt[j++] = q;
        k = i;
        for(i=0; i<k; i++){
          bufpt[j++] = ch = escarg[i];
          if( ch==q ) bufpt[j++] = ch;
        }
        if( needQuote ) bufpt[j++] = q;
        bufpt[j] = 0;
        length = j;
        goto adjust_width_for_utf8;
      }
      case etTOKEN: {
        if( (pAccum->printfFlags & SQLITE_PRINTF_INTERNAL)==0 ) return;
        if( flag_alternateform ){
          /* %#T means an Expr pointer that uses Expr.u.zToken */
          Expr *pExpr = va_arg(ap,Expr*);
          if( ALWAYS(pExpr) && ALWAYS(!ExprHasProperty(pExpr,EP_IntValue)) ){
            sqlite3_str_appendall(pAccum, (const char*)pExpr->u.zToken);
            sqlite3RecordErrorOffsetOfExpr(pAccum->db, pExpr);
          }
        }else{
          /* %T means a Token pointer */
          Token *pToken = va_arg(ap, Token*);
          assert( bArgList==0 );
          if( pToken && pToken->n ){
            sqlite3_str_append(pAccum, (const char*)pToken->z, pToken->n);
            sqlite3RecordErrorByteOffset(pAccum->db, pToken->z);
          }
        }
        length = width = 0;
        break;
      }
      case etSRCITEM: {
        SrcItem *pItem;
        if( (pAccum->printfFlags & SQLITE_PRINTF_INTERNAL)==0 ) return;
        pItem = va_arg(ap, SrcItem*);
        assert( bArgList==0 );
        if( pItem->zAlias && !flag_altform2 ){
          sqlite3_str_appendall(pAccum, pItem->zAlias);
        }else if( pItem->zName ){
          if( pItem->fg.fixedSchema==0
           && pItem->fg.isSubquery==0
           && pItem->u4.zDatabase!=0
          ){
            sqlite3_str_appendall(pAccum, pItem->u4.zDatabase);
            sqlite3_str_append(pAccum, ".", 1);
          }
          sqlite3_str_appendall(pAccum, pItem->zName);
        }else if( pItem->zAlias ){
          sqlite3_str_appendall(pAccum, pItem->zAlias);
        }else if( ALWAYS(pItem->fg.isSubquery) ){/* Because of tag-20240424-1 */
          Select *pSel = pItem->u4.pSubq->pSelect;
          assert( pSel!=0 );
          if( pSel->selFlags & SF_NestedFrom ){
            sqlite3_str_appendf(pAccum, "(join-%u)", pSel->selId);
          }else if( pSel->selFlags & SF_MultiValue ){
            assert( !pItem->fg.isTabFunc && !pItem->fg.isIndexedBy );
            sqlite3_str_appendf(pAccum, "%u-ROW VALUES CLAUSE",
                                pItem->u1.nRow);
          }else{
            sqlite3_str_appendf(pAccum, "(subquery-%u)", pSel->selId);
          }
        }
        length = width = 0;
        break;
      }
      default: {
        assert( xtype==etINVALID );
        return;
      }
    }/* End switch over the format type */
    /*
    ** The text of the conversion is pointed to by "bufpt" and is
    ** "length" characters long.  The field width is "width".  Do
    ** the output.  Both length and width are in bytes, not characters,
    ** at this point.  If the "!" flag was present on string conversions
    ** indicating that width and precision should be expressed in characters,
    ** then the values have been translated prior to reaching this point.
    */
    width -= length;
    if( width>0 ){
      if( !flag_leftjustify ) sqlite3_str_appendchar(pAccum, width, ' ');
      sqlite3_str_append(pAccum, bufpt, length);
      if( flag_leftjustify ) sqlite3_str_appendchar(pAccum, width, ' ');
    }else{
      sqlite3_str_append(pAccum, bufpt, length);
    }

    if( zExtra ){
      sqlite3DbFree(pAccum->db, zExtra);
      zExtra = 0;
    }
  }/* End for loop over the format string */
} /* End of function */
