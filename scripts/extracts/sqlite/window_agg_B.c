/* SQLite 3.49.1 — L172775-172880 — windowAggStep B: aggregate function call + return */
      assert( pWin->bExprArgs || !nArg ||nArg==pWin->pOwner->x.pList->nExpr );
      assert( pWin->bExprArgs || nArg  ||pWin->pOwner->x.pList==0 );
      regTmp = sqlite3GetTempReg(pParse);
      sqlite3VdbeAddOp3(v, OP_Column, csr, pWin->iArgCol+nArg,regTmp);
      addrIf = sqlite3VdbeAddOp3(v, OP_IfNot, regTmp, 0, 1);
      VdbeCoverage(v);
      sqlite3ReleaseTempReg(pParse, regTmp);
    }

    if( pMWin->regStartRowid==0
     && (pFunc->funcFlags & SQLITE_FUNC_MINMAX)
     && (pWin->eStart!=TK_UNBOUNDED)
    ){
      int addrIsNull = sqlite3VdbeAddOp1(v, OP_IsNull, regArg);
      VdbeCoverage(v);
      if( bInverse==0 ){
        sqlite3VdbeAddOp2(v, OP_AddImm, pWin->regApp+1, 1);
        sqlite3VdbeAddOp2(v, OP_SCopy, regArg, pWin->regApp);
        sqlite3VdbeAddOp3(v, OP_MakeRecord, pWin->regApp, 2, pWin->regApp+2);
        sqlite3VdbeAddOp2(v, OP_IdxInsert, pWin->csrApp, pWin->regApp+2);
      }else{
        sqlite3VdbeAddOp4Int(v, OP_SeekGE, pWin->csrApp, 0, regArg, 1);
        VdbeCoverageNeverTaken(v);
        sqlite3VdbeAddOp1(v, OP_Delete, pWin->csrApp);
        sqlite3VdbeJumpHere(v, sqlite3VdbeCurrentAddr(v)-2);
      }
      sqlite3VdbeJumpHere(v, addrIsNull);
    }else if( pWin->regApp ){
      assert( pWin->pFilter==0 );
      assert( pFunc->zName==nth_valueName
           || pFunc->zName==first_valueName
      );
      assert( bInverse==0 || bInverse==1 );
      sqlite3VdbeAddOp2(v, OP_AddImm, pWin->regApp+1-bInverse, 1);
    }else if( pFunc->xSFunc!=noopStepFunc ){
      if( pWin->bExprArgs ){
        int iOp = sqlite3VdbeCurrentAddr(v);
        int iEnd;

        assert( ExprUseXList(pWin->pOwner) );
        nArg = pWin->pOwner->x.pList->nExpr;
        regArg = sqlite3GetTempRange(pParse, nArg);
        sqlite3ExprCodeExprList(pParse, pWin->pOwner->x.pList, regArg, 0, 0);

        for(iEnd=sqlite3VdbeCurrentAddr(v); iOp<iEnd; iOp++){
          VdbeOp *pOp = sqlite3VdbeGetOp(v, iOp);
          if( pOp->opcode==OP_Column && pOp->p1==pMWin->iEphCsr ){
            pOp->p1 = csr;
          }
        }
      }
      if( pFunc->funcFlags & SQLITE_FUNC_NEEDCOLL ){
        CollSeq *pColl;
        assert( nArg>0 );
        assert( ExprUseXList(pWin->pOwner) );
        pColl = sqlite3ExprNNCollSeq(pParse, pWin->pOwner->x.pList->a[0].pExpr);
        sqlite3VdbeAddOp4(v, OP_CollSeq, 0,0,0, (const char*)pColl, P4_COLLSEQ);
      }
      sqlite3VdbeAddOp3(v, bInverse? OP_AggInverse : OP_AggStep,
                        bInverse, regArg, pWin->regAccum);
      sqlite3VdbeAppendP4(v, pFunc, P4_FUNCDEF);
      sqlite3VdbeChangeP5(v, (u16)nArg);
      if( pWin->bExprArgs ){
        sqlite3ReleaseTempRange(pParse, regArg, nArg);
      }
    }

    if( addrIf ) sqlite3VdbeJumpHere(v, addrIf);
  }
}

/*
** Values that may be passed as the second argument to windowCodeOp().
*/
#define WINDOW_RETURN_ROW 1
#define WINDOW_AGGINVERSE 2
#define WINDOW_AGGSTEP    3

/*
** Generate VM code to invoke either xValue() (bFin==0) or xFinalize()
** (bFin==1) for each window function in the linked list starting at
** pMWin. Or, for built-in window-functions that do not use the standard
** API, generate the equivalent VM code.
*/
static void windowAggFinal(WindowCodeArg *p, int bFin){
  Parse *pParse = p->pParse;
  Window *pMWin = p->pMWin;
  Vdbe *v = sqlite3GetVdbe(pParse);
  Window *pWin;

  for(pWin=pMWin; pWin; pWin=pWin->pNextWin){
    if( pMWin->regStartRowid==0
     && (pWin->pWFunc->funcFlags & SQLITE_FUNC_MINMAX)
     && (pWin->eStart!=TK_UNBOUNDED)
    ){
      sqlite3VdbeAddOp2(v, OP_Null, 0, pWin->regResult);
      sqlite3VdbeAddOp1(v, OP_Last, pWin->csrApp);
      VdbeCoverage(v);
      sqlite3VdbeAddOp3(v, OP_Column, pWin->csrApp, 0, pWin->regResult);
      sqlite3VdbeJumpHere(v, sqlite3VdbeCurrentAddr(v)-2);
    }else if( pWin->regApp ){
      assert( pMWin->regStartRowid==0 );
    }else{
      int nArg = windowArgCount(pWin);
      if( bFin ){
        sqlite3VdbeAddOp2(v, OP_AggFinal, pWin->regAccum, nArg);
