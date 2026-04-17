/* SQLite 3.49.1 amalgamation — L166635-166745 — whereLoopAddBtreeIndex B: nRow/nOut arithmetic */
        || (pNew->wsFlags & WHERE_COLUMN_IN)!=0
        || (pNew->wsFlags & WHERE_SKIPSCAN)!=0
    );

    if( eOp & WO_IN ){
      Expr *pExpr = pTerm->pExpr;
      if( ExprUseXSelect(pExpr) ){
        /* "x IN (SELECT ...)":  TUNING: the SELECT returns 25 rows */
        int i;
        nIn = 46;  assert( 46==sqlite3LogEst(25) );

        /* The expression may actually be of the form (x, y) IN (SELECT...).
        ** In this case there is a separate term for each of (x) and (y).
        ** However, the nIn multiplier should only be applied once, not once
        ** for each such term. The following loop checks that pTerm is the
        ** first such term in use, and sets nIn back to 0 if it is not. */
        for(i=0; i<pNew->nLTerm-1; i++){
          if( pNew->aLTerm[i] && pNew->aLTerm[i]->pExpr==pExpr ) nIn = 0;
        }
      }else if( ALWAYS(pExpr->x.pList && pExpr->x.pList->nExpr) ){
        /* "x IN (value, value, ...)" */
        nIn = sqlite3LogEst(pExpr->x.pList->nExpr);
      }
      if( pProbe->hasStat1 && rLogSize>=10 ){
        LogEst M, logK, x;
        /* Let:
        **   N = the total number of rows in the table
        **   K = the number of entries on the RHS of the IN operator
        **   M = the number of rows in the table that match terms to the
        **       to the left in the same index.  If the IN operator is on
        **       the left-most index column, M==N.
        **
        ** Given the definitions above, it is better to omit the IN operator
        ** from the index lookup and instead do a scan of the M elements,
        ** testing each scanned row against the IN operator separately, if:
        **
        **        M*log(K) < K*log(N)
        **
        ** Our estimates for M, K, and N might be inaccurate, so we build in
        ** a safety margin of 2 (LogEst: 10) that favors using the IN operator
        ** with the index, as using an index has better worst-case behavior.
        ** If we do not have real sqlite_stat1 data, always prefer to use
        ** the index.  Do not bother with this optimization on very small
        ** tables (less than 2 rows) as it is pointless in that case.
        */
        M = pProbe->aiRowLogEst[saved_nEq];
        logK = estLog(nIn);
        /* TUNING      v-----  10 to bias toward indexed IN */
        x = M + logK + 10 - (nIn + rLogSize);
        if( x>=0 ){
          WHERETRACE(0x40,
            ("IN operator (N=%d M=%d logK=%d nIn=%d rLogSize=%d x=%d) "
             "prefers indexed lookup\n",
             saved_nEq, M, logK, nIn, rLogSize, x));
        }else if( nInMul<2 && OptimizationEnabled(db, SQLITE_SeekScan) ){
          WHERETRACE(0x40,
            ("IN operator (N=%d M=%d logK=%d nIn=%d rLogSize=%d x=%d"
             " nInMul=%d) prefers skip-scan\n",
             saved_nEq, M, logK, nIn, rLogSize, x, nInMul));
          pNew->wsFlags |= WHERE_IN_SEEKSCAN;
        }else{
          WHERETRACE(0x40,
            ("IN operator (N=%d M=%d logK=%d nIn=%d rLogSize=%d x=%d"
             " nInMul=%d) prefers normal scan\n",
             saved_nEq, M, logK, nIn, rLogSize, x, nInMul));
          continue;
        }
      }
      pNew->wsFlags |= WHERE_COLUMN_IN;
    }else if( eOp & (WO_EQ|WO_IS) ){
      int iCol = pProbe->aiColumn[saved_nEq];
      pNew->wsFlags |= WHERE_COLUMN_EQ;
      assert( saved_nEq==pNew->u.btree.nEq );
      if( iCol==XN_ROWID
       || (iCol>=0 && nInMul==0 && saved_nEq==pProbe->nKeyCol-1)
      ){
        if( iCol==XN_ROWID || pProbe->uniqNotNull
         || (pProbe->nKeyCol==1 && pProbe->onError && (eOp & WO_EQ))
        ){
          pNew->wsFlags |= WHERE_ONEROW;
        }else{
          pNew->wsFlags |= WHERE_UNQ_WANTED;
        }
      }
      if( scan.iEquiv>1 ) pNew->wsFlags |= WHERE_TRANSCONS;
    }else if( eOp & WO_ISNULL ){
      pNew->wsFlags |= WHERE_COLUMN_NULL;
    }else{
      int nVecLen = whereRangeVectorLen(
          pParse, pSrc->iCursor, pProbe, saved_nEq, pTerm
      );
      if( eOp & (WO_GT|WO_GE) ){
        testcase( eOp & WO_GT );
        testcase( eOp & WO_GE );
        pNew->wsFlags |= WHERE_COLUMN_RANGE|WHERE_BTM_LIMIT;
        pNew->u.btree.nBtm = nVecLen;
        pBtm = pTerm;
        pTop = 0;
        if( pTerm->wtFlags & TERM_LIKEOPT ){
          /* Range constraints that come from the LIKE optimization are
          ** always used in pairs. */
          pTop = &pTerm[1];
          assert( (pTop-(pTerm->pWC->a))<pTerm->pWC->nTerm );
          assert( pTop->wtFlags & TERM_LIKEOPT );
          assert( pTop->eOperator==WO_LT );
          if( whereLoopResize(db, pNew, pNew->nLTerm+1) ) break; /* OOM */
          pNew->aLTerm[pNew->nLTerm++] = pTop;
          pNew->wsFlags |= WHERE_TOP_LIMIT;
          pNew->u.btree.nTop = 1;
        }
      }else{
