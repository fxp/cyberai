/* SQLite 3.49.1 — L84370-84500 — sqlite3VdbeMemFromBtreeZeroOffset + VdbeMemSetStr: mem management */
*/
static SQLITE_NOINLINE const void *valueToText(sqlite3_value* pVal, u8 enc){
  assert( pVal!=0 );
  assert( pVal->db==0 || sqlite3_mutex_held(pVal->db->mutex) );
  assert( (enc&3)==(enc&~SQLITE_UTF16_ALIGNED) );
  assert( !sqlite3VdbeMemIsRowSet(pVal) );
  assert( (pVal->flags & (MEM_Null))==0 );
  if( pVal->flags & (MEM_Blob|MEM_Str) ){
    if( ExpandBlob(pVal) ) return 0;
    pVal->flags |= MEM_Str;
    if( pVal->enc != (enc & ~SQLITE_UTF16_ALIGNED) ){
      sqlite3VdbeChangeEncoding(pVal, enc & ~SQLITE_UTF16_ALIGNED);
    }
    if( (enc & SQLITE_UTF16_ALIGNED)!=0 && 1==(1&SQLITE_PTR_TO_INT(pVal->z)) ){
      assert( (pVal->flags & (MEM_Ephem|MEM_Static))!=0 );
      if( sqlite3VdbeMemMakeWriteable(pVal)!=SQLITE_OK ){
        return 0;
      }
    }
    sqlite3VdbeMemNulTerminate(pVal); /* IMP: R-31275-44060 */
  }else{
    sqlite3VdbeMemStringify(pVal, enc, 0);
    assert( 0==(1&SQLITE_PTR_TO_INT(pVal->z)) );
  }
  assert(pVal->enc==(enc & ~SQLITE_UTF16_ALIGNED) || pVal->db==0
              || pVal->db->mallocFailed );
  if( pVal->enc==(enc & ~SQLITE_UTF16_ALIGNED) ){
    assert( sqlite3VdbeMemValidStrRep(pVal) );
    return pVal->z;
  }else{
    return 0;
  }
}

/* This function is only available internally, it is not part of the
** external API. It works in a similar way to sqlite3_value_text(),
** except the data returned is in the encoding specified by the second
** parameter, which must be one of SQLITE_UTF16BE, SQLITE_UTF16LE or
** SQLITE_UTF8.
**
** (2006-02-16:)  The enc value can be or-ed with SQLITE_UTF16_ALIGNED.
** If that is the case, then the result must be aligned on an even byte
** boundary.
*/
SQLITE_PRIVATE const void *sqlite3ValueText(sqlite3_value* pVal, u8 enc){
  if( !pVal ) return 0;
  assert( pVal->db==0 || sqlite3_mutex_held(pVal->db->mutex) );
  assert( (enc&3)==(enc&~SQLITE_UTF16_ALIGNED) );
  assert( !sqlite3VdbeMemIsRowSet(pVal) );
  if( (pVal->flags&(MEM_Str|MEM_Term))==(MEM_Str|MEM_Term) && pVal->enc==enc ){
    assert( sqlite3VdbeMemValidStrRep(pVal) );
    return pVal->z;
  }
  if( pVal->flags&MEM_Null ){
    return 0;
  }
  return valueToText(pVal, enc);
}

/* Return true if sqlit3_value object pVal is a string or blob value
** that uses the destructor specified in the second argument.
**
** TODO:  Maybe someday promote this interface into a published API so
** that third-party extensions can get access to it?
*/
SQLITE_PRIVATE int sqlite3ValueIsOfClass(const sqlite3_value *pVal, void(*xFree)(void*)){
  if( ALWAYS(pVal!=0)
   && ALWAYS((pVal->flags & (MEM_Str|MEM_Blob))!=0)
   && (pVal->flags & MEM_Dyn)!=0
   && pVal->xDel==xFree
  ){
    return 1;
  }else{
    return 0;
  }
}

/*
** Create a new sqlite3_value object.
*/
SQLITE_PRIVATE sqlite3_value *sqlite3ValueNew(sqlite3 *db){
  Mem *p = sqlite3DbMallocZero(db, sizeof(*p));
  if( p ){
    p->flags = MEM_Null;
    p->db = db;
  }
  return p;
}

/*
** Context object passed by sqlite3Stat4ProbeSetValue() through to
** valueNew(). See comments above valueNew() for details.
*/
struct ValueNewStat4Ctx {
  Parse *pParse;
  Index *pIdx;
  UnpackedRecord **ppRec;
  int iVal;
};

/*
** Allocate and return a pointer to a new sqlite3_value object. If
** the second argument to this function is NULL, the object is allocated
** by calling sqlite3ValueNew().
**
** Otherwise, if the second argument is non-zero, then this function is
** being called indirectly by sqlite3Stat4ProbeSetValue(). If it has not
** already been allocated, allocate the UnpackedRecord structure that
** that function will return to its caller here. Then return a pointer to
** an sqlite3_value within the UnpackedRecord.a[] array.
*/
static sqlite3_value *valueNew(sqlite3 *db, struct ValueNewStat4Ctx *p){
#ifdef SQLITE_ENABLE_STAT4
  if( p ){
    UnpackedRecord *pRec = p->ppRec[0];

    if( pRec==0 ){
      Index *pIdx = p->pIdx;      /* Index being probed */
      int nByte;                  /* Bytes of space to allocate */
      int i;                      /* Counter variable */
      int nCol = pIdx->nColumn;   /* Number of index columns including rowid */

      nByte = sizeof(Mem) * nCol + ROUND8(sizeof(UnpackedRecord));
      pRec = (UnpackedRecord*)sqlite3DbMallocZero(db, nByte);
      if( pRec ){
        pRec->pKeyInfo = sqlite3KeyInfoOfIndex(p->pParse, pIdx);
        if( pRec->pKeyInfo ){
          assert( pRec->pKeyInfo->nAllField==nCol );
          assert( pRec->pKeyInfo->enc==ENC(db) );
          pRec->aMem = (Mem *)((u8*)pRec + ROUND8(sizeof(UnpackedRecord)));
          for(i=0; i<nCol; i++){
