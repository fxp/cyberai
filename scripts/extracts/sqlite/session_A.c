/* SQLite 3.49.1 — L230220-230335 — sessionReadRecord A: session change record reader (CVE-2023-7104 class) */
** This function sets the value of the sqlite3_value object passed as the
** first argument to a copy of the string or blob held in the aData[]
** buffer. SQLITE_OK is returned if successful, or SQLITE_NOMEM if an OOM
** error occurs.
*/
static int sessionValueSetStr(
  sqlite3_value *pVal,            /* Set the value of this object */
  u8 *aData,                      /* Buffer containing string or blob data */
  int nData,                      /* Size of buffer aData[] in bytes */
  u8 enc                          /* String encoding (0 for blobs) */
){
  /* In theory this code could just pass SQLITE_TRANSIENT as the final
  ** argument to sqlite3ValueSetStr() and have the copy created
  ** automatically. But doing so makes it difficult to detect any OOM
  ** error. Hence the code to create the copy externally. */
  u8 *aCopy = sqlite3_malloc64((sqlite3_int64)nData+1);
  if( aCopy==0 ) return SQLITE_NOMEM;
  memcpy(aCopy, aData, nData);
  sqlite3ValueSetStr(pVal, nData, (char*)aCopy, enc, sqlite3_free);
  return SQLITE_OK;
}

/*
** Deserialize a single record from a buffer in memory. See "RECORD FORMAT"
** for details.
**
** When this function is called, *paChange points to the start of the record
** to deserialize. Assuming no error occurs, *paChange is set to point to
** one byte after the end of the same record before this function returns.
** If the argument abPK is NULL, then the record contains nCol values. Or,
** if abPK is other than NULL, then the record contains only the PK fields
** (in other words, it is a patchset DELETE record).
**
** If successful, each element of the apOut[] array (allocated by the caller)
** is set to point to an sqlite3_value object containing the value read
** from the corresponding position in the record. If that value is not
** included in the record (i.e. because the record is part of an UPDATE change
** and the field was not modified), the corresponding element of apOut[] is
** set to NULL.
**
** It is the responsibility of the caller to free all sqlite_value structures
** using sqlite3_free().
**
** If an error occurs, an SQLite error code (e.g. SQLITE_NOMEM) is returned.
** The apOut[] array may have been partially populated in this case.
*/
static int sessionReadRecord(
  SessionInput *pIn,              /* Input data */
  int nCol,                       /* Number of values in record */
  u8 *abPK,                       /* Array of primary key flags, or NULL */
  sqlite3_value **apOut,          /* Write values to this array */
  int *pbEmpty
){
  int i;                          /* Used to iterate through columns */
  int rc = SQLITE_OK;

  assert( pbEmpty==0 || *pbEmpty==0 );
  if( pbEmpty ) *pbEmpty = 1;
  for(i=0; i<nCol && rc==SQLITE_OK; i++){
    int eType = 0;                /* Type of value (SQLITE_NULL, TEXT etc.) */
    if( abPK && abPK[i]==0 ) continue;
    rc = sessionInputBuffer(pIn, 9);
    if( rc==SQLITE_OK ){
      if( pIn->iNext>=pIn->nData ){
        rc = SQLITE_CORRUPT_BKPT;
      }else{
        eType = pIn->aData[pIn->iNext++];
        assert( apOut[i]==0 );
        if( eType ){
          if( pbEmpty ) *pbEmpty = 0;
          apOut[i] = sqlite3ValueNew(0);
          if( !apOut[i] ) rc = SQLITE_NOMEM;
        }
      }
    }

    if( rc==SQLITE_OK ){
      u8 *aVal = &pIn->aData[pIn->iNext];
      if( eType==SQLITE_TEXT || eType==SQLITE_BLOB ){
        int nByte;
        pIn->iNext += sessionVarintGet(aVal, &nByte);
        rc = sessionInputBuffer(pIn, nByte);
        if( rc==SQLITE_OK ){
          if( nByte<0 || nByte>pIn->nData-pIn->iNext ){
            rc = SQLITE_CORRUPT_BKPT;
          }else{
            u8 enc = (eType==SQLITE_TEXT ? SQLITE_UTF8 : 0);
            rc = sessionValueSetStr(apOut[i],&pIn->aData[pIn->iNext],nByte,enc);
            pIn->iNext += nByte;
          }
        }
      }
      if( eType==SQLITE_INTEGER || eType==SQLITE_FLOAT ){
        if( (pIn->nData-pIn->iNext)<8 ){
          rc = SQLITE_CORRUPT_BKPT;
        }else{
          sqlite3_int64 v = sessionGetI64(aVal);
          if( eType==SQLITE_INTEGER ){
            sqlite3VdbeMemSetInt64(apOut[i], v);
          }else{
            double d;
            memcpy(&d, &v, 8);
            sqlite3VdbeMemSetDouble(apOut[i], d);
          }
          pIn->iNext += 8;
        }
      }
    }
  }

  return rc;
}

/*
** The input pointer currently points to the second byte of a table-header.
** Specifically, to the following:
