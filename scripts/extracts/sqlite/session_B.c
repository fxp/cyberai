/* SQLite 3.49.1 — L230850-230975 — sessionApplyChange: session record applier */
    /* Test for EOF. */
    if( (rc = sessionInputBuffer(pInput, 2)) ) goto finished_invert;
    if( pInput->iNext>=pInput->nData ) break;
    eType = pInput->aData[pInput->iNext];

    switch( eType ){
      case 'T': {
        /* A 'table' record consists of:
        **
        **   * A constant 'T' character,
        **   * Number of columns in said table (a varint),
        **   * An array of nCol bytes (sPK),
        **   * A nul-terminated table name.
        */
        int nByte;
        int nVar;
        pInput->iNext++;
        if( (rc = sessionChangesetBufferTblhdr(pInput, &nByte)) ){
          goto finished_invert;
        }
        nVar = sessionVarintGet(&pInput->aData[pInput->iNext], &nCol);
        sPK.nBuf = 0;
        sessionAppendBlob(&sPK, &pInput->aData[pInput->iNext+nVar], nCol, &rc);
        sessionAppendByte(&sOut, eType, &rc);
        sessionAppendBlob(&sOut, &pInput->aData[pInput->iNext], nByte, &rc);
        if( rc ) goto finished_invert;

        pInput->iNext += nByte;
        sqlite3_free(apVal);
        apVal = 0;
        abPK = sPK.aBuf;
        break;
      }

      case SQLITE_INSERT:
      case SQLITE_DELETE: {
        int nByte;
        int bIndirect = pInput->aData[pInput->iNext+1];
        int eType2 = (eType==SQLITE_DELETE ? SQLITE_INSERT : SQLITE_DELETE);
        pInput->iNext += 2;
        assert( rc==SQLITE_OK );
        rc = sessionChangesetBufferRecord(pInput, nCol, &nByte);
        sessionAppendByte(&sOut, eType2, &rc);
        sessionAppendByte(&sOut, bIndirect, &rc);
        sessionAppendBlob(&sOut, &pInput->aData[pInput->iNext], nByte, &rc);
        pInput->iNext += nByte;
        if( rc ) goto finished_invert;
        break;
      }

      case SQLITE_UPDATE: {
        int iCol;

        if( 0==apVal ){
          apVal = (sqlite3_value **)sqlite3_malloc64(sizeof(apVal[0])*nCol*2);
          if( 0==apVal ){
            rc = SQLITE_NOMEM;
            goto finished_invert;
          }
          memset(apVal, 0, sizeof(apVal[0])*nCol*2);
        }

        /* Write the header for the new UPDATE change. Same as the original. */
        sessionAppendByte(&sOut, eType, &rc);
        sessionAppendByte(&sOut, pInput->aData[pInput->iNext+1], &rc);

        /* Read the old.* and new.* records for the update change. */
        pInput->iNext += 2;
        rc = sessionReadRecord(pInput, nCol, 0, &apVal[0], 0);
        if( rc==SQLITE_OK ){
          rc = sessionReadRecord(pInput, nCol, 0, &apVal[nCol], 0);
        }

        /* Write the new old.* record. Consists of the PK columns from the
        ** original old.* record, and the other values from the original
        ** new.* record. */
        for(iCol=0; iCol<nCol; iCol++){
          sqlite3_value *pVal = apVal[iCol + (abPK[iCol] ? 0 : nCol)];
          sessionAppendValue(&sOut, pVal, &rc);
        }

        /* Write the new new.* record. Consists of a copy of all values
        ** from the original old.* record, except for the PK columns, which
        ** are set to "undefined". */
        for(iCol=0; iCol<nCol; iCol++){
          sqlite3_value *pVal = (abPK[iCol] ? 0 : apVal[iCol]);
          sessionAppendValue(&sOut, pVal, &rc);
        }

        for(iCol=0; iCol<nCol*2; iCol++){
          sqlite3ValueFree(apVal[iCol]);
        }
        memset(apVal, 0, sizeof(apVal[0])*nCol*2);
        if( rc!=SQLITE_OK ){
          goto finished_invert;
        }

        break;
      }

      default:
        rc = SQLITE_CORRUPT_BKPT;
        goto finished_invert;
    }

    assert( rc==SQLITE_OK );
    if( xOutput && sOut.nBuf>=sessions_strm_chunk_size ){
      rc = xOutput(pOut, sOut.aBuf, sOut.nBuf);
      sOut.nBuf = 0;
      if( rc!=SQLITE_OK ) goto finished_invert;
    }
  }

  assert( rc==SQLITE_OK );
  if( pnInverted && ALWAYS(ppInverted) ){
    *pnInverted = sOut.nBuf;
    *ppInverted = sOut.aBuf;
    sOut.aBuf = 0;
  }else if( sOut.nBuf>0 && ALWAYS(xOutput!=0) ){
    rc = xOutput(pOut, sOut.aBuf, sOut.nBuf);
  }

 finished_invert:
  sqlite3_free(sOut.aBuf);
  sqlite3_free(apVal);
  sqlite3_free(sPK.aBuf);
