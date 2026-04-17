/* SQLite 3.49.1 — L180853-180960 — sqlite3RunParser: SQL tokenize+parse loop entry */
SQLITE_PRIVATE int sqlite3RunParser(Parse *pParse, const char *zSql){
  int nErr = 0;                   /* Number of errors encountered */
  void *pEngine;                  /* The LEMON-generated LALR(1) parser */
  int n = 0;                      /* Length of the next token token */
  int tokenType;                  /* type of the next token */
  int lastTokenParsed = -1;       /* type of the previous token */
  sqlite3 *db = pParse->db;       /* The database connection */
  int mxSqlLen;                   /* Max length of an SQL string */
  Parse *pParentParse = 0;        /* Outer parse context, if any */
#ifdef sqlite3Parser_ENGINEALWAYSONSTACK
  yyParser sEngine;    /* Space to hold the Lemon-generated Parser object */
#endif
  VVA_ONLY( u8 startedWithOom = db->mallocFailed );

  assert( zSql!=0 );
  mxSqlLen = db->aLimit[SQLITE_LIMIT_SQL_LENGTH];
  if( db->nVdbeActive==0 ){
    AtomicStore(&db->u1.isInterrupted, 0);
  }
  pParse->rc = SQLITE_OK;
  pParse->zTail = zSql;
#ifdef SQLITE_DEBUG
  if( db->flags & SQLITE_ParserTrace ){
    printf("parser: [[[%s]]]\n", zSql);
    sqlite3ParserTrace(stdout, "parser: ");
  }else{
    sqlite3ParserTrace(0, 0);
  }
#endif
#ifdef sqlite3Parser_ENGINEALWAYSONSTACK
  pEngine = &sEngine;
  sqlite3ParserInit(pEngine, pParse);
#else
  pEngine = sqlite3ParserAlloc(sqlite3Malloc, pParse);
  if( pEngine==0 ){
    sqlite3OomFault(db);
    return SQLITE_NOMEM_BKPT;
  }
#endif
  assert( pParse->pNewTable==0 );
  assert( pParse->pNewTrigger==0 );
  assert( pParse->nVar==0 );
  assert( pParse->pVList==0 );
  pParentParse = db->pParse;
  db->pParse = pParse;
  while( 1 ){
    n = sqlite3GetToken((u8*)zSql, &tokenType);
    mxSqlLen -= n;
    if( mxSqlLen<0 ){
      pParse->rc = SQLITE_TOOBIG;
      pParse->nErr++;
      break;
    }
#ifndef SQLITE_OMIT_WINDOWFUNC
    if( tokenType>=TK_WINDOW ){
      assert( tokenType==TK_SPACE || tokenType==TK_OVER || tokenType==TK_FILTER
           || tokenType==TK_ILLEGAL || tokenType==TK_WINDOW
           || tokenType==TK_QNUMBER || tokenType==TK_COMMENT
      );
#else
    if( tokenType>=TK_SPACE ){
      assert( tokenType==TK_SPACE || tokenType==TK_ILLEGAL
           || tokenType==TK_QNUMBER || tokenType==TK_COMMENT
      );
#endif /* SQLITE_OMIT_WINDOWFUNC */
      if( AtomicLoad(&db->u1.isInterrupted) ){
        pParse->rc = SQLITE_INTERRUPT;
        pParse->nErr++;
        break;
      }
      if( tokenType==TK_SPACE ){
        zSql += n;
        continue;
      }
      if( zSql[0]==0 ){
        /* Upon reaching the end of input, call the parser two more times
        ** with tokens TK_SEMI and 0, in that order. */
        if( lastTokenParsed==TK_SEMI ){
          tokenType = 0;
        }else if( lastTokenParsed==0 ){
          break;
        }else{
          tokenType = TK_SEMI;
        }
        n = 0;
#ifndef SQLITE_OMIT_WINDOWFUNC
      }else if( tokenType==TK_WINDOW ){
        assert( n==6 );
        tokenType = analyzeWindowKeyword((const u8*)&zSql[6]);
      }else if( tokenType==TK_OVER ){
        assert( n==4 );
        tokenType = analyzeOverKeyword((const u8*)&zSql[4], lastTokenParsed);
      }else if( tokenType==TK_FILTER ){
        assert( n==6 );
        tokenType = analyzeFilterKeyword((const u8*)&zSql[6], lastTokenParsed);
#endif /* SQLITE_OMIT_WINDOWFUNC */
      }else if( tokenType==TK_COMMENT && (db->flags & SQLITE_Comments)!=0 ){
        zSql += n;
        continue;
      }else if( tokenType!=TK_QNUMBER ){
        Token x;
        x.z = zSql;
        x.n = n;
        sqlite3ErrorMsg(pParse, "unrecognized token: \"%T\"", &x);
        break;
      }
    }
    pParse->sLastToken.z = zSql;
