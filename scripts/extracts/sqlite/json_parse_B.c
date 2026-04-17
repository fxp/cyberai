/* SQLite 3.49.1 — L207070-207230 — jsonParseFunc: JSON tree building + node allocation */
** linger, it needs to increment the nPJRef reference counter.
*/
static JsonParse *jsonCacheSearch(
  sqlite3_context *ctx,    /* The SQL statement context holding the cache */
  sqlite3_value *pArg      /* Function argument containing SQL text */
){
  JsonCache *p;
  int i;
  const char *zJson;
  int nJson;

  if( sqlite3_value_type(pArg)!=SQLITE_TEXT ){
    return 0;
  }
  zJson = (const char*)sqlite3_value_text(pArg);
  if( zJson==0 ) return 0;
  nJson = sqlite3_value_bytes(pArg);

  p = sqlite3_get_auxdata(ctx, JSON_CACHE_ID);
  if( p==0 ){
    return 0;
  }
  for(i=0; i<p->nUsed; i++){
    if( p->a[i]->zJson==zJson ) break;
  }
  if( i>=p->nUsed ){
    for(i=0; i<p->nUsed; i++){
      if( p->a[i]->nJson!=nJson ) continue;
      if( memcmp(p->a[i]->zJson, zJson, nJson)==0 ) break;
    }
  }
  if( i<p->nUsed ){
    if( i<p->nUsed-1 ){
      /* Make the matching entry the most recently used entry */
      JsonParse *tmp = p->a[i];
      memmove(&p->a[i], &p->a[i+1], (p->nUsed-i-1)*sizeof(tmp));
      p->a[p->nUsed-1] = tmp;
      i = p->nUsed - 1;
    }
    assert( p->a[i]->delta==0 );
    return p->a[i];
  }else{
    return 0;
  }
}

/**************************************************************************
** Utility routines for dealing with JsonString objects
**************************************************************************/

/* Turn uninitialized bulk memory into a valid JsonString object
** holding a zero-length string.
*/
static void jsonStringZero(JsonString *p){
  p->zBuf = p->zSpace;
  p->nAlloc = sizeof(p->zSpace);
  p->nUsed = 0;
  p->bStatic = 1;
}

/* Initialize the JsonString object
*/
static void jsonStringInit(JsonString *p, sqlite3_context *pCtx){
  p->pCtx = pCtx;
  p->eErr = 0;
  jsonStringZero(p);
}

/* Free all allocated memory and reset the JsonString object back to its
** initial state.
*/
static void jsonStringReset(JsonString *p){
  if( !p->bStatic ) sqlite3RCStrUnref(p->zBuf);
  jsonStringZero(p);
}

/* Report an out-of-memory (OOM) condition
*/
static void jsonStringOom(JsonString *p){
  p->eErr |= JSTRING_OOM;
  if( p->pCtx ) sqlite3_result_error_nomem(p->pCtx);
  jsonStringReset(p);
}

/* Enlarge pJson->zBuf so that it can hold at least N more bytes.
** Return zero on success.  Return non-zero on an OOM error
*/
static int jsonStringGrow(JsonString *p, u32 N){
  u64 nTotal = N<p->nAlloc ? p->nAlloc*2 : p->nAlloc+N+10;
  char *zNew;
  if( p->bStatic ){
    if( p->eErr ) return 1;
    zNew = sqlite3RCStrNew(nTotal);
    if( zNew==0 ){
      jsonStringOom(p);
      return SQLITE_NOMEM;
    }
    memcpy(zNew, p->zBuf, (size_t)p->nUsed);
    p->zBuf = zNew;
    p->bStatic = 0;
  }else{
    p->zBuf = sqlite3RCStrResize(p->zBuf, nTotal);
    if( p->zBuf==0 ){
      p->eErr |= JSTRING_OOM;
      jsonStringZero(p);
      return SQLITE_NOMEM;
    }
  }
  p->nAlloc = nTotal;
  return SQLITE_OK;
}

/* Append N bytes from zIn onto the end of the JsonString string.
*/
static SQLITE_NOINLINE void jsonStringExpandAndAppend(
  JsonString *p,
  const char *zIn,
  u32 N
){
  assert( N>0 );
  if( jsonStringGrow(p,N) ) return;
  memcpy(p->zBuf+p->nUsed, zIn, N);
  p->nUsed += N;
}
static void jsonAppendRaw(JsonString *p, const char *zIn, u32 N){
  if( N==0 ) return;
  if( N+p->nUsed >= p->nAlloc ){
    jsonStringExpandAndAppend(p,zIn,N);
  }else{
    memcpy(p->zBuf+p->nUsed, zIn, N);
    p->nUsed += N;
  }
}
static void jsonAppendRawNZ(JsonString *p, const char *zIn, u32 N){
  assert( N>0 );
  if( N+p->nUsed >= p->nAlloc ){
    jsonStringExpandAndAppend(p,zIn,N);
  }else{
    memcpy(p->zBuf+p->nUsed, zIn, N);
    p->nUsed += N;
  }
}

/* Append formatted text (not to exceed N bytes) to the JsonString.
*/
static void jsonPrintf(int N, JsonString *p, const char *zFormat, ...){
  va_list ap;
  if( (p->nUsed + N >= p->nAlloc) && jsonStringGrow(p, N) ) return;
  va_start(ap, zFormat);
  sqlite3_vsnprintf(N, p->zBuf+p->nUsed, zFormat, ap);
  va_end(ap);
  p->nUsed += (int)strlen(p->zBuf+p->nUsed);
}

/* Append a single character
*/
static SQLITE_NOINLINE void jsonAppendCharExpand(JsonString *p, char c){
  if( jsonStringGrow(p,1) ) return;
  p->zBuf[p->nUsed++] = c;
}
static void jsonAppendChar(JsonString *p, char c){
