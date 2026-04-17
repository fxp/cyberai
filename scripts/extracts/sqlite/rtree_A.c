/* SQLite 3.49.1 — L215830-215960 — rtreenode: R-Tree node parser (CVE-2019-8457 class) */
rtreeInit_fail:
  if( rc==SQLITE_OK ) rc = SQLITE_ERROR;
  assert( *ppVtab==0 );
  assert( pRtree->nBusy==1 );
  rtreeRelease(pRtree);
  return rc;
}


/*
** Implementation of a scalar function that decodes r-tree nodes to
** human readable strings. This can be used for debugging and analysis.
**
** The scalar function takes two arguments: (1) the number of dimensions
** to the rtree (between 1 and 5, inclusive) and (2) a blob of data containing
** an r-tree node.  For a two-dimensional r-tree structure called "rt", to
** deserialize all nodes, a statement like:
**
**   SELECT rtreenode(2, data) FROM rt_node;
**
** The human readable string takes the form of a Tcl list with one
** entry for each cell in the r-tree node. Each entry is itself a
** list, containing the 8-byte rowid/pageno followed by the
** <num-dimension>*2 coordinates.
*/
static void rtreenode(sqlite3_context *ctx, int nArg, sqlite3_value **apArg){
  RtreeNode node;
  Rtree tree;
  int ii;
  int nData;
  int errCode;
  sqlite3_str *pOut;

  UNUSED_PARAMETER(nArg);
  memset(&node, 0, sizeof(RtreeNode));
  memset(&tree, 0, sizeof(Rtree));
  tree.nDim = (u8)sqlite3_value_int(apArg[0]);
  if( tree.nDim<1 || tree.nDim>5 ) return;
  tree.nDim2 = tree.nDim*2;
  tree.nBytesPerCell = 8 + 8 * tree.nDim;
  node.zData = (u8 *)sqlite3_value_blob(apArg[1]);
  if( node.zData==0 ) return;
  nData = sqlite3_value_bytes(apArg[1]);
  if( nData<4 ) return;
  if( nData<NCELL(&node)*tree.nBytesPerCell ) return;

  pOut = sqlite3_str_new(0);
  for(ii=0; ii<NCELL(&node); ii++){
    RtreeCell cell;
    int jj;

    nodeGetCell(&tree, &node, ii, &cell);
    if( ii>0 ) sqlite3_str_append(pOut, " ", 1);
    sqlite3_str_appendf(pOut, "{%lld", cell.iRowid);
    for(jj=0; jj<tree.nDim2; jj++){
#ifndef SQLITE_RTREE_INT_ONLY
      sqlite3_str_appendf(pOut, " %g", (double)cell.aCoord[jj].f);
#else
      sqlite3_str_appendf(pOut, " %d", cell.aCoord[jj].i);
#endif
    }
    sqlite3_str_append(pOut, "}", 1);
  }
  errCode = sqlite3_str_errcode(pOut);
  sqlite3_result_error_code(ctx, errCode);
  sqlite3_result_text(ctx, sqlite3_str_finish(pOut), -1, sqlite3_free);
}

/* This routine implements an SQL function that returns the "depth" parameter
** from the front of a blob that is an r-tree node.  For example:
**
**     SELECT rtreedepth(data) FROM rt_node WHERE nodeno=1;
**
** The depth value is 0 for all nodes other than the root node, and the root
** node always has nodeno=1, so the example above is the primary use for this
** routine.  This routine is intended for testing and analysis only.
*/
static void rtreedepth(sqlite3_context *ctx, int nArg, sqlite3_value **apArg){
  UNUSED_PARAMETER(nArg);
  if( sqlite3_value_type(apArg[0])!=SQLITE_BLOB
   || sqlite3_value_bytes(apArg[0])<2

  ){
    sqlite3_result_error(ctx, "Invalid argument to rtreedepth()", -1);
  }else{
    u8 *zBlob = (u8 *)sqlite3_value_blob(apArg[0]);
    if( zBlob ){
      sqlite3_result_int(ctx, readInt16(zBlob));
    }else{
      sqlite3_result_error_nomem(ctx);
    }
  }
}

/*
** Context object passed between the various routines that make up the
** implementation of integrity-check function rtreecheck().
*/
typedef struct RtreeCheck RtreeCheck;
struct RtreeCheck {
  sqlite3 *db;                    /* Database handle */
  const char *zDb;                /* Database containing rtree table */
  const char *zTab;               /* Name of rtree table */
  int bInt;                       /* True for rtree_i32 table */
  int nDim;                       /* Number of dimensions for this rtree tbl */
  sqlite3_stmt *pGetNode;         /* Statement used to retrieve nodes */
  sqlite3_stmt *aCheckMapping[2]; /* Statements to query %_parent/%_rowid */
  int nLeaf;                      /* Number of leaf cells in table */
  int nNonLeaf;                   /* Number of non-leaf cells in table */
  int rc;                         /* Return code */
  char *zReport;                  /* Message to report */
  int nErr;                       /* Number of lines in zReport */
};

#define RTREE_CHECK_MAX_ERROR 100

/*
** Reset SQL statement pStmt. If the sqlite3_reset() call returns an error,
** and RtreeCheck.rc==SQLITE_OK, set RtreeCheck.rc to the error code.
*/
static void rtreeCheckReset(RtreeCheck *pCheck, sqlite3_stmt *pStmt){
  int rc = sqlite3_reset(pStmt);
  if( pCheck->rc==SQLITE_OK ) pCheck->rc = rc;
}

/*
** The second and subsequent arguments to this function are a format string
** and printf style arguments. This function formats the string and attempts
** to compile it as an SQL statement.
**
** If successful, a pointer to the new SQL statement is returned. Otherwise,
