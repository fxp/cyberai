#!/usr/bin/env python3
"""libexpat 2.6.4 T1 scan — GLM-5.1, function-level extracts.

Covers: xmlparse.c (all major parsing functions)

Target: expat-2.6.4/lib/xmlparse.c
History: CVE-2021-46143 (doProlog integer overflow), CVE-2022-22822-22827 (lookup/addBinding/
         defineAttribute/storeAtts integer overflows), all fixed in 2.4.3.
         Scanning 2.6.4 for NEW issues post-fix.

Segment order: high-risk first (storeAtts, addBinding, lookup, appendAttributeValue, doProlog),
then medium risk (doContent, storeEntityValue, processXmlDecl, defineAttribute, poolGrow, build_model)

Run after API quota resets (08:00 BJT daily):
    python scripts/scan_expat_t1.py
    python scripts/scan_expat_t1.py --start 5   # resume from segment index 5
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "expat"

# Ordered by security priority (highest risk first)
SCANS = [
    # storeAtts: historically CVE-prone attribute storage
    ("storeAtts_A.c",        "xmlparse.c [storeAtts A: setup+default-attrs]"),
    ("storeAtts_B.c",        "xmlparse.c [storeAtts B: namespace-attr-detect]"),
    ("storeAtts_C.c",        "xmlparse.c [storeAtts C: dup-check+pool-storage]"),
    ("storeAtts_D.c",        "xmlparse.c [storeAtts D: binding-creation]"),
    ("storeAtts_E.c",        "xmlparse.c [storeAtts E: addBinding-calls]"),
    ("storeAtts_F.c",        "xmlparse.c [storeAtts F: id-tracking+cleanup]"),
    # addBinding: namespace binding allocation
    ("addBinding_A.c",       "xmlparse.c [addBinding A: URI-lookup+malloc]"),
    ("addBinding_B.c",       "xmlparse.c [addBinding B: prefix-chain-update]"),
    # lookup: hash table with dynamic growth
    ("lookup.c",             "xmlparse.c [lookup: hash-table-growth]"),
    # appendAttributeValue: attribute expansion
    ("appendAttrValue_A.c",  "xmlparse.c [appendAttrValue A: tokenizer-loop]"),
    ("appendAttrValue_B.c",  "xmlparse.c [appendAttrValue B: entity-ref-expand]"),
    ("appendAttrValue_C.c",  "xmlparse.c [appendAttrValue C: pool-append-completion]"),
    # doProlog: XML prolog parsing (historically largest CVE surface)
    ("doProlog_A.c",         "xmlparse.c [doProlog A: entry+XML-decl]"),
    ("doProlog_B.c",         "xmlparse.c [doProlog B: DOCTYPE+subset]"),
    ("doProlog_C.c",         "xmlparse.c [doProlog C: ELEMENT-declarations]"),
    ("doProlog_D.c",         "xmlparse.c [doProlog D: ENTITY-declarations]"),
    ("doProlog_E.c",         "xmlparse.c [doProlog E: ATTLIST-declarations]"),
    # doContent: main content dispatch
    ("doContent_A.c",        "xmlparse.c [doContent A: entry+startTag]"),
    ("doContent_B.c",        "xmlparse.c [doContent B: endTag-level]"),
    ("doContent_C.c",        "xmlparse.c [doContent C: charData+PI+comment]"),
    ("doContent_D.c",        "xmlparse.c [doContent D: CDATA+entityRef]"),
    ("doContent_E.c",        "xmlparse.c [doContent E: internal-entity-push]"),
    ("doContent_F.c",        "xmlparse.c [doContent F: error+EOF]"),
    # storeEntityValue: entity value storage
    ("storeEntityValue_A.c", "xmlparse.c [storeEntityValue A: tokenizer+copy]"),
    ("storeEntityValue_B.c", "xmlparse.c [storeEntityValue B: entity-refs]"),
    # smaller functions
    ("processXmlDecl.c",     "xmlparse.c [processXmlDecl: XML-decl-attrs]"),
    ("defineAttribute.c",    "xmlparse.c [defineAttribute: DTD-att-uniqueness]"),
    ("poolGrow.c",           "xmlparse.c [poolGrow: string-pool-realloc]"),
    ("build_model_A.c",      "xmlparse.c [build_model A: tree-allocation]"),
    ("build_model_B.c",      "xmlparse.c [build_model B: tree-population]"),
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
    parser.add_argument("--start", type=int, default=0, help="Resume from segment index")
    parser.add_argument("--delay", type=int, default=30, help="Seconds between scans")
    parser.add_argument("--timeout", type=int, default=150, help="Per-scan timeout (default 150s)")
    args = parser.parse_args()

    import os
    from cyberai.models.glm import GLMAdapter
    api_key = os.environ.get("GLM_API_KEY", "")
    agent = GLMAdapter(model_name="glm-5.1", api_key=api_key)

    results = []
    scans = SCANS[args.start:]
    print(f"Scanning {len(scans)} segments (starting at {args.start}), {args.delay}s delay\n")

    for i, (fname, label) in enumerate(scans):
        if i > 0:
            print(f"  [waiting {args.delay}s...]", flush=True)
            await asyncio.sleep(args.delay)
        r = await scan_one(agent, fname, label, timeout=args.timeout)
        results.append(r)

    out = pathlib.Path("research/expat/t1_glm5_results.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump({"model": "glm-5.1", "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                   "target": "expat-2.6.4", "results": results}, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved → {out}")


if __name__ == "__main__":
    asyncio.run(main())
