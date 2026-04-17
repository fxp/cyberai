/* SQLite 3.49.1 — L172700-172775 — windowAggStep A: window-frame advance (CVE-2019-5018 class) */
** Generate VM code to read the window frames peer values from cursor csr into
** an array of registers starting at reg.
*/
static void windowReadPeerValues(
  WindowCodeArg *p,
  int csr,
  int reg
){
  Window *pMWin = p->pMWin;
  ExprList *pOrderBy = pMWin->pOrderBy;
  if( pOrderBy ){
    Vdbe *v = sqlite3GetVdbe(p->pParse);
    ExprList *pPart = pMWin->pPartition;
    int iColOff = pMWin->nBufferCol + (pPart ? pPart->nExpr : 0);
    int i;
    for(i=0; i<pOrderBy->nExpr; i++){
      sqlite3VdbeAddOp3(v, OP_Column, csr, iColOff+i, reg+i);
    }
  }
}

/*
** Generate VM code to invoke either xStep() (if bInverse is 0) or
** xInverse (if bInverse is non-zero) for each window function in the
** linked list starting at pMWin. Or, for built-in window functions
** that do not use the standard function API, generate the required
** inline VM code.
**
** If argument csr is greater than or equal to 0, then argument reg is
** the first register in an array of registers guaranteed to be large
** enough to hold the array of arguments for each function. In this case
** the arguments are extracted from the current row of csr into the
** array of registers before invoking OP_AggStep or OP_AggInverse
**
** Or, if csr is less than zero, then the array of registers at reg is
** already populated with all columns from the current row of the sub-query.
**
** If argument regPartSize is non-zero, then it is a register containing the
** number of rows in the current partition.
*/
static void windowAggStep(
  WindowCodeArg *p,
  Window *pMWin,                  /* Linked list of window functions */
  int csr,                        /* Read arguments from this cursor */
  int bInverse,                   /* True to invoke xInverse instead of xStep */
  int reg                         /* Array of registers */
){
  Parse *pParse = p->pParse;
  Vdbe *v = sqlite3GetVdbe(pParse);
  Window *pWin;
  for(pWin=pMWin; pWin; pWin=pWin->pNextWin){
    FuncDef *pFunc = pWin->pWFunc;
    int regArg;
    int nArg = pWin->bExprArgs ? 0 : windowArgCount(pWin);
    int i;
    int addrIf = 0;

    assert( bInverse==0 || pWin->eStart!=TK_UNBOUNDED );

    /* All OVER clauses in the same window function aggregate step must
    ** be the same. */
    assert( pWin==pMWin || sqlite3WindowCompare(pParse,pWin,pMWin,0)!=1 );

    for(i=0; i<nArg; i++){
      if( i!=1 || pFunc->zName!=nth_valueName ){
        sqlite3VdbeAddOp3(v, OP_Column, csr, pWin->iArgCol+i, reg+i);
      }else{
        sqlite3VdbeAddOp3(v, OP_Column, pMWin->iEphCsr, pWin->iArgCol+i, reg+i);
      }
    }
    regArg = reg;

    if( pWin->pFilter ){
      int regTmp;
      assert( ExprUseXList(pWin->pOwner) );
      assert( pWin->bExprArgs || !nArg ||nArg==pWin->pOwner->x.pList->nExpr );
