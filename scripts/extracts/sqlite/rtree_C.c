/* SQLite 3.49.1 — L216215-216340 — rtreeCheckNode B: bounding box + rtreeCheckTable */
            pCheck->nLeaf++;
          }
        }
      }
    }
    sqlite3_free(aNode);
  }
}

/*
** The second argument to this function must be either "_rowid" or
** "_parent". This function checks that the number of entries in the
** %_rowid or %_parent table is exactly nExpect. If not, it adds
** an error message to the report in the RtreeCheck object indicated
** by the first argument.
*/
static void rtreeCheckCount(RtreeCheck *pCheck, const char *zTbl, i64 nExpect){
  if( pCheck->rc==SQLITE_OK ){
    sqlite3_stmt *pCount;
    pCount = rtreeCheckPrepare(pCheck, "SELECT count(*) FROM %Q.'%q%s'",
        pCheck->zDb, pCheck->zTab, zTbl
    );
    if( pCount ){
      if( sqlite3_step(pCount)==SQLITE_ROW ){
        i64 nActual = sqlite3_column_int64(pCount, 0);
        if( nActual!=nExpect ){
          rtreeCheckAppendMsg(pCheck, "Wrong number of entries in %%%s table"
              " - expected %lld, actual %lld" , zTbl, nExpect, nActual
          );
        }
      }
      pCheck->rc = sqlite3_finalize(pCount);
    }
  }
}

/*
** This function does the bulk of the work for the rtree integrity-check.
** It is called by rtreecheck(), which is the SQL function implementation.
*/
static int rtreeCheckTable(
  sqlite3 *db,                    /* Database handle to access db through */
  const char *zDb,                /* Name of db ("main", "temp" etc.) */
  const char *zTab,               /* Name of rtree table to check */
  char **pzReport                 /* OUT: sqlite3_malloc'd report text */
){
  RtreeCheck check;               /* Common context for various routines */
  sqlite3_stmt *pStmt = 0;        /* Used to find column count of rtree table */
  int nAux = 0;                   /* Number of extra columns. */

  /* Initialize the context object */
  memset(&check, 0, sizeof(check));
  check.db = db;
  check.zDb = zDb;
  check.zTab = zTab;

  /* Find the number of auxiliary columns */
  pStmt = rtreeCheckPrepare(&check, "SELECT * FROM %Q.'%q_rowid'", zDb, zTab);
  if( pStmt ){
    nAux = sqlite3_column_count(pStmt) - 2;
    sqlite3_finalize(pStmt);
  }else
  if( check.rc!=SQLITE_NOMEM ){
    check.rc = SQLITE_OK;
  }

  /* Find number of dimensions in the rtree table. */
  pStmt = rtreeCheckPrepare(&check, "SELECT * FROM %Q.%Q", zDb, zTab);
  if( pStmt ){
    int rc;
    check.nDim = (sqlite3_column_count(pStmt) - 1 - nAux) / 2;
    if( check.nDim<1 ){
      rtreeCheckAppendMsg(&check, "Schema corrupt or not an rtree");
    }else if( SQLITE_ROW==sqlite3_step(pStmt) ){
      check.bInt = (sqlite3_column_type(pStmt, 1)==SQLITE_INTEGER);
    }
    rc = sqlite3_finalize(pStmt);
    if( rc!=SQLITE_CORRUPT ) check.rc = rc;
  }

  /* Do the actual integrity-check */
  if( check.nDim>=1 ){
    if( check.rc==SQLITE_OK ){
      rtreeCheckNode(&check, 0, 0, 1);
    }
    rtreeCheckCount(&check, "_rowid", check.nLeaf);
    rtreeCheckCount(&check, "_parent", check.nNonLeaf);
  }

  /* Finalize SQL statements used by the integrity-check */
  sqlite3_finalize(check.pGetNode);
  sqlite3_finalize(check.aCheckMapping[0]);
  sqlite3_finalize(check.aCheckMapping[1]);

  *pzReport = check.zReport;
  return check.rc;
}

/*
** Implementation of the xIntegrity method for Rtree.
*/
static int rtreeIntegrity(
  sqlite3_vtab *pVtab,   /* The virtual table to check */
  const char *zSchema,   /* Schema in which the virtual table lives */
  const char *zName,     /* Name of the virtual table */
  int isQuick,           /* True for a quick_check */
  char **pzErr           /* Write results here */
){
  Rtree *pRtree = (Rtree*)pVtab;
  int rc;
  assert( pzErr!=0 && *pzErr==0 );
  UNUSED_PARAMETER(zSchema);
  UNUSED_PARAMETER(zName);
  UNUSED_PARAMETER(isQuick);
  rc = rtreeCheckTable(pRtree->db, pRtree->zDb, pRtree->zName, pzErr);
  if( rc==SQLITE_OK && *pzErr ){
    *pzErr = sqlite3_mprintf("In RTree %s.%s:\n%z",
                 pRtree->zDb, pRtree->zName, *pzErr);
    if( (*pzErr)==0 ) rc = SQLITE_NOMEM;
  }
  return rc;
}

/*
** Usage:
**
