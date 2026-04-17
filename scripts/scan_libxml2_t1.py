#!/usr/bin/env python3
"""libxml2 2.13.5 T1 scan — GLM-5.1, function-level extracts.

Covers: parser.c (XML parser, 14,194 lines)
        xpath.c  (XPath evaluation, 13,557 lines)
        entities.c (entity resolution, 979 lines)

Historical CVEs covered:
- CVE-2022-40303: integer overflow in xmlParseNameComplex (name length)
- CVE-2022-40304: entity reference dict corruption via recursive entities
- CVE-2023-28484: NULL dereference in XSD schema validation (xmlSchemas.c)
- CVE-2023-29469: NULL dereference when lax parsing (hashing)
- CVE-2021-3541: exponential entity expansion (billion laughs)
- CVE-2021-3537: NULL dereference in recovering mode
- Scanning for NEW issues in 2.13.5

Run after API quota resets (08:00 BJT daily):
    python scripts/scan_libxml2_t1.py
    python scripts/scan_libxml2_t1.py --start 10   # resume from index 10
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "libxml2"

SCANS = [
    # ── parser.c: character reference + name parsing ──
    ("parser_charref.c",       "libxml2/parser.c [xmlParseCharRef: char ref decoder L2578-2750]"),
    ("parser_name_A.c",        "libxml2/parser.c [xmlParseNameComplex: name len+colon (CVE-2022-40303)]"),
    # ── parser.c: entity value + attribute value parsing ──
    ("parser_entity_val_A.c",  "libxml2/parser.c [xmlParseEntityValue A: entity literal L3870-4032]"),
    ("parser_entity_val_B.c",  "libxml2/parser.c [xmlParseEntityValue B: entity expansion L4032-4168]"),
    ("parser_entity_val_C.c",  "libxml2/parser.c [xmlExpandEntitiesInAttValue (CVE-2022-40304 area)]"),
    ("parser_att_A.c",         "libxml2/parser.c [xmlParseAttValueInternal A: attrib decode L4303-4420]"),
    ("parser_att_B.c",         "libxml2/parser.c [xmlParseAttValueInternal B: entity in attrib L4420-4535]"),
    # ── parser.c: CDATA + entity declarations ──
    ("parser_chardata.c",      "libxml2/parser.c [xmlParseCharDataInternal: CDATA + text L4775-4910]"),
    ("parser_entdecl_A.c",     "libxml2/parser.c [xmlParseEntityDecl A: decl entry+type L5698-5800]"),
    ("parser_entdecl_B.c",     "libxml2/parser.c [xmlParseEntityDecl B: value+system L5800-5920]"),
    # ── parser.c: namespaced start tag (xmlParseStartTag2) ──
    ("parser_tag2_A.c",        "libxml2/parser.c [xmlParseStartTag2 A: ns prefix resolve L8974-9095]"),
    ("parser_tag2_B.c",        "libxml2/parser.c [xmlParseStartTag2 B: attrib accumulator L9095-9215]"),
    ("parser_tag2_C.c",        "libxml2/parser.c [xmlParseStartTag2 C: ns URI lookup L9215-9340]"),
    ("parser_tag2_D.c",        "libxml2/parser.c [xmlParseStartTag2 D: ns expand+attrib dedup L9340-9460]"),
    ("parser_tag2_E.c",        "libxml2/parser.c [xmlParseStartTag2 E: attrib flush+end L9460-9570]"),
    # ── xpath.c: node set operations ──
    ("xpath_nodeset_A.c",      "libxml2/xpath.c [xmlXPathNodeSetAdd: array growth OOB L2910-3075]"),
    ("xpath_nodeset_B.c",      "libxml2/xpath.c [xmlXPathNodeSetMergeAndClear: merge+dedup L3115-3250]"),
    # ── xpath.c: comparison and equality ──
    ("xpath_equality.c",       "libxml2/xpath.c [xmlXPathEqualNodeSets: equality compare L5831-5940]"),
    ("xpath_compare_A.c",      "libxml2/xpath.c [xmlXPathCompareValues: type coercion compare L6299-6415]"),
    # ── xpath.c: axis iterators ──
    ("xpath_axis_A.c",         "libxml2/xpath.c [xmlXPathNextAncestor: ancestor axis traversal L6954-7060]"),
    # ── xpath.c: string functions ──
    ("xpath_string_A.c",       "libxml2/xpath.c [xmlXPathSubstringFunction+Before: alloc L8051-8200]"),
    ("xpath_string_B.c",       "libxml2/xpath.c [xmlXPathSubstringAfter+Normalize+Translate L8200-8336]"),
    # ── xpath.c: name parsing + path compilation ──
    ("xpath_name_A.c",         "libxml2/xpath.c [xmlXPathCurrentChar+ParseNameComplex A L8698-8855]"),
    ("xpath_name_B.c",         "libxml2/xpath.c [xmlXPathParseNameComplex B: qualified name L8855-8990]"),
    ("xpath_comp_A.c",         "libxml2/xpath.c [xmlXPathCompPathExpr: path expression compile L9544-9665]"),
    ("xpath_filter.c",         "libxml2/xpath.c [xmlXPathNodeSetFilter: predicate filter L10471-10595]"),
    # ── entities.c: entity table operations ──
    ("entities_A.c",           "libxml2/entities.c [xmlCreateEntity+xmlAddEntity: create+insert L1-155]"),
    ("entities_B.c",           "libxml2/entities.c [xmlGetPredefinedEntity+GetEntityFromTable L155-286]"),
    ("entities_C.c",           "libxml2/entities.c [xmlGetDocEntity+DtdEntity: lookup chain L286-430]"),
    ("entities_D.c",           "libxml2/entities.c [xmlAddDtdEntity+NewEntity+GetParameter L430-562]"),
    ("entities_E.c",           "libxml2/entities.c [entity content handling+value encode L562-726]"),
    ("entities_F.c",           "libxml2/entities.c [entity value escaping+output+cleanup L726-908]"),
]


async def scan_one(agent, fname: str, label: str, timeout: int = 150) -> dict:
    content = (EXTRACTS_DIR / fname).read_text()
    print(f"[{time.strftime('%H:%M:%S')}] {label} ({len(content):,} chars)...", flush=True)
    t0 = time.time()
    try:
        result = await asyncio.wait_for(
            agent.scan_file(content, label, timeout=float(timeout)), timeout=timeout + 30)
        elapsed = time.time() - t0
        print(f"  Done {elapsed:.0f}s — {result.vuln_count} findings", flush=True)
        for f in result.findings:
            print(f"  [{f.severity.value.upper()}] {f.title} (L{f.line_start}, conf={f.confidence:.0%})", flush=True)
        if result.error:
            print(f"  Error: {result.error}", flush=True)
        return {
            "file": label, "chars": len(content),
            "elapsed_s": round(elapsed, 1), "status": "ok",
            "findings": [
                {"severity": f.severity.value, "title": f.title,
                 "line_start": f.line_start, "confidence": f.confidence,
                 "description": f.description, "poc": f.proof_of_concept}
                for f in result.findings
            ],
        }
    except asyncio.TimeoutError:
        print(f"  TIMEOUT {time.time()-t0:.0f}s", flush=True)
        return {"file": label, "chars": len(content), "status": "timeout", "findings": []}
    except Exception as e:
        print(f"  Error: {e}", flush=True)
        return {"file": label, "chars": len(content), "status": f"error:{e}", "findings": []}


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--start", type=int, default=0)
    parser.add_argument("--delay", type=int, default=30)
    parser.add_argument("--timeout", type=int, default=150)
    args = parser.parse_args()

    import os
    from cyberai.models.glm import GLMAdapter
    api_key = os.environ.get("GLM_API_KEY", "GLM_KEY_REMOVED")
    agent = GLMAdapter(model_name="glm-5.1", api_key=api_key)

    results = []
    scans = SCANS[args.start:]
    print(f"Scanning {len(scans)} segments (start={args.start}), {args.delay}s delay\n")

    for i, (fname, label) in enumerate(scans):
        if i > 0:
            print(f"  [waiting {args.delay}s...]", flush=True)
            await asyncio.sleep(args.delay)
        r = await scan_one(agent, fname, label, timeout=args.timeout)
        results.append(r)

    out = pathlib.Path("research/libxml2/t1_glm5_results.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump({"model": "glm-5.1", "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                   "target": "libxml2-2.13.5", "results": results}, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved → {out}")


if __name__ == "__main__":
    asyncio.run(main())
