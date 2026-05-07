# CyberAI Pipeline A Scan — 2026-05-04

**Run**: `cyberai-all-scan` on `cyberai-main` (i-rj9iivpdwq8g59aa161m, us-west-1)
**Wall time**: 20h 2min (2026-05-03 14:19 → 2026-05-04 10:16 UTC)
**Model**: glm-5.1
**Source**: GitHub fxp/cyberai @ HEAD `a1590119` + the timeout patch series

## Pipeline overview

```
Pipeline A scan (20h)            ─→ 630 raw findings (57 CRITICAL + 170 HIGH + ...)
  └─ H. Adversarial verify       ─→ 70 actionable (58 CONFIRMED + 12 PARTIAL)
     glm-5.1, 3.5h, $0.45                  vs 98 FALSE_POSITIVE / 68 timeout
  └─ Validate (J1+J2+J3)         ─→ 2 high-confidence survivors
     ~17min, $1.5                       J1 NVD + J2 grounding + J3 glm-4-plus
  └─ Drafts (J5)                 ─→ 2 coordinated-disclosure email drafts
     glm-5.1, 4min, $0.003              ready in drafts/ for human review
```

## H — Adversarial verify (2026-05-05, glm-5.1, $0.45)

236 CRITICAL+HIGH findings re-judged on the ECS. Verifier prompt asked for
verdict + exploitability + known-CVE recall per finding.

| Severity | CONFIRMED | PARTIAL | FALSE_POSITIVE | ERROR (300s timeout) |
|---|---:|---:|---:|---:|
| CRITICAL | 18 | 4 | 23 | 19 |
| HIGH | 40 | 8 | 75 | 49 |
| **Total** | **58** | **12** | **98** | **68** |

GLM-recalled CVEs: `CVE-2022-25235` (expat) · `CVE-2019-8457` (sqlite) · `CVE-2023-5678` (openssl) · `CVE-2022-40303` / `CVE-2022-40304` (libxml2). These are **already-fixed published CVEs** and add no new disclosure value, but they show the static scanner has decent CVE recall on hardened libraries.

**Top concentrations** (CONFIRMED+PARTIAL, suggests true-positive hotspots):
- libssh2 — 18
- freetype — 16
- expat — 8
- libxml2 — 7

**Per-finding outputs**:
- Raw verdicts: [`verify/verify_20260505_175238.jsonl`](./verify/verify_20260505_175238.jsonl)
- Stats summary: [`verify/verify_20260505_175238_summary.json`](./verify/verify_20260505_175238_summary.json)

## Validate (J1+J2+J3) — 2026-05-07

70 CONFIRMED+PARTIAL findings put through three filters:

| Stage | What it does | Filtered |
|---|---|---:|
| **J1: NVD cross-ref** | Public NVD keyword search per finding (Python httpx, no LLM, ~7s/req rate-limited) | 0 |
| **J2: source-extract grounding** | Locate the actual `.c` extract that was scanned, read it. Filter `NO_EXTRACT` if the extract can't be matched (these are findings the scanner emitted at lines outside its own context) | 24 |
| **J3: glm-4-plus cross-check** | Re-verify with the actual code in context using a different BigModel model than glm-5.1. Filter `FALSE_POSITIVE`, `NEEDS_MORE_CONTEXT`, `code_match=no` | 44 |
| **Survivors** | Findings that pass all three | **2** |

Cost: J1 free, J3 ~$1.5 in glm-4-plus tokens. Total wall: ~17 min for 70 findings.

### 2 survivors

| # | Target | Sev | Conf | glm-4-plus | Title |
|---|---|---|---:|---|---|
| 1 | **libpng** | HIGH | 0.85 | CONFIRMED match=yes (0.9) | Integer Overflow in end_mask Calculation Due to Unchecked pixel_depth × row_width |
| 2 | **libxml2** | HIGH | 0.95 | PARTIAL match=yes (0.8) | Type confusion in namespace node ancestor traversal |

**Per-finding outputs**:
- All 70 with annotations: [`validate/validated_20260507_065336.jsonl`](./validate/validated_20260507_065336.jsonl)
- Survivors only: [`validate/survivors_20260507_065336.jsonl`](./validate/survivors_20260507_065336.jsonl)

## J5 — Disclosure drafts

GLM-5.1 authored a coordinated-disclosure email draft for each survivor:

- [`drafts/INDEX.md`](./drafts/INDEX.md) — index
- [`drafts/001_libpng_pngrutil_c__png_combine_row_.md`](./drafts/001_libpng_pngrutil_c__png_combine_row_.md)
- [`drafts/002_libxml2_libxml2_xpath_c__xmlXPathNextAncestor__ancestor_axis_travers.md`](./drafts/002_libxml2_libxml2_xpath_c__xmlXPathNextAncestor__ancestor_axis_travers.md)

Each draft has subject, summary, affected versions, vulnerability detail, repro sketch, suggested mitigation, and 90-day disclosure clause. **Drafts need human review before sending** — they are GLM-authored boilerplate, not vetted security content.

## Overall results

| Target | Segments | C 🔴 | H 🟠 | M 🟡 | L 🔵 | I ⚪ | Total | Report |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| **libpng** | 14 | 4 | 7 | 10 | 6 | 3 | **30** | [`libpng.md`](./libpng.md) |
| **expat** | 30 | 6 | 17 | 18 | 7 | 1 | **49** | [`expat.md`](./expat.md) |
| **curl** | 14 | 0 | 4 | 19 | 14 | 3 | **40** | [`curl.md`](./curl.md) |
| **nginx** | 33 | 2 | 10 | 16 | 10 | 3 | **41** | [`nginx.md`](./nginx.md) |
| **sqlite** | 29 | 4 | 14 | 27 | 21 | 0 | **66** | [`sqlite.md`](./sqlite.md) |
| **openssl** | 32 | 1 | 11 | 21 | 19 | 5 | **57** | [`openssl.md`](./openssl.md) |
| **zlib** | 25 | 1 | 11 | 12 | 12 | 3 | **39** | [`zlib.md`](./zlib.md) |
| **libxml2** | 32 | 10 | 23 | 36 | 18 | 5 | **92** | [`libxml2.md`](./libxml2.md) |
| **libssh2** | 40 | 14 | 32 | 36 | 11 | 3 | **96** | [`libssh2.md`](./libssh2.md) |
| **freetype** | 49 | 15 | 41 | 36 | 20 | 8 | **120** | [`freetype.md`](./freetype.md) |
| **TOTAL** | **298** | **57** | **170** | **231** | **138** | **34** | **630** | — |

## Top 30 CRITICAL+HIGH (sorted by confidence desc)

| Sev | Target | Loc | Conf | Title |
|---|---|---|---:|---|
| 🔴 CRITICAL | **nginx** | `ngx_http_parse.c [parse_unsafe_uri: trav` L48 | 1.0 | Path Traversal Bypass via `/..` and `/..?args` due to Flawed Traversal Detection Logic |
| 🔴 CRITICAL | **libssh2** | `libssh2/userauth.c [userauth_list A: sen` L83 | 1.0 | Integer Overflow in userauth_list leading to Heap Buffer Overflow |
| 🔴 CRITICAL | **libssh2** | `libssh2/channel.c [_libssh2_channel_read` L2160 | 1.0 | Integer Underflow and Out-of-Bounds Read in _libssh2_channel_read |
| 🔴 CRITICAL | **expat** | `xmlparse.c [doProlog E: ATTLIST-declarat` L79 | 0.95 | Off-by-one error in nameLen calculation includes null terminator |
| 🔴 CRITICAL | **sqlite** | `sqlite3.c [rtreenode: R-Tree node parser` L215862 | 0.95 | Heap Buffer Over-Read in rtreenode() due to Missing Header Offset in Bounds Check (CVE-201 |
| 🔴 CRITICAL | **zlib** | `zlib/inftrees.c [inflate_table D: incomp` L14 | 0.95 | Heap Buffer Overflow due to Missing Oversubscribed Tree Check |
| 🔴 CRITICAL | **libxml2** | `libxml2/parser.c [xmlParseNameComplex: n` L36 | 0.95 | Integer overflow in name length tracking allows OOB read via xmlDictLookup (CVE-2022-40303 |
| 🔴 CRITICAL | **libxml2** | `libxml2/parser.c [xmlExpandEntitiesInAtt` L12 | 0.95 | Entity content corruption on parse error (CVE-2022-40304) |
| 🔴 CRITICAL | **libssh2** | `libssh2/userauth.c [keyboard_interactive` L449 | 0.95 | Heap Buffer Overflow due to Missing newpw_len in Allocation Size |
| 🔴 CRITICAL | **libssh2** | `libssh2/scp.c [scp_recv A: banner+path p` L313 | 0.95 | Command Injection via Unquoted SCP Path |
| 🔴 CRITICAL | **freetype** | `freetype/ttcmap.c [tt_cmap4_char_map_bin` L1244 | 0.95 | Out-of-bounds heap read via unvalidated num_segs in cmap format 4 binary search |
| 🔴 CRITICAL | **sqlite** | `sqlite3.c [sqlite3VdbeMemFromBtree: payl` L63 | 0.92 | Integer overflow in offset+amt bypasses bounds check in sqlite3VdbeMemFromBtree |
| 🔴 CRITICAL | **freetype** | `freetype/ttload.c [tt_face_load_name B: ` L976 | 0.92 | Integer overflow in string bounds check allows out-of-bounds read |
| 🔴 CRITICAL | **freetype** | `freetype/cffload.c [cff_blend_doBlend A:` L1310 | 0.92 | Integer Overflow in numOperands Calculation Bypasses Stack Underflow Check |
| 🔴 CRITICAL | **libxml2** | `libxml2/parser.c [xmlParseNameComplex: n` L102 | 0.9 | Integer overflow in `len + 1` expression for CRLF adjustment |
| 🔴 CRITICAL | **libxml2** | `libxml2/parser.c [xmlExpandEntitiesInAtt` L38 | 0.9 | Entity content corruption on entity reference parse error |
| 🔴 CRITICAL | **freetype** | `freetype/cffload.c [cff_font_load C: cha` L2411 | 0.9 | Integer Overflow in FD Array Offset Calculation |
| 🔴 CRITICAL | **sqlite** | `sqlite3.c [sqlite3VdbeMemFromBtree: payl` L66 | 0.88 | Integer overflow in amt+1 causes undersized allocation and heap buffer overflow |
| 🔴 CRITICAL | **libpng** | `pngrutil.c [png_handle_tEXt]` L2614 | 0.85 | Integer Overflow in length+1 Leading to Heap Buffer Overflow |
| 🔴 CRITICAL | **expat** | `xmlparse.c [appendAttrValue A: tokenizer` L47 | 0.85 | Out-of-bounds read via XML_TOK_TRAILING_CR encoding mismatch |
| 🔴 CRITICAL | **nginx** | `ngx_http_v2.c [CONTINUATION+list-size-li` L1 | 0.85 | HTTP/2 CONTINUATION Frame Flood - Missing Header List Size Enforcement |
| 🔴 CRITICAL | **sqlite** | `sqlite3.c [sessionApplyChange: session r` L55 | 0.85 | Integer overflow in apVal allocation leading to heap buffer overflow on 32-bit systems |
| 🔴 CRITICAL | **libxml2** | `libxml2/xpath.c [xmlXPathNodeSetAdd: arr` L2925 | 0.85 | Integer Overflow in NodeSet Array Growth Size Calculation |
| 🔴 CRITICAL | **libxml2** | `libxml2/xpath.c [xmlXPathNodeSetAdd: arr` L2968 | 0.85 | Same Integer Overflow in xmlXPathNodeSetAddUnique Growth |
| 🔴 CRITICAL | **libxml2** | `libxml2/xpath.c [xmlXPathNodeSetMergeAnd` L3126 | 0.85 | Use-After-Free in namespace node deduplication during merge |
| 🔴 CRITICAL | **libxml2** | `libxml2/entities.c [xmlAddDtdEntity+NewE` L525 | 0.85 | Use-After-Free in growBufferReentrant macro - dangling `out` pointer after xmlRealloc |
| 🔴 CRITICAL | **libssh2** | `libssh2/transport.c [fullpacket A: decry` L195 | 0.85 | Integer Underflow in fullpacket_payload_len Calculation |
| 🔴 CRITICAL | **libssh2** | `libssh2/transport.c [fullpacket B: decom` L289 | 0.85 | Integer underflow in padding length subtraction |
| 🔴 CRITICAL | **libssh2** | `libssh2/transport.c [_libssh2_transport_` L547 | 0.85 | Untrusted length from get_len callback stored without validation |
| 🔴 CRITICAL | **libssh2** | `libssh2/transport.c [_libssh2_transport_` L839 | 0.85 | Integer underflow in bounds check enables heap buffer overflow |

## Sources

- ECS: `/root/cyberai/research/<target>/t1_glm5_results.json`
- OSS: `oss://cyberai-scan-results-us1/scans/2026-05-04/<target>.json`
- Local raw: `research/scan-2026-05-04/raw/<target>.json`
