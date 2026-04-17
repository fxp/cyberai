/* SQLite 3.49.1 — L216100-216215 — rtreeCheckNode A: R-Tree node consistency + child walk */
    if( ii!=iVal ){
      rtreeCheckAppendMsg(pCheck,
          "Found (%lld -> %lld) in %s table, expected (%lld -> %lld)",
          iKey, ii, (bLeaf ? "%_rowid" : "%_parent"), iKey, iVal
      );
    }
  }
  rtreeCheckReset(pCheck, pStmt);
}

/*
** Argument pCell points to an array of coordinates stored on an rtree page.
** This function checks that the coordinates are internally consistent (no
** x1>x2 conditions) and adds an error message to the RtreeCheck object
** if they are not.
**
** Additionally, if pParent is not NULL, then it is assumed to point to
** the array of coordinates on the parent page that bound the page
** containing pCell. In this case it is also verified that the two
** sets of coordinates are mutually consistent and an error message added
** to the RtreeCheck object if they are not.
*/
static void rtreeCheckCellCoord(
  RtreeCheck *pCheck,
  i64 iNode,                      /* Node id to use in error messages */
  int iCell,                      /* Cell number to use in error messages */
  u8 *pCell,                      /* Pointer to cell coordinates */
  u8 *pParent                     /* Pointer to parent coordinates */
){
  RtreeCoord c1, c2;
  RtreeCoord p1, p2;
  int i;

  for(i=0; i<pCheck->nDim; i++){
    readCoord(&pCell[4*2*i], &c1);
    readCoord(&pCell[4*(2*i + 1)], &c2);

    /* printf("%e, %e\n", c1.u.f, c2.u.f); */
    if( pCheck->bInt ? c1.i>c2.i : c1.f>c2.f ){
      rtreeCheckAppendMsg(pCheck,
          "Dimension %d of cell %d on node %lld is corrupt", i, iCell, iNode
      );
    }

    if( pParent ){
      readCoord(&pParent[4*2*i], &p1);
      readCoord(&pParent[4*(2*i + 1)], &p2);

      if( (pCheck->bInt ? c1.i<p1.i : c1.f<p1.f)
       || (pCheck->bInt ? c2.i>p2.i : c2.f>p2.f)
      ){
        rtreeCheckAppendMsg(pCheck,
            "Dimension %d of cell %d on node %lld is corrupt relative to parent"
            , i, iCell, iNode
        );
      }
    }
  }
}

/*
** Run rtreecheck() checks on node iNode, which is at depth iDepth within
** the r-tree structure. Argument aParent points to the array of coordinates
** that bound node iNode on the parent node.
**
** If any problems are discovered, an error message is appended to the
** report accumulated in the RtreeCheck object.
*/
static void rtreeCheckNode(
  RtreeCheck *pCheck,
  int iDepth,                     /* Depth of iNode (0==leaf) */
  u8 *aParent,                    /* Buffer containing parent coords */
  i64 iNode                       /* Node to check */
){
  u8 *aNode = 0;
  int nNode = 0;

  assert( iNode==1 || aParent!=0 );
  assert( pCheck->nDim>0 );

  aNode = rtreeCheckGetNode(pCheck, iNode, &nNode);
  if( aNode ){
    if( nNode<4 ){
      rtreeCheckAppendMsg(pCheck,
          "Node %lld is too small (%d bytes)", iNode, nNode
      );
    }else{
      int nCell;                  /* Number of cells on page */
      int i;                      /* Used to iterate through cells */
      if( aParent==0 ){
        iDepth = readInt16(aNode);
        if( iDepth>RTREE_MAX_DEPTH ){
          rtreeCheckAppendMsg(pCheck, "Rtree depth out of range (%d)", iDepth);
          sqlite3_free(aNode);
          return;
        }
      }
      nCell = readInt16(&aNode[2]);
      if( (4 + nCell*(8 + pCheck->nDim*2*4))>nNode ){
        rtreeCheckAppendMsg(pCheck,
            "Node %lld is too small for cell count of %d (%d bytes)",
            iNode, nCell, nNode
        );
      }else{
        for(i=0; i<nCell; i++){
          u8 *pCell = &aNode[4 + i*(8 + pCheck->nDim*2*4)];
          i64 iVal = readInt64(pCell);
          rtreeCheckCellCoord(pCheck, iNode, i, &pCell[8], aParent);

          if( iDepth>0 ){
            rtreeCheckMapping(pCheck, 0, iVal, iNode);
            rtreeCheckNode(pCheck, iDepth-1, &pCell[8], iVal);
            pCheck->nNonLeaf++;
          }else{
            rtreeCheckMapping(pCheck, 1, iVal, iNode);
            pCheck->nLeaf++;
