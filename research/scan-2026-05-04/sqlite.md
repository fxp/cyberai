# sqlite — GLM-5.1 Pipeline A scan (2026-05-05)

- **Model**: glm-5.1
- **Segments**: 29 (0 hit GLM API timeout)
- **Total findings**: 66
  - 🔴 **CRITICAL**: 4
  - 🟠 **HIGH**: 14
  - 🟡 **MEDIUM**: 27
  - 🔵 **LOW**: 21

## Per-segment summary

| Segment | Time (s) | Findings | Status |
|---|---:|---:|---|
| `sqlite3.c [sqlite3_str_vappendf A: format-loop+width+precision]` | 221.8 | 3 | ✅ ok |
| `sqlite3.c [sqlite3_str_vappendf B: integer+radix flag setup]` | 235.7 | 3 | ✅ ok |
| `sqlite3.c [sqlite3_str_vappendf C: integer-to-string+buf sizing]` | 360.4 | 0 | ✅ ok |
| `sqlite3.c [sqlite3_str_vappendf D: numeric output+thousands sep]` | 360.4 | 0 | ✅ ok |
| `sqlite3.c [sqlite3_str_vappendf E: float+EXP+GENERIC]` | 360.5 | 0 | ✅ ok |
| `sqlite3.c [sqlite3_str_vappendf F: string+SQL-escape+TOKEN]` | 360.4 | 0 | ✅ ok |
| `sqlite3.c [sqlite3GetToken A: CC_ class switch method+blob+string]` | 360.5 | 0 | ✅ ok |
| `sqlite3.c [sqlite3GetToken B: keyword+ident+end-of-input]` | 360.5 | 0 | ✅ ok |
| `sqlite3.c [sqlite3RunParser: tokenize+parse loop entry]` | 337.0 | 4 | ✅ ok |
| `sqlite3.c [sqlite3VdbeSerialGet A: type decode + int serialization]` | 98.2 | 4 | ✅ ok |
| `sqlite3.c [sqlite3VdbeSerialGet B: string/blob+float types]` | 161.4 | 3 | ✅ ok |
| `sqlite3.c [sqlite3VdbeMemFromBtree: payload fetch with bounds check]` | 196.3 | 5 | ✅ ok |
| `sqlite3.c [sqlite3VdbeMemFromBtreeZeroOffset: direct page access]` | 176.0 | 3 | ✅ ok |
| `sqlite3.c [windowAggStep A: window-frame advance (CVE-2019-5018)]` | 71.3 | 4 | ✅ ok |
| `sqlite3.c [windowAggStep B: aggregate call + return]` | 360.4 | 0 | ✅ ok |
| `sqlite3.c [rtreenode: R-Tree node parser (CVE-2019-8457)]` | 57.2 | 1 | ✅ ok |
| `sqlite3.c [rtreeCheckNode A: R-Tree node consistency + child walk]` | 360.4 | 0 | ✅ ok |
| `sqlite3.c [rtreeCheckNode B: bounding box + rtreeCheckTable]` | 129.4 | 4 | ✅ ok |
| `sqlite3.c [sessionReadRecord A: session change record (CVE-2023-7104)]` | 360.4 | 0 | ✅ ok |
| `sqlite3.c [sessionApplyChange: session record applier]` | 265.0 | 4 | ✅ ok |
| `sqlite3.c [whereLoopAddBtreeIndex A: index-scan+cost (CVE-2019-16168)]` | 60.5 | 3 | ✅ ok |
| `sqlite3.c [whereLoopAddBtreeIndex B: nRow/nOut arithmetic]` | 178.6 | 3 | ✅ ok |
| `sqlite3.c [whereLoopAddBtreeIndex C: multi-column+IN operator]` | 199.5 | 4 | ✅ ok |
| `sqlite3.c [whereLoopAddBtreeIndex D: skip-scan+tail conditions]` | 296.3 | 3 | ✅ ok |
| `sqlite3.c [sqlite3AlterFinishAddColumn A: schema rebuild (CVE-2020-35527)]` | 53.2 | 2 | ✅ ok |
| `sqlite3.c [sqlite3AlterFinishAddColumn B: column validate+constraints]` | 360.4 | 0 | ✅ ok |
| `sqlite3.c [sqlite3AlterFinishAddColumn C: default+affinity+codegen]` | 263.4 | 4 | ✅ ok |
| `sqlite3.c [jsonParseFuncArg: JSON input validation+cache lookup]` | 255.2 | 4 | ✅ ok |
| `sqlite3.c [jsonParseFunc: JSON tree building+node allocation]` | 183.2 | 5 | ✅ ok |

## 🔴 CRITICAL (4)

### `sqlite3.c [rtreenode: R-Tree node parser (CVE-2019-8457)]` — L215862 — conf=0.95
**Heap Buffer Over-Read in rtreenode() due to Missing Header Offset in Bounds Check (CVE-2019-8457)**

The bounds check in rtreenode() fails to account for the 4-byte node header (2 bytes depth + 2 bytes cell count) when validating that the blob is large enough to contain all declared cells. The check `if( nData<NCELL(&node)*tree.nBytesPerCell ) return;` only ensures the blob is at least NCELL*nBytesPerCell bytes, but the actual cell data begins at offset 4 (after the header). The correct check should be `if( nData<4+NCELL(&node)*tree.nBytesPerCell ) return;`. An attacker can craft a blob where NCELL indicates cells exist and the blob size passes the insufficient check, but is too small to cont…

*PoC sketch:* For a 2-dimensional rtree (nDim=2), nBytesPerCell = 8 + 8*2 = 24. Craft a 24-byte blob where bytes 2-3 encode NCELL=1. The check evaluates: 24 < 1*24 → false, so execution continues. However, nodeGetCell reads 24 bytes starting at offset 4, requiring bytes 4-27 (28 bytes total). Since the blob is on…

### `sqlite3.c [sqlite3VdbeMemFromBtree: payload fetch with bounds check]` — L63 — conf=0.92
**Integer overflow in offset+amt bypasses bounds check in sqlite3VdbeMemFromBtree**

In sqlite3VdbeMemFromBtree, both `offset` and `amt` are u32. The expression `offset+amt` is computed as u32 arithmetic, which can wrap around to a small value. For example, offset=1 and amt=0xFFFFFFFF yields offset+amt=0, which passes the `sqlite3BtreeMaxRecordSize(pCur)<offset+amt` corruption check. This allows a maliciously crafted database record to bypass the bounds validation, leading to out-of-bounds reads and subsequent heap corruption.

*PoC sketch:* Craft a SQLite database file with a record whose serialized field size encodes amt=0xFFFFFFFF and an opcode that passes offset=1 to sqlite3VdbeMemFromBtree. The bounds check evaluates (1 + 0xFFFFFFFF) = 0 (u32 wrap), which is <= maxRecordSize, so the check passes. The function then proceeds to alloc…

### `sqlite3.c [sqlite3VdbeMemFromBtree: payload fetch with bounds check]` — L66 — conf=0.88
**Integer overflow in amt+1 causes undersized allocation and heap buffer overflow**

In sqlite3VdbeMemFromBtree, when amt=0xFFFFFFFF, the expression `amt+1` overflows to 0 (u32 wrap). This value is passed to sqlite3VdbeMemClearAndResize(pMem, 0), which either allocates a minimal/zero-size buffer or triggers undefined behavior (the function asserts szNew>0). Subsequently, sqlite3BtreePayload(pCur, offset, amt, pMem->z) attempts to write 0xFFFFFFFF bytes into the undersized buffer, and `pMem->z[amt] = 0` writes a null byte at offset 0xFFFFFFFF. This is a critical heap buffer overflow exploitable via a crafted database file.

*PoC sketch:* 1. Create a malicious SQLite database with a B-tree record containing a varint-encoded field length of 0xFFFFFFFF. 2. Trigger a VDBE opcode (e.g., OP_Column) that calls sqlite3VdbeMemFromBtree with amt=0xFFFFFFFF. 3. Combined with the offset+amt overflow (offset=1), the bounds check is bypassed. 4. …

### `sqlite3.c [sessionApplyChange: session record applier]` — L55 — conf=0.85
**Integer overflow in apVal allocation leading to heap buffer overflow on 32-bit systems**

On 32-bit systems, the expression `sizeof(apVal[0])*nCol*2` at line 55 can overflow to 0 when nCol is large (e.g., nCol=0x20000000). Since sizeof(apVal[0])=4 on 32-bit, 4*0x20000000*2 = 0x100000000 which wraps to 0 in 32-bit size_t. sqlite3_malloc64(0) may return a non-NULL pointer to a zero-size allocation. The subsequent memset at line 60 also overflows to 0 (no-op), bypassing initialization. Then sessionReadRecord writes to apVal[0] and apVal[nCol] (line 69/71), and the loops at lines 77-78 and 85-86 read apVal[iCol + nCol] — all massively out-of-bounds. The cleanup loop at line 90-91 calls…

*PoC sketch:* Craft a changeset with a 'T' record specifying nCol=0x20000000 (536870912), followed by an UPDATE record. On a 32-bit build: 1) The 'T' record sets nCol=0x20000000 and provides a minimal sPK array. 2) The UPDATE case computes sizeof(apVal[0])*nCol*2 = 4*0x20000000*2 = 0 (32-bit overflow). 3) sqlite3…


## 🟠 HIGH (14)

### `sqlite3.c [sqlite3VdbeSerialGet B: string/blob+float types]` — L48 — conf=0.88
**Unhandled serial types 10/11 cause integer underflow in length computation**

Serial types 10 and 11 are reserved/invalid in SQLite's record format but are not handled by any explicit case in the switch statement. They fall through to the `default` case which computes the data length as `(serial_type-12)/2`. Since `serial_type` is unsigned (u32), subtracting 12 from 10 or 11 causes an integer underflow: for serial_type=10, `(10-12)` wraps to `0xFFFFFFFE`, yielding `0x7FFFFFFF` (~2GB) after division. This enormous length is assigned to `pMem->n` while `pMem->z` points to a buffer containing far fewer bytes. When this Mem value is subsequently accessed (compared, copied, …

*PoC sketch:* Create a crafted SQLite database file where a record header contains a varint-encoded serial type of 10 or 11 (e.g., by directly modifying the database file bytes). When SQLite reads and unpacks this record via sqlite3VdbeRecordUnpack -> sqlite3VdbeSerialGet, the default case will set pMem->n to ~2G…

### `sqlite3.c [windowAggStep A: window-frame advance (CVE-2019-5018)]` — L8 — conf=0.85
**Out-of-bounds column read in windowReadPeerValues (CVE-2019-5018 class)**

In windowReadPeerValues, the column offset iColOff is computed as pMWin->nBufferCol + (pPart ? pPart->nExpr : 0), and then pOrderBy->nExpr columns are read starting at that offset via OP_Column(csr, iColOff+i, reg+i). There is no bounds check verifying that iColOff + pOrderBy->nExpr does not exceed the actual number of columns in cursor csr. A crafted SQL query with a window function whose ORDER BY clause, combined with partition and buffer column counts, exceeds the cursor's column count will cause the generated OP_Column instructions to read past the end of the record at runtime, resulting i…

*PoC sketch:* CREATE TABLE t1(a, b, c); INSERT INTO t1 VALUES(1,2,3),(4,5,6),(7,8,9); SELECT a, rank() OVER (PARTITION BY a ORDER BY b, c, a, b, c) FROM t1; -- A query where the combined nBufferCol + pPartition->nExpr + pOrderBy->nExpr exceeds the ephemeral cursor column count triggers the overread.

### `sqlite3.c [whereLoopAddBtreeIndex A: index-scan+cost (CVE-2019-16168)]` — L82 — conf=0.85
**Division-by-zero in index scan cost estimation (CVE-2019-16168 class)**

The variable rSize is derived from pProbe->aiRowLogEst[0], which holds a logarithmic row-count estimate for the index. When this value is zero or extremely small (e.g., due to corrupted sqlite_stat1 data, a newly created empty index, or a crafted database), subsequent cost calculations that use rSize and rLogSize as divisors can trigger a division by zero. The original CVE-2019-16168 was exactly this class of bug in whereLoopAddBtreeIndex: the query planner crashed when computing the cost of an index scan with a zero-row estimate. The code shown sets up rSize/rLogSize but the truncated portion…

*PoC sketch:* 1. Create a SQLite database with an index on a table. 2. Insert into sqlite_stat1 a row claiming the index has 0 rows (e.g., INSERT INTO sqlite_stat1 VALUES('t1','idx1','0');). 3. Run ANALYZE or let the planner read the corrupted stats. 4. Execute a query that uses the index with a range constraint,…

### `sqlite3.c [sqlite3AlterFinishAddColumn A: schema rebuild (CVE-2020-35527)]` — L117200 — conf=0.85
**Use-After-Free in sqlite3AlterFinishAddColumn during schema rebuild (CVE-2020-35527)**

The sqlite3AlterFinishAddColumn function performs an ALTER TABLE ADD COLUMN operation that triggers a schema rebuild. During this rebuild, the internal Table structure (pNew/pTab) obtained from pParse->pNewTable can be freed and reallocated by the schema reload mechanism (invoked via renameReloadSchema/renameTestSchema or the nested parse operations). However, the function continues to reference the stale pNew/pTab pointers after the schema has been invalidated. When the schema contains virtual tables, the sqlite3StrAccumAppendAll function may be used incorrectly during the rebuild, leading to…

*PoC sketch:* -- Create a database with a virtual table and a regular table CREATE VIRTUAL TABLE vt USING fts5(content); CREATE TABLE t1(a INTEGER PRIMARY KEY); -- Trigger the use-after-free by adding a column while the schema -- contains the virtual table, causing the rebuild to invalidate -- the Table* pointers…

### `sqlite3.c [jsonParseFunc: JSON tree building+node allocation]` — L207118 — conf=0.85
**Integer overflow in jsonStringGrow doubling logic leads to heap buffer overflow**

In jsonStringGrow(), the expression `p->nAlloc*2` is computed in u32 arithmetic before promotion to u64. When nAlloc (u32) reaches 0x80000000 or above, the doubling overflows to a small value (e.g., 0). This small value is assigned to u64 nTotal, causing sqlite3RCStrNew(nTotal) to allocate a tiny buffer. The subsequent memcpy(zNew, p->zBuf, (size_t)p->nUsed) then copies ~2GB+ of data into the undersized allocation, resulting in a critical heap buffer overflow. The same issue affects the second branch: `p->nAlloc+N+10` can also overflow in u32 arithmetic.

*PoC sketch:* Craft a JSON operation that causes repeated buffer doublings until nAlloc approaches 0x80000000 (requires ~2GB of JSON data, feasible if SQLITE_MAX_LENGTH is raised above default). The next append triggers jsonStringGrow with nAlloc=0x80000000, computing nTotal = 0x80000000*2 = 0 (u32 overflow), all…

### `sqlite3.c [sqlite3_str_vappendf B: integer+radix flag setup]` — L131 — conf=0.82
**Integer truncation in nOut assignment leads to heap buffer underflow**

When precision is large enough that the computed buffer size 'n' (u64) exceeds INT_MAX, the assignment 'nOut = (int)n' truncates the value, producing a negative nOut. This negative value is then used as an array index in 'bufpt = &zOut[nOut-1]', causing an out-of-bounds memory access before the allocated heap buffer. On systems with memory overcommit (e.g., Linux default), the preceding printfTempBuf allocation of ~2GB may succeed, making this exploitable. The subsequent digit-writing loop writes to bufpt and decrements it, corrupting heap metadata and adjacent data.

*PoC sketch:* Format string with very large precision on a decimal/radix conversion, e.g.: sqlite3_str_appendf(s, "%.2147483640d", 0); This sets precision=2147483640, n=(u64)2147483640+10=2147483650, nOut=(int)2147483650=-2147483646, bufpt=&zOut[-2147483647] — heap underflow. With cThousand (e.g., '%!' flag for r…

### `sqlite3.c [rtreeCheckNode B: bounding box + rtreeCheckTable]` — L216275 — conf=0.82
**Integer underflow in nAux leading to inflated nDim and heap buffer overread in rtreeCheckNode**

In rtreeCheckTable(), nAux is computed as `sqlite3_column_count(pStmt) - 2` from the _rowid table. If the _rowid table schema is corrupted and has fewer than 2 columns, nAux underflows to a negative value (e.g., -1 or -2). This negative nAux is then used in the nDim calculation: `check.nDim = (sqlite3_column_count(pStmt) - 1 - nAux) / 2`. A negative nAux inflates nDim beyond its true value. For example, a 2D rtree with 1 auxiliary column (6 columns in main table, 3 in _rowid) would normally compute nDim=2, but if _rowid is corrupted to have 1 column, nAux=-1 and nDim=(6-1+1)/2=3. The inflated …

*PoC sketch:* 1. Create an rtree table: CREATE VIRTUAL TABLE t1 USING rtree(id, x1, x2, y1, y2, +aux1); 2. Insert data. 3. Craft the database file by modifying the sqlite_master schema for the _rowid table to have fewer columns (e.g., remove columns so it has only 1 column). 4. Run integrity_check on the database…

### `sqlite3.c [sessionApplyChange: session record applier]` — L78 — conf=0.8
**NULL pointer dereference / use-of-uninitialized abPK when UPDATE precedes 'T' record**

The variable `abPK` is only assigned inside the 'T' case (line 31: `abPK = sPK.aBuf`). If a malformed changeset contains an UPDATE record before any 'T' record, abPK is either NULL (if zero-initialized) or uninitialized (if stack-allocated without initialization). At line 78, `abPK[iCol]` is dereferenced in the expression `apVal[iCol + (abPK[iCol] ? 0 : nCol)]`, causing a NULL pointer dereference crash (DoS) or reading from an arbitrary memory address (if abPK is uninitialized), potentially leaking memory contents. Similarly at line 86: `(abPK[iCol] ? 0 : apVal[iCol])`.

*PoC sketch:* Craft a changeset that starts directly with an UPDATE record (byte 0x17) without a preceding 'T' record. The changeset bytes: [0x17, 0x00, <update record data>]. When parsed, the UPDATE case is entered with abPK still NULL/uninitialized, and abPK[iCol] dereferences invalid memory, crashing the proce…

### `sqlite3.c [jsonParseFunc: JSON tree building+node allocation]` — L207140 — conf=0.8
**Integer overflow in jsonAppendRaw/jsonAppendRawNZ size check bypasses buffer growth**

In jsonAppendRaw() and jsonAppendRawNZ(), the bounds check `N+p->nUsed >= p->nAlloc` is performed in u32 arithmetic. When N and nUsed are both large u32 values whose sum exceeds 0xFFFFFFFF, the addition wraps around to a small value. If this wrapped value is less than nAlloc, the growth path is skipped and the else-branch memcpy(p->zBuf+p->nUsed, zIn, N) writes past the end of the allocated buffer, causing a heap buffer overflow.

*PoC sketch:* With nUsed=0xFFFFFF00 and N=0x200, the check computes 0xFFFFFF00+0x200=0x100 (u32 wrap). If nAlloc > 0x100, growth is skipped and memcpy writes 0x200 bytes at offset 0xFFFFFF00 in a buffer of size nAlloc, overflowing the heap.

### `sqlite3.c [sqlite3VdbeMemFromBtreeZeroOffset: direct page access]` — L84480 — conf=0.75
**Integer overflow in valueNew() leading to heap buffer overflow**

In valueNew(), the calculation `nByte = sizeof(Mem) * nCol + ROUND8(sizeof(UnpackedRecord))` can overflow when nCol (derived from pIdx->nColumn) is extremely large. sizeof(Mem) is typically 48-64 bytes on 64-bit systems. The multiplication is performed in size_t arithmetic, but the result is truncated when assigned to `int nByte`. If the truncated value is a small positive number, sqlite3DbMallocZero() allocates a small buffer, but the subsequent loop `for(i=0; i<nCol; i++)` writes nCol Mem structures far beyond the allocated heap buffer. nColumn is read from the database schema, so a crafted/…

*PoC sketch:* 1. Create a corrupted SQLite database file where an index's nColumn field is set to an extremely large value (e.g., ~77 million on 64-bit) such that sizeof(Mem)*nCol wraps to a small positive int when truncated. 2. Open the database and trigger STAT4 analysis (e.g., via ANALYZE or a query using the …

### `sqlite3.c [sqlite3VdbeMemFromBtree: payload fetch with bounds check]` — L13 — conf=0.72
**Integer truncation from i64 to int in allocation size leads to heap overflow**

In the SQLITE_TRANSIENT branch, `nAlloc` is computed as i64. The allocation call `sqlite3VdbeMemClearAndResize(pMem, (int)MAX(nAlloc,32))` truncates nAlloc to 32-bit int. If nAlloc > INT_MAX (e.g., 0x100000020), the cast produces a small value (0x20 = 32), allocating only 32 bytes. The subsequent `memcpy(pMem->z, z, nAlloc)` copies the full i64 nAlloc bytes (interpreted as size_t on 64-bit platforms), causing a massive heap buffer overflow. This requires nByte to exceed INT_MAX, which may be achievable via large SQL string literals or blob bindings.

*PoC sketch:* Execute SQL with an extremely large string or blob value (>2GB) where nByte exceeds INT_MAX. The (int) cast truncates the allocation size while memcpy uses the untruncated i64 value, overflowing the heap buffer.

### `sqlite3.c [sqlite3VdbeSerialGet A: type decode + int serialization]` — L2 — conf=0.7
**Missing Buffer Bounds Validation in Deserialization**

All deserialization functions (the unnamed 8-byte handler, serialGet7, and sqlite3VdbeSerialGet) read from `buf` based solely on the `serial_type` without any buffer length parameter or bounds checking. For example, serial_type 6/7 reads 8 bytes via FOUR_BYTE_UINT(buf) and FOUR_BYTE_UINT(buf+4), serial_type 1 reads 1 byte, serial_type 2 reads 2 bytes, etc. If a corrupted or maliciously crafted SQLite database file contains a record header declaring a serial_type that requires more bytes than the record body actually contains, these functions will perform out-of-bounds heap reads. This is a kno…

*PoC sketch:* Create a minimal SQLite database file. Corrupt a record's header in the B-tree page to declare serial_type=6 (8-byte integer) while the cell's payload size indicates only 2 bytes of body data. When sqlite3VdbeSerialGet is called, FOUR_BYTE_UINT(buf+4) will read 4 bytes beyond the allocated record bu…

### `sqlite3.c [windowAggStep A: window-frame advance (CVE-2019-5018)]` — L49 — conf=0.7
**Out-of-bounds column read in windowAggStep via unchecked iArgCol+nArg**

In windowAggStep, the inner loop generates OP_Column instructions accessing column index pWin->iArgCol+i for i in [0, nArg). There is no validation that pWin->iArgCol + nArg does not exceed the number of columns in the source cursor (csr or pMWin->iEphCsr). If iArgCol is computed incorrectly (e.g., due to inconsistent window function argument tracking) or if nArg from windowArgCount() is larger than expected, the generated VM code will read beyond the cursor's record boundary at runtime, causing a heap buffer overread.

*PoC sketch:* CREATE TABLE t(x); INSERT INTO t VALUES(1),(2),(3); SELECT x, lead(x, 1) OVER (ORDER BY x) FROM t; -- If iArgCol is misaligned with the actual ephemeral table schema, the OP_Column for the lead() function arguments reads past the record boundary.

### `sqlite3.c [jsonParseFuncArg: JSON input validation+cache lookup]` — L207050 — conf=0.7
**Use-after-free risk in cache lookup — nJPRef not incremented by caller**

The cache search function (partially shown at the end of the snippet) returns a JsonParse pointer that still belongs to the cache. The comment explicitly warns: 'The JsonParse object returned still belongs to the Cache and might be deleted at any moment. If the caller wants the JsonParse to linger, it needs to increment the nJPRef reference counter.' If any caller retrieves a cached JsonParse via lookup but fails to increment nJPRef before the cache evicts that entry (e.g., via a subsequent jsonCacheInsert that triggers LRU eviction calling jsonParseFree on the oldest entry), the caller holds …

*PoC sketch:* 1. Execute a SQL statement that calls a JSON function, causing a JsonParse to be cached. 2. In the same statement context, trigger enough additional JSON cache insertions (JSON_CACHE_SIZE+1 distinct JSON inputs) to cause LRU eviction of the first entry. 3. If the first function's result still refere…


## 🟡 MEDIUM (27)

### `sqlite3.c [sqlite3_str_vappendf A: format-loop+width+precision]` — L31734 — conf=0.9
**Denial of Service via unbounded width causing excessive memory allocation**

When SQLITE_PRINTF_PRECISION_LIMIT is not defined at compile time (the default), the width value is only masked to 0x7fffffff (2,147,483,647) but otherwise unbounded. A format string like "%2147483647d" causes the formatter to attempt padding with ~2GB of spaces, triggering a massive memory allocation via sqlite3_str_append. This applies to both the numeric width path (wx parsing) and the '*' variadic width path. The SQLITE_PRINTF_PRECISION_LIMIT guard exists but is opt-in and not enabled by default, leaving most builds vulnerable to DoS through user-controlled format strings (e.g., via SQL pr…

*PoC sketch:* SELECT printf('%2147483647d', 1); — attempts to allocate ~2GB for left-padding. Similarly, SELECT printf('%*d', 2147483647, 1); uses the '*' path to the same effect.

### `sqlite3.c [sqlite3_str_vappendf B: integer+radix flag setup]` — L124 — conf=0.88
**flag_zeropad bypasses SQLITE_PRINTF_PRECISION_LIMIT**

The SQLITE_PRINTF_PRECISION_LIMIT check (lines 29-33) is applied to the explicitly-specified precision, but the subsequent flag_zeropad adjustment 'precision = width-(prefix!=0)' can inflate precision far beyond that limit. This bypasses the intended safety bound and is the easiest path to trigger the nOut truncation vulnerability above. Even with SQLITE_PRINTF_PRECISION_LIMIT set to a safe value (e.g., 1000000), a format like '%02147483640d' sets width=2147483640, and since flag_zeropad is set, precision becomes 2147483640 — far exceeding the limit.

*PoC sketch:* sqlite3_str_appendf(s, "%02147483640d", 42); — width=2147483640 with zeropad flag. The precision limit check sees precision=-1 (default), which passes. Then flag_zeropad sets precision=2147483640, bypassing the limit entirely and triggering the nOut truncation.

### `sqlite3.c [sqlite3_str_vappendf A: format-loop+width+precision]` — L31730 — conf=0.85
**Integer overflow in width parsing via unsigned wx multiplication**

The `unsigned wx` variable overflows when parsing large numeric width values from the format string. The loop `wx = wx*10 + c - '0'` wraps modulo 2^32 for sufficiently large width strings. After overflow, `width = wx & 0x7fffffff` produces an unpredictable value that does not match the format string specification. The `testcase( wx>0x7fffffff )` comment confirms developer awareness without a proper fix. An attacker controlling the format string can craft a numeric width that, after overflow and masking, yields any desired width in [0, 0x7fffffff] — potentially bypassing upstream format-string …

*PoC sketch:* Format string "%4294967296d" — wx overflows: 4294967296 mod 2^32 = 0, so width = 0 & 0x7fffffff = 0. Format string "%9999999999d" — wx = 9999999999 mod 2^32 = 1410065407, width = 1410065407. An attacker can solve for a numeric string that yields any target width w in [0, 0x7fffffff] via w + k*2^32 f…

### `sqlite3.c [sqlite3RunParser: tokenize+parse loop entry]` — L180870 — conf=0.85
**TOCTOU Race Condition on Interrupt Flag Clearing**

The code checks `db->nVdbeActive==0` non-atomically, then performs `AtomicStore(&db->u1.isInterrupted, 0)`. Between the check and the store, another thread could increment `nVdbeActive` and call `sqlite3_interrupt()` to set `isInterrupted=1`. The subsequent `AtomicStore` would then silently clear that interrupt signal, causing a lost interrupt. This allows a long-running query to continue executing when it should have been interrupted — a denial-of-service condition in multi-threaded deployments using SQLite's serialized mode.

*PoC sketch:* Thread A: enters sqlite3RunParser, reads nVdbeActive==0 (true) Thread B: starts a Vdbe (nVdbeActive becomes 1), calls sqlite3_interrupt() → isInterrupted=1 Thread A: AtomicStore(&db->u1.isInterrupted, 0) — clears Thread B's interrupt Result: Thread B's query runs unimpeded despite the interrupt requ…

### `sqlite3.c [rtreeCheckNode B: bounding box + rtreeCheckTable]` — L216278 — conf=0.78
**SILENT error suppression in _rowid table check masks corruption**

When the prepared statement for `SELECT * FROM %Q.'%q_rowid'` fails (pStmt is NULL) and the error is not SQLITE_NOMEM, the error code is silently cleared: `if( check.rc!=SQLITE_NOMEM ){ check.rc = SQLITE_OK; }`. This means if the _rowid table is missing or has a corrupted schema (returning SQLITE_ERROR, SQLITE_CORRUPT, etc.), the integrity check silently continues with nAux=0 instead of reporting the corruption. This directly enables the nDim inflation vulnerability above, as a missing _rowid table is treated the same as a valid _rowid table with 2 columns. The integrity checker should report …

*PoC sketch:* 1. Create an rtree table with auxiliary columns. 2. Manually DROP the _rowid table or corrupt its schema in sqlite_master. 3. Run integrity_check - no error about the missing _rowid table is reported, and nAux defaults to 0, causing incorrect nDim calculation.

### `sqlite3.c [sqlite3VdbeSerialGet A: type decode + int serialization]` — L8 — conf=0.75
**Strict Aliasing Violation in Integer Deserialization (type 6)**

When serial_type==6, the code uses `pMem->u.i = *(i64*)&x;` to reinterpret a u64 as an i64 via pointer casting. This violates the C strict aliasing rule and is undefined behavior. Notably, the float path (serial_type==7) correctly uses `memcpy(&pMem->u.r, &x, sizeof(x))` to avoid this issue. An aggressive compiler optimizing under the strict aliasing assumption could miscompile the integer path, potentially leading to incorrect values being stored. This inconsistency between the integer and float paths suggests the integer path was overlooked during a past hardening pass.

*PoC sketch:* Compile sqlite3.c with aggressive optimization (-O2 -fstrict-aliasing) on GCC/Clang. Craft a database record with serial_type=6 and a negative 64-bit integer value (high bit set). The compiler may assume that the u64 variable `x` and the i64 lvalue `pMem->u.i` cannot alias, potentially reordering or…

### `sqlite3.c [sqlite3VdbeMemFromBtree: payload fetch with bounds check]` — L71 — conf=0.75
**Negative pMem->n from u32-to-int cast in sqlite3VdbeMemFromBtree**

In sqlite3VdbeMemFromBtree, `pMem->n = (int)amt` casts u32 amt to int. If amt > INT_MAX (0x7FFFFFFF), the cast produces a negative value. A negative pMem->n violates invariants expected by downstream VDBE operations that use pMem->n as a length or size parameter. This can cause buffer under-reads, incorrect memory operations, or crashes in code that assumes pMem->n >= 0. Combined with the offset+amt overflow, a crafted database can trigger this path.

*PoC sketch:* Craft a database where a record field triggers amt=0x80000000 in sqlite3VdbeMemFromBtree. The bounds check may pass if offset+amt overflows to a small value. pMem->n becomes (int)0x80000000 = -2147483648, causing undefined behavior in downstream operations that use pMem->n as a memcpy length, compar…

### `sqlite3.c [jsonParseFuncArg: JSON input validation+cache lookup]` — L207020 — conf=0.75
**Security-critical preconditions validated only by assert() — compiled out in release builds**

jsonCacheInsert validates critical preconditions using assert() only: assert(pParse->zJson!=0), assert(pParse->bJsonIsRCStr), assert(pParse->delta==0), and assert(pParse->nBlobAlloc>0). In release builds (NDEBUG defined), these assertions are removed entirely. If a caller violates any precondition — e.g., passing a JsonParse with delta!=0 (pending size edits) — the corrupted object is cached. A non-zero delta causes incorrect size calculations on cache retrieval, potentially leading to buffer overflows when the cached parse result is used to compute buffer sizes or offsets. Similarly, bJsonIsR…

*PoC sketch:* In a debug build, craft a scenario where jsonParseFuncArg is called with a JsonParse that has delta!=0 (e.g., after a json_set operation that resized the JSON). The assert fires. In a release build, the same scenario silently caches the corrupted object, and subsequent cache hits return a JsonParse …

### `sqlite3.c [jsonParseFunc: JSON tree building+node allocation]` — L207130 — conf=0.7
**Truncation of u64 nTotal to u32 nAlloc in jsonStringGrow undermines size tracking**

At the end of jsonStringGrow(), the assignment `p->nAlloc = nTotal` truncates the u64 nTotal value to u32 nAlloc. If nTotal exceeds UINT32_MAX (possible when the second branch p->nAlloc+N+10 is computed correctly in u64 due to implicit promotion), nAlloc wraps to a small value. Subsequent bounds checks (e.g., N+p->nUsed >= p->nAlloc) would then incorrectly believe the buffer is small, triggering unnecessary growth attempts but also potentially allowing writes when the check incorrectly passes. This desynchronization between actual allocation size and tracked size creates inconsistent state.

*PoC sketch:* Trigger jsonStringGrow with large N such that nTotal = p->nAlloc + N + 10 > 0xFFFFFFFF. The u64 computation is correct, sqlite3RCStrNew allocates the full size, but p->nAlloc truncates. Subsequent append operations see a small nAlloc and attempt to re-grow or make incorrect bounds decisions.

### `sqlite3.c [sqlite3VdbeMemFromBtree: payload fetch with bounds check]` — L29 — conf=0.68
**Length masking inconsistency between stored length and actual data size**

The assignment `pMem->n = (int)(nByte & 0x7fffffff)` masks the stored length to 31 bits. When nByte >= 0x80000000, pMem->n stores a truncated value (e.g., nByte=0x80000001 yields pMem->n=1) while the actual buffer contains nAlloc bytes of data. Downstream code that relies on pMem->n to determine data length will read or expose incorrect amounts of data, potentially leading to information disclosure (reading beyond intended bounds) or logic errors in string/blob operations.

*PoC sketch:* Provide a string or blob with nByte=0x80000001. The memcpy copies 0x80000001+ bytes into the buffer, but pMem->n is set to 1. Any subsequent operation using pMem->n as the length (e.g., string comparison, display) will only see 1 byte, while the full data remains in the heap. If the Mem is later reu…

### `sqlite3.c [sqlite3VdbeSerialGet B: string/blob+float types]` — L111 — conf=0.65
**Missing bounds validation of szHdr against nKey in sqlite3VdbeRecordUnpack**

In `sqlite3VdbeRecordUnpack`, the header size `szHdr` is read from the record via `getVarint32(aKey, szHdr)` and then used directly as an offset (`d = szHdr`) without validating that `szHdr <= nKey`. A crafted database record where the header-size varint claims a header larger than the actual record payload would cause `d` to exceed the buffer bounds. Subsequent reads from `aKey[d]` or `aKey[idx]` (as the function iterates through header entries) would access memory beyond the `nKey`-byte record buffer, resulting in an out-of-bounds read. This could leak sensitive heap data or cause a crash.

*PoC sketch:* Craft a SQLite database file where a record's header-size varint encodes a value larger than the actual record size (nKey). For example, in a minimal record, set the first varint to 255 (claiming a 255-byte header) while the actual record is only 10 bytes. When sqlite3VdbeRecordUnpack processes this…

### `sqlite3.c [whereLoopAddBtreeIndex D: skip-scan+tail conditions]` — L166920 — conf=0.65
**Negative nIter from corrupted statistics causes integer underflow in skip-scan cost estimation**

In the skip-scan block, nIter is computed as `pProbe->aiRowLogEst[saved_nEq] - pProbe->aiRowLogEst[saved_nEq+1]`. The aiRowLogEst values are derived from the sqlite_stat1 table, which is stored in the database file and can be crafted by an attacker. If the statistics are manipulated such that aiRowLogEst[saved_nEq] < aiRowLogEst[saved_nEq+1] (violating the expected monotonicity), nIter becomes negative. This negative nIter is then used in `pNew->nOut -= nIter` (inflating nOut), and passed as `nIter + nInMul` to the recursive `whereLoopAddBtreeIndex` call where it is added to `pNew->rRun` and `…

*PoC sketch:* 1. Create a SQLite database with an index on a table. 2. Manually insert corrupted statistics into sqlite_stat1: INSERT INTO sqlite_stat1 VALUES('tbl','idx','4 1000000'); -- where the second value (distinct count for first column prefix) far exceeds the first (total rows), violating monotonicity. 3.…

### `sqlite3.c [sqlite3AlterFinishAddColumn C: default+affinity+codegen]` — L10 — conf=0.65
**Incomplete quick_check validation condition allows NOT NULL constraint bypass**

The condition governing whether quick_check validation runs after ALTER TABLE ADD COLUMN only triggers for: (1) tables with CHECK constraints, (2) NOT NULL generated columns, or (3) STRICT tables. Regular NOT NULL non-generated columns on non-STRICT tables without CHECK constraints skip validation entirely. If the DEFAULT expression evaluates to NULL at runtime (e.g., DEFAULT (NULLIF(1,1)) or DEFAULT (CASE WHEN 0=1 THEN 'x' END)), the NOT NULL constraint is violated for existing rows but quick_check is never invoked to detect it. This creates a constraint bypass where a NOT NULL column silentl…

*PoC sketch:* CREATE TABLE t(a TEXT); INSERT INTO t VALUES('hello'); ALTER TABLE t ADD COLUMN c TEXT NOT NULL DEFAULT (CASE WHEN 0=1 THEN 'valid' END); -- The DEFAULT evaluates to NULL, but quick_check is skipped because: --   pNew->pCheck == 0 (no CHECK constraints) --   c is NOT generated --   t is not STRICT -…

### `sqlite3.c [sessionApplyChange: session record applier]` — L90 — conf=0.6
**Integer overflow in nCol*2 loop bound causing skipped cleanup and memory corruption**

At line 90, the loop bound `nCol*2` is computed as a signed int multiplication. If nCol >= 0x40000000 (1073741824), nCol*2 overflows to a negative value (e.g., 0x80000000 = -2147483648). The comparison `iCol < nCol*2` with iCol=0 and a negative bound evaluates to false, so the cleanup loop is entirely skipped. This means sqlite3ValueFree is never called for the apVal entries, and the memset at line 93 (which has the same overflow) also becomes a no-op. The stale apVal entries from a previous iteration may then be reused in the next UPDATE record, leading to use-after-free when sqlite3ValueFree…

*PoC sketch:* Craft a changeset with a 'T' record setting nCol=0x40000001, followed by two UPDATE records. The first UPDATE allocates apVal and populates it. The cleanup loop (nCol*2 = -2147483646) is skipped. The second UPDATE reuses the same apVal (since 0==apVal is false), and sessionReadRecord overwrites apVa…

### `sqlite3.c [whereLoopAddBtreeIndex A: index-scan+cost (CVE-2019-16168)]` — L55 — conf=0.6
**Assert-only bounds check on saved_nEq allows OOB read in indexColumnNotNull**

The bounds checks for saved_nEq (pNew->u.btree.nEq < pProbe->nColumn and pNew->u.btree.nEq < pProbe->nKeyCol) are implemented only as assert() statements. In release builds (NDEBUG defined), these asserts are compiled out entirely. If a corrupted or crafted database causes nEq to exceed nColumn, the subsequent call to indexColumnNotNull(pProbe, saved_nEq) on line ~68 will read out-of-bounds memory from the index column metadata arrays. While this requires a crafted database and is mitigated by the query planner's normal invariants, the reliance on asserts for memory safety is a defensive progr…

*PoC sketch:* 1. Craft a database file where the Index structure's nColumn field is set to a small value (e.g., 1) but the WhereLoop's nEq is set to a larger value through memory corruption or fuzzing. 2. In a release build (asserts disabled), trigger whereLoopAddBtreeIndex with the corrupted state. 3. indexColum…

### `sqlite3.c [whereLoopAddBtreeIndex B: nRow/nOut arithmetic]` — L69 — conf=0.6
**Out-of-bounds read via pTop = &pTerm[1] in LIKE optimization pair handling**

When a WHERE term has the TERM_LIKEOPT flag (from LIKE optimization), the code unconditionally accesses the next adjacent term via `pTop = &pTerm[1]`. Bounds checking is performed only by `assert()` statements (lines 72-74), which are compiled out in release builds. If a TERM_LIKEOPT term is ever the last element in the WhereClause array (pWC->a), this results in an out-of-bounds read. The OOB-read pointer is then stored into pNew->aLTerm[pNew->nLTerm++] and later dereferenced during query execution, potentially leading to further memory corruption or information disclosure.

*PoC sketch:* Craft a SQL query that triggers the LIKE optimization (e.g., SELECT * FROM t WHERE x LIKE 'abc%') in a context where the TERM_LIKEOPT lower-bound term ends up as the last entry in the WhereClause term array. This could occur if term reordering or prior term elimination causes the paired upper-bound …

### `sqlite3.c [jsonParseFunc: JSON tree building+node allocation]` — L207085 — conf=0.6
**Use-after-free in jsonCacheSearch via dangling zJson pointer in memcmp**

In jsonCacheSearch(), the second lookup loop performs memcmp(p->a[i]->zJson, zJson, nJson) on cached JsonParse entries. The zJson pointer in a cached JsonParse may reference the text buffer of a previously used sqlite3_value that has since been freed. If the cache does not properly invalidate entries when their source sqlite3_value is destroyed, this memcmp reads from freed memory — a use-after-free. The first loop's pointer comparison (p->a[i]->zJson==zJson) is safe since zJson is freshly obtained, but the fallback memcmp dereferences the potentially-stale cached pointer.

*PoC sketch:* Execute a JSON function call that caches a JsonParse with zJson pointing to a sqlite3_value's text buffer. Then execute another statement that causes the original sqlite3_value to be freed (e.g., by resetting the statement). A subsequent JSON function call with different text triggers the second loo…

### `sqlite3.c [sqlite3RunParser: tokenize+parse loop entry]` — L180887 — conf=0.55
**Missing Zero-Length Token Guard Leading to Infinite Loop**

When `tokenType==TK_SPACE` (line ~180903) or `tokenType==TK_COMMENT` (line ~180920), the code does `zSql += n; continue;` without checking whether `n > 0`. If `sqlite3GetToken` ever returns `n==0` for a space or comment token, `zSql` never advances and `mxSqlLen` is never decremented (since `mxSqlLen -= 0`), producing an infinite loop. The `mxSqlLen < 0` guard on line ~180889 cannot catch this because `n==0` never drives `mxSqlLen` negative. While the current `sqlite3GetToken` implementation should not return `n==0` for whitespace, the parser loop lacks a defensive check, making it fragile aga…

*PoC sketch:* If sqlite3GetToken could be induced to return n=0 with tokenType=TK_SPACE (e.g., via a crafted multi-byte sequence that the tokenizer misparses as zero-length whitespace), the loop at lines 180887-180910 would spin indefinitely:   while(1) { n=0; mxSqlLen-=0; /* not <0 */; tokenType==TK_SPACE → zSql…

### `sqlite3.c [sqlite3VdbeSerialGet B: string/blob+float types]` — L82 — conf=0.55
**Integer overflow in UnpackedRecord allocation size computation**

The computation `sizeof(Mem)*(pKeyInfo->nKeyField+1)` in `sqlite3VdbeAllocUnpackedRecord` can overflow a 32-bit `int` (`nByte`) if `nKeyField` is sufficiently large. On 64-bit systems where `sizeof(Mem)` is ~48-64 bytes, an `nKeyField` value around 45-89 million would cause the multiplication to wrap around, resulting in a small heap allocation. Subsequent population of the `p->aMem` array (up to `nKeyField+1` entries) would then write far beyond the allocated buffer, causing a heap buffer overflow. While `nKeyField` is typically bounded by `SQLITE_MAX_COLUMN` (default 2000), builds with very …

*PoC sketch:* Compile SQLite with SQLITE_MAX_COLUMN set to an extremely large value (e.g., 100,000,000). Create an index on a table with enough columns to make nKeyField approach the overflow threshold. Alternatively, if KeyInfo structures can be corrupted in memory (e.g., via a use-after-free or heap corruption …

### `sqlite3.c [sqlite3VdbeMemFromBtreeZeroOffset: direct page access]` — L84378 — conf=0.55
**Missing error handling for sqlite3VdbeChangeEncoding() in valueToText()**

In valueToText(), after calling sqlite3VdbeChangeEncoding(pVal, enc), the return value is not checked. If the encoding conversion fails (e.g., due to OOM), the Mem object pVal may be left in an inconsistent state — the old buffer could have been freed but no new buffer allocated, leaving pVal->z as a dangling pointer. The subsequent call to sqlite3VdbeMemNulTerminate(pVal) would then attempt to write a NUL terminator through this dangling pointer, resulting in a use-after-free / heap buffer write. While SQLite's convention is to set values to a null-like state on allocation failure, the lack o…

*PoC sketch:* 1. Construct a scenario where sqlite3VdbeChangeEncoding() is called on a value with a large string that requires reallocation during encoding conversion. 2. Force an OOM condition at the point of reallocation inside sqlite3VdbeChangeEncoding(). 3. If the function does not properly nullify pVal->z on…

### `sqlite3.c [windowAggStep A: window-frame advance (CVE-2019-5018)]` — L49 — conf=0.55
**Register array overwrite without bounds verification in windowAggStep**

windowAggStep writes argument values into registers reg+i for i in [0, nArg). The function relies on the caller guaranteeing that the register array starting at reg is large enough for each window function's arguments. However, no bounds check is performed within this function. If nArg (from windowArgCount) is inconsistent with the allocated register array size due to a logic error in the caller, the OP_Column instructions will write past the end of the register file, corrupting adjacent VDBE state. The risk is elevated because the loop iterates over a linked list of window functions (pWin->pN…

*PoC sketch:* CREATE TABLE t(a,b,c,d,e,f,g); INSERT INTO t VALUES(1,2,3,4,5,6,7); SELECT a, max(b+c+d+e+f+g) OVER (), min(a) OVER () FROM t; -- If the register allocation for the first window function's arguments is smaller than windowArgCount returns, the second iteration's writes overflow.

### `sqlite3.c [sessionApplyChange: session record applier]` — L21 — conf=0.55
**Potential out-of-bounds read in sPK buffer copy from untrusted nCol**

At line 21, nCol is read from the input via sessionVarintGet. At line 23, `sessionAppendBlob(&sPK, &pInput->aData[pInput->iNext+nVar], nCol, &rc)` copies nCol bytes from the input buffer into sPK. If sessionChangesetBufferTblhdr does not validate that the sPK array actually contains nCol bytes (i.e., if nByte < nVar + nCol), this reads beyond the validated portion of the input buffer. A crafted changeset with a large nCol varint but a short sPK array would trigger an out-of-bounds read, potentially leaking heap data into the sPK buffer and subsequently into the inverted changeset output.

*PoC sketch:* Craft a 'T' record where the varint encodes nCol=100 but the actual sPK array in the input is only 10 bytes, with the table name following immediately after. If sessionChangesetBufferTblhdr computes nByte based on the actual data layout rather than nCol, the copy at line 23 reads 90 bytes beyond the…

### `sqlite3.c [whereLoopAddBtreeIndex A: index-scan+cost (CVE-2019-16168)]` — L82 — conf=0.55
**Unchecked aiRowLogEst array access on potentially malformed Index**

The access pProbe->aiRowLogEst[0] assumes the aiRowLogEst array is properly allocated and initialized. For certain internal index types or in corruption scenarios, aiRowLogEst could be NULL or undersized. There is no NULL check or bounds validation before dereferencing. A crafted database file with a malformed index header could cause this to read from invalid memory, leading to a crash or information disclosure.

*PoC sketch:* 1. Use a SQLite database fuzzer or hex editor to corrupt the index header of a database file such that the Index object's aiRowLogEst pointer is NULL or points to undersized memory. 2. Open the corrupted database and execute a query that triggers index scan planning for the corrupted index. 3. The d…

### `sqlite3.c [whereLoopAddBtreeIndex C: multi-column+IN operator]` — L24 — conf=0.55
**Out-of-bounds read on aiRowLogEst array in fallback estimation path**

In the fallback estimation path (when STAT4 is unavailable or nOut==0), the code accesses pProbe->aiRowLogEst[nEq] and pProbe->aiRowLogEst[nEq-1] without bounds checking. The STAT4 path has an explicit guard (ALWAYS(pNew->u.btree.nEq<=pProbe->nSampleCol)), but the fallback path at the end of the else block has no equivalent check. If nEq (incremented via ++pNew->u.btree.nEq) exceeds the number of entries in aiRowLogEst (which has nColumn+1 entries), this results in an out-of-bounds read. This is particularly relevant for multi-column indexes with IN operators, where the interaction between equ…

*PoC sketch:* Craft a query against a multi-column index with a combination of IN operators and equality constraints that causes the query planner to process more equality terms than there are index columns. For example: CREATE TABLE t(a,b,c); CREATE INDEX idx ON t(a,b); SELECT * FROM t WHERE a IN (1,2,3) AND b I…

### `sqlite3.c [sqlite3AlterFinishAddColumn C: default+affinity+codegen]` — L96 — conf=0.55
**Missing NULL check after pNew->aCol allocation**

After allocating pNew->aCol with sqlite3DbMallocZero on line 96, there is no NULL check analogous to the one for pNew on line 89. If the allocation fails (OOM), sqlite3DbMallocZero returns NULL and sets db->mallocFailed. However, any subsequent dereference of pNew->aCol before the malloc-failed flag is checked would cause a NULL pointer dereference crash. The inconsistent checking pattern (pNew is checked, pNew->aCol is not) suggests an oversight. While SQLite's deferred error-checking model often catches malloc failures later, code paths between this allocation and the next flag check that ac…

*PoC sketch:* 1. Create a table with many columns to increase memory pressure. 2. Use a mechanism to limit SQLite's heap (e.g., sqlite3_soft_heap_limit64 with a very small value). 3. Execute ALTER TABLE t ADD COLUMN c TEXT; 4. If the aCol allocation fails and subsequent code dereferences pNew->aCol before checkin…

### `sqlite3.c [sqlite3VdbeMemFromBtreeZeroOffset: direct page access]` — L84488 — conf=0.5
**Unchecked NULL from sqlite3KeyInfoOfIndex() may lead to use of uninitialized aMem pointer**

In valueNew(), after allocating pRec, sqlite3KeyInfoOfIndex() is called and its result is checked with `if( pRec->pKeyInfo )`. However, if this returns NULL, the code inside the if-block (which sets pRec->aMem and initializes the Mem array) is skipped. The code after the if-block (not shown, cut off) may still use pRec assuming aMem was properly initialized. If pRec is returned to the caller with an uninitialized aMem pointer, subsequent access to the UnpackedRecord's Mem values would dereference garbage, leading to a crash or potential code execution. Additionally, if pRec is not freed on the…

*PoC sketch:* 1. Create a database with an index that causes sqlite3KeyInfoOfIndex() to return NULL (e.g., corrupted index schema, or an index referencing a deleted collation sequence). 2. Trigger STAT4 probing on this index. 3. valueNew() allocates pRec but skips aMem initialization due to NULL pKeyInfo. 4. Call…

### `sqlite3.c [whereLoopAddBtreeIndex C: multi-column+IN operator]` — L8 — conf=0.4
**Potential out-of-bounds read on aLTerm array when WHERE_BTM_LIMIT is set**

When processing a top-limit constraint (WO_LT/WO_LE), the code accesses pNew->aLTerm[pNew->nLTerm-2] if the WHERE_BTM_LIMIT flag is set. If nLTerm is less than 2 when this flag is set (due to a logic inconsistency where the flag was set but the corresponding term was not properly added to the aLTerm array), the index nLTerm-2 becomes negative, resulting in an out-of-bounds read. This could leak heap memory contents or cause a crash.

*PoC sketch:* Craft a query with range constraints that causes the query planner to set WHERE_BTM_LIMIT without properly populating aLTerm. For example: SELECT * FROM t WHERE a > 5 AND a < 10; on a table with a multi-column index where the bottom-limit term processing has an edge case that sets the flag but skips…


## 🔵 LOW (21)

### `sqlite3.c [windowAggStep A: window-frame advance (CVE-2019-5018)]` — L44 — conf=0.9
**Security-critical assertions disabled in release builds**

Two assert() statements guard important invariants: (1) bInverse==0 || pWin->eStart!=TK_UNBOUNDED, and (2) window comparison consistency. In release builds (NDEBUG defined), these assertions are compiled out, providing no runtime protection. If either invariant is violated due to a logic error elsewhere, the function proceeds with invalid state, potentially leading to undefined behavior. The first assertion prevents calling xInverse on an unbounded-start window, which could corrupt aggregate state.

*PoC sketch:* N/A - This is a defense-in-depth issue. Triggering requires a separate bug that violates the asserted invariant.

### `sqlite3.c [sqlite3_str_vappendf B: integer+radix flag setup]` — L21 — conf=0.7
**Unsigned integer overflow in precision parsing yields unexpected precision values**

The variable 'px' (unsigned, typically 32-bit) accumulates precision digits via 'px = px*10 + c - "0"'. For format strings with extremely long digit sequences (e.g., >10 digits), px wraps around due to unsigned overflow. The subsequent 'precision = px & 0x7fffffff' then produces an essentially arbitrary precision value. While this cannot directly produce negative values, it allows an attacker to select from a wide range of precision values by carefully choosing the digit string, potentially reaching values that trigger the nOut truncation without needing an explicit huge number.

*PoC sketch:* A format string like "%.4294967296d" causes px to overflow to 0, yielding precision=0. But "%.4294967306d" (4294967296+10) overflows to 10, yielding precision=10. By choosing appropriate overflow values, an attacker can achieve precision values up to 0x7fffffff that may not match the apparent format…

### `sqlite3.c [sqlite3VdbeSerialGet A: type decode + int serialization]` — L24 — conf=0.65
**NaN to NULL Conversion May Violate Type Invariants**

When deserializing a floating-point value (serial_type 7), if the IEEE 754 bit pattern represents NaN, the value is silently converted to NULL (MEM_Null) instead of MEM_Real. This means a crafted database can store a NaN value that, when read back, appears as NULL. This type confusion between REAL and NULL can break application logic that expects a non-NULL REAL value from a REAL-typed column, potentially leading to authentication bypass (e.g., NULL comparisons in SQL) or incorrect financial calculations.

*PoC sketch:* Insert a row with a REAL column value of NaN (e.g., via a custom-built database file with serial_type=7 and bytes 0x7FF8000000000000). When read back, sqlite3VdbeSerialGet will set flags to MEM_Null. A query like `SELECT * FROM t WHERE real_col IS NOT NULL` will exclude this row, while `SELECT typeo…

### `sqlite3.c [rtreeCheckNode B: bounding box + rtreeCheckTable]` — L216237 — conf=0.65
**Unescaped %s format specifier in rtreeCheckCount SQL query construction**

In rtreeCheckCount(), the SQL query is constructed using `"SELECT count(*) FROM %Q.'%q%s'"` where zTbl is appended via `%s` without escaping. While currently safe because zTbl is always the hardcoded string "_rowid" or "_parent", the use of `%s` instead of `%q` means that if a future caller passes a user-controlled or non-hardcoded zTbl value, SQL injection becomes possible. This is a defense-in-depth weakness rather than an immediately exploitable vulnerability.

*PoC sketch:* Not directly exploitable with current callers. To demonstrate the risk, if rtreeCheckCount were called with zTbl="'; DROP TABLE users; --", the resulting SQL would be: SELECT count(*) FROM "db".'tab'; DROP TABLE users; --' which would execute the injected SQL.

### `sqlite3.c [sqlite3RunParser: tokenize+parse loop entry]` — L180913 — conf=0.6
**Assert-Only Token Length Validation Before Pointer Arithmetic**

The code uses `assert(n==6)` and `assert(n==4)` to validate token lengths before computing offsets like `&zSql[6]` and `&zSql[4]` for `analyzeWindowKeyword`, `analyzeOverKeyword`, and `analyzeFilterKeyword`. In release builds (NDEBUG), these assertions are compiled out. If `sqlite3GetToken` ever returns an incorrect `n` for these token types, the hardcoded offsets would read from wrong positions in the SQL string — potentially past the token boundary or, in extreme cases, past the end of the allocated buffer. The reliance on assertions for safety-critical bounds validation is a defense-in-dept…

*PoC sketch:* In a release build, if sqlite3GetToken returned TK_WINDOW with n=3 (due to a tokenizer bug), the call analyzeWindowKeyword(&zSql[6]) would read 3 bytes past the actual token into adjacent SQL text, potentially misclassifying the keyword or reading uninitialized memory.

### `sqlite3.c [sqlite3VdbeSerialGet A: type decode + int serialization]` — L55 — conf=0.6
**Fall-through from Reserved Serial Type 11 to NULL Handling**

Serial type 11 is marked 'Reserved for future use' but falls through to case 0 (NULL) without any warning or distinct handling. If a future SQLite version assigns meaning to serial_type 11, old code silently treating it as NULL could lead to type confusion or data loss. Additionally, a crafted database could use serial_type 11 to inject NULL values where the application logic doesn't expect them, potentially bypassing NOT NULL constraints if validation relies on serial_type checks upstream.

*PoC sketch:* In a crafted database file, set a column's serial_type to 11 in a record header. The value will be deserialized as NULL regardless of any data present in the buffer, potentially bypassing application-level NOT NULL validation that doesn't account for this reserved type.

### `sqlite3.c [rtreeCheckNode B: bounding box + rtreeCheckTable]` — L216293 — conf=0.6
**SQLITE_CORRUPT error suppressed during finalize in rtreeCheckTable**

After finalizing the statement used to determine nDim and bInt, the code explicitly suppresses SQLITE_CORRUPT errors: `if( rc!=SQLITE_CORRUPT ) check.rc = rc;`. This means if the database is corrupt and sqlite3_finalize returns SQLITE_CORRUPT, the error is discarded and the integrity check continues. While this may be intentional to allow the check to proceed and find more errors, it means the corruption signal from the main table query is lost, and subsequent operations (rtreeCheckNode) may operate on invalid data with a clean check.rc, potentially leading to undefined behavior in node traver…

*PoC sketch:* Provide a corrupt rtree database where the main table query triggers SQLITE_CORRUPT on finalize. The integrity check will continue with check.rc==SQLITE_OK, potentially accessing invalid node data in rtreeCheckNode.

### `sqlite3.c [jsonParseFuncArg: JSON input validation+cache lookup]` — L206975 — conf=0.6
**Stack overflow via deeply nested JSON — depth limit may be insufficient on constrained stacks**

JSON_MAX_DEPTH is set to 1000 to prevent stack overflow in the recursive descent parser. However, on systems with small thread stacks (e.g., 64KB–128KB on embedded systems or custom thread pools), 1000 recursive frames — each consuming stack space for local variables, return addresses, and saved registers — can exceed the stack limit. The comment acknowledges this is a concern ('This limit is needed to avoid a stack overflow') but the chosen value of 1000 may still be too high for some deployment environments. An attacker supplying a JSON document with 1000 levels of nesting could crash the pr…

*PoC sketch:* Generate a JSON document with 1000 levels of array nesting: '[[[[[...[1]...]]]]]' with 1000 opening brackets. On a thread with a 64KB stack, this may overflow. On typical desktop stacks (1–8MB), 1000 levels are safe, making this platform-dependent.

### `sqlite3.c [sqlite3_str_vappendf A: format-loop+width+precision]` — L31724 — conf=0.55
**Potential out-of-bounds read via 'l' flag at end of format string**

When the format string ends with '%l' or '%ll', the 'l' case advances fmt with `c = *++fmt` (and potentially again for 'll'), reading the null terminator into c. It then sets done=1 and exits the flag-parsing loop with c=0. The subsequent format-specifier dispatch code (not shown in this snippet) must handle c=0 gracefully; if it uses c as an index into a lookup table (e.g., et_info array) without a bounds check, it could perform an out-of-bounds read. While the outer for-loop checks `c=(*fmt)!=0`, the 'l' case has already consumed past that check point by advancing fmt internally.

*PoC sketch:* Format string "%l" or "%ll" — after processing, c=0 (null terminator) and done=1. If downstream code uses c to index an array without validating c!=0, an OOB read occurs.

### `sqlite3.c [whereLoopAddBtreeIndex D: skip-scan+tail conditions]` — L166926 — conf=0.55
**Missing nLTerm restoration after skip-scan recursive call leaves stale aLTerm entry**

In the skip-scan block, `pNew->aLTerm[pNew->nLTerm++] = 0` increments nLTerm and writes a NULL entry. After the recursive `whereLoopAddBtreeIndex` call, the code restores nOut, nEq, nSkip, and wsFlags to their saved values, but does NOT restore nLTerm or clear the stale NULL entry in aLTerm. While the function returns immediately after the skip-scan block (and the caller restores its own saved state), any error path or future code change that reads aLTerm between this function returning and the caller's restoration could observe the stale NULL, potentially causing a NULL pointer dereference in…

*PoC sketch:* This is a latent state management bug. To observe: 1. Enable SQLITE_DEBUG and add an assertion after the skip-scan block that pNew->nLTerm == saved_nLTerm. 2. Trigger skip-scan on a multi-column index. 3. The assertion will fire because nLTerm was incremented but not restored.

### `sqlite3.c [whereLoopAddBtreeIndex B: nRow/nOut arithmetic]` — L20 — conf=0.5
**Stale nIn value when ALWAYS() invariant is violated in IN-operator handling**

The `ALWAYS(pExpr->x.pList && pExpr->x.pList->nExpr)` guard on the IN-list branch assumes the expression always has a non-empty pList. In release builds, ALWAYS() is a no-op hint. If this invariant is violated (pList is NULL or nExpr is 0), the branch body is skipped and `nIn` retains its previous value (potentially 0 from the duplicate-check loop on line 17, or an uninitialized/stale value). This stale nIn feeds into the LogEst arithmetic `x = M + logK + 10 - (nIn + rLogSize)` on line 38, producing an incorrect query plan decision. While primarily a logic/correctness bug, an attacker who can …

*PoC sketch:* Execute a query with an IN clause where the expression's pList is unexpectedly NULL or empty (e.g., via a malformed virtual table or corrupted schema). In a release build, the ALWAYS check is elided, nIn is not set, and the optimizer makes decisions based on garbage LogEst values.

### `sqlite3.c [jsonParseFunc: JSON tree building+node allocation]` — L207160 — conf=0.45
**Signed integer N in jsonPrintf enables negative size passed to vsnprintf**

jsonPrintf accepts `int N` (signed) and passes it to sqlite3_vsnprintf(N, ...). If a caller passes a negative N, the growth check `p->nUsed + N >= p->nAlloc` could evaluate incorrectly (N negative makes the sum smaller, bypassing growth). Then sqlite3_vsnprintf receives a negative size, which when implicitly converted to size_t becomes a very large value, potentially causing out-of-bounds writes. While current internal callers likely pass positive values, the signed parameter type is a latent risk.

*PoC sketch:* If any code path passes a negative N to jsonPrintf (e.g., due to an arithmetic error in the caller), the growth check is bypassed and sqlite3_vsnprintf writes with an effectively unlimited size into the buffer.

### `sqlite3.c [sqlite3RunParser: tokenize+parse loop entry]` — L180880 — conf=0.4
**Unprotected Parse Context Swap Without Error-Path Rollback**

The code saves `pParentParse = db->pParse` and sets `db->pParse = pParse` without an immediate paired restore visible in this snippet. If any early-return error path (e.g., the `SQLITE_TOOBIG` break, `SQLITE_INTERRUPT` break, or `SQLITE_NOMEM_BKPT` return) exits the function without restoring `db->pParse = pParentParse`, the database connection retains a dangling pointer to the local/stack `pParse` after it goes out of scope. Subsequent operations on `db->pParse` would constitute a use-after-free. The restore likely exists in the truncated portion, but the visible error exits lack it, indicati…

*PoC sketch:* 1. Set SQLITE_LIMIT_SQL_LENGTH to 1 2. Execute a multi-byte SQL statement → triggers SQLITE_TOOBIG break 3. If db->pParse is not restored before function return, db->pParse dangles 4. Next operation using db->pParse reads freed/invalid memory

### `sqlite3.c [whereLoopAddBtreeIndex D: skip-scan+tail conditions]` — L166878 — conf=0.4
**Unbounded nInMul accumulation through recursive tail-condition calls may overflow**

The recursive call `whereLoopAddBtreeIndex(pBuilder, pSrc, pProbe, nInMul+nIn)` accumulates nInMul across recursive invocations. Inside each recursion, `pNew->rRun += nInMul + nIn` and `pNew->nOut += nInMul + nIn` are executed. While LogEst values are typically small (logarithmic scale), a deeply nested index with many IN-clause terms could cause nInMul to grow large enough that `nInMul + nIn` overflows a 32-bit int. The recursion depth is bounded by pProbe->nColumn, but SQLite allows indexes with up to 30 columns, and each level can contribute significant nIn values from IN-clauses. An intege…

*PoC sketch:* 1. Create a table with an index on 30 columns. 2. Construct a query with IN-clauses on many of the indexed columns, each with many values. 3. The accumulated nInMul across 30 levels of recursion could exceed INT_MAX, causing signed integer overflow in the cost calculation additions.

### `sqlite3.c [sqlite3AlterFinishAddColumn A: schema rebuild (CVE-2020-35527)]` — L117175 — conf=0.4
**Potential SQL injection via insufficient identifier escaping in sqlite3ErrorIfNotEmpty**

The sqlite3ErrorIfNotEmpty function constructs a SQL statement using sqlite3NestedParse with the %w format specifier for zDb and zTab. While %w performs double-quote escaping for identifiers, edge cases with extremely long identifier names or names containing null bytes could potentially bypass the escaping. However, since these values originate from the parser (already-validated schema names), the practical exploitability is very limited. This is more of a defense-in-depth concern than an exploitable vulnerability.

*PoC sketch:* -- Theoretical: if a table name could contain a double-quote that -- bypasses %w escaping (not normally possible through the parser): -- CREATE TABLE '"."." (a int);  -- parser rejects this -- ALTER TABLE '"."." ADD COLUMN b int; -- The %w specifier should handle this, but null byte truncation -- in…

### `sqlite3.c [sqlite3AlterFinishAddColumn C: default+affinity+codegen]` — L94 — conf=0.4
**Integer overflow in nAlloc calculation for column array sizing**

The calculation nAlloc = (((pNew->nCol-1)/8)*8)+8 can produce signed integer overflow when pNew->nCol is near INT_MAX. For example, if nCol = 2147483647 (INT_MAX), then (2147483647-1)/8 = 268435455, 268435455*8 = 2147483640, and 2147483640+8 = 2147483648 which overflows to -2147483648 (signed 32-bit). This negative nAlloc would then be passed to sqlite3DbMallocZero as sizeof(Column)*nAlloc, where the implicit conversion to size_t produces a huge value causing allocation failure, or on some platforms a small allocation leading to heap buffer overflow when columns are copied. The assert on line …

*PoC sketch:* This requires bypassing SQLITE_MAX_COLUMN or corrupting pTab->nCol. If an attacker can influence nCol to be near INT_MAX (e.g., via memory corruption of the Table structure), the overflow triggers. In standard configurations with default SQLITE_MAX_COLUMN=2000, nAlloc = (((2000-1)/8)*8)+8 = 2008, wh…

### `sqlite3.c [jsonParseFuncArg: JSON input validation+cache lookup]` — L207035 — conf=0.4
**Potential double-count of nJPRef if same JsonParse inserted into cache twice**

jsonCacheInsert unconditionally increments pParse->nJPRef without checking whether the same JsonParse object is already present in the cache. If a caller mistakenly inserts the same object twice, nJPRef is incremented twice. Upon cache eviction or deletion, jsonParseFree is called for each cache slot, decrementing nJPRef twice. This is correct for the double-insertion case. However, if the object was also referenced externally (nJPRef was already >1 before the double insertion), the reference counting remains consistent. The real risk is that the same object appears in two cache slots, and if …

*PoC sketch:* Call jsonParseFuncArg with the same JSON input twice in a way that bypasses the cache lookup (e.g., with JSON_EDITABLE flag set, which may skip cache). If both resulting JsonParse objects are inserted into the cache, the same underlying object could be cached twice.

### `sqlite3.c [whereLoopAddBtreeIndex B: nRow/nOut arithmetic]` — L34 — conf=0.35
**Potential OOB read on pProbe->aiRowLogEst[saved_nEq] without bounds validation**

The expression `M = pProbe->aiRowLogEst[saved_nEq]` accesses the aiRowLogEst array using saved_nEq as an index without an explicit bounds check in this code path. The aiRowLogEst array has nColumn+1 entries. If a prior logic error or corrupted index metadata causes saved_nEq to exceed the number of index columns, this results in an out-of-bounds read. The read value (M) is then used in the cost comparison `x = M + logK + 10 - (nIn + rLogSize)`, potentially influencing optimizer decisions with attacker-controlled heap data.

*PoC sketch:* Create a schema with a multi-column index and craft a query with enough equality constraints on index columns that saved_nEq could exceed the index column count due to a logic error in the equality constraint counting loop (not shown in this snippet). The OOB read on aiRowLogEst would leak heap data…

### `sqlite3.c [whereLoopAddBtreeIndex C: multi-column+IN operator]` — L31 — conf=0.35
**Integer underflow in nOut estimation via nIn subtraction**

The code subtracts nIn from pNew->nOut in two places (the truthProb path and the STAT4 path). If nIn is large (from a large IN clause) and pNew->nOut is small, the subtraction can cause nOut to become very negative. While nOut is a LogEst (signed integer), a very negative value could propagate through cost calculations and potentially cause incorrect query plan selection or, in downstream code that converts LogEst to unsigned types, an integer wrap-around leading to unexpectedly large values affecting memory allocation or loop bounds.

*PoC sketch:* SELECT * FROM t WHERE a IN (1,2,3,...,10000); -- with a table having very few rows and a selective index, nOut could be small while nIn is large, causing underflow in the nOut -= nIn calculation.

### `sqlite3.c [sqlite3AlterFinishAddColumn C: default+affinity+codegen]` — L78 — conf=0.35
**Assert-only validation of addColOffset and nCol allows corrupted state in release builds**

Critical invariants are enforced only via assert() statements: pTab->u.tab.addColOffset>0 (line 78), IsOrdinaryTable(pTab) (line 77), and pNew->nCol>0 (line 93). In release builds (NDEBUG), these asserts are compiled out. If memory corruption or a logic error causes pTab->u.tab.addColOffset to be 0 or pTab->nCol to be 0, the code proceeds without validation. A zero addColOffset could cause incorrect schema rewriting. A zero nCol makes (nCol-1) = -1, leading to nAlloc = ((-1)/8)*8+8 = 8 (due to C integer division toward zero), which allocates space but with nCol=0 creates an inconsistent Table …

*PoC sketch:* No direct SQL trigger. This would require prior memory corruption of the Table structure (e.g., via a separate vulnerability) to zero out addColOffset or nCol fields, after which ALTER TABLE ADD COLUMN would proceed without the safety checks that asserts provide in debug builds.

### `sqlite3.c [whereLoopAddBtreeIndex C: multi-column+IN operator]` — L28 — conf=0.3
**Unchecked aiColumn access at saved_nEq index**

The condition pProbe->aiColumn[saved_nEq]>=0 accesses the aiColumn array at index saved_nEq without verifying that saved_nEq is within bounds. The aiColumn array has nColumn entries. If saved_nEq equals or exceeds nColumn (which could happen if the loop processes more terms than there are index columns), this is an out-of-bounds read. While the loop structure should prevent this, there is no explicit bounds check, unlike the STAT4 path's ALWAYS() assertion.

*PoC sketch:* Similar to the aiRowLogEst issue: a crafted query with more equality/IN constraints than index columns could cause saved_nEq to exceed the aiColumn array bounds.

