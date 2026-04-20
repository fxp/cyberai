#!/usr/bin/env python3
"""SQLite 3.49.1 T1 scan — GLM-5.1, function-level extracts.

Covers: sqlite3.c amalgamation (261,454 lines)
        - Format string handler: sqlite3_str_vappendf (CVE-2022-35737 class)
        - SQL tokenizer: sqlite3GetToken
        - SQL parser entry: sqlite3RunParser
        - Record decoder: sqlite3VdbeSerialGet
        - B-tree payload: sqlite3VdbeMemFromBtree
        - Window function: windowAggStep (CVE-2019-5018 class)
        - R-Tree node: rtreenode (CVE-2019-8457 class)
        - Sessions module: sessionReadRecord (CVE-2023-7104 class)
        - WHERE optimizer: whereLoopAddBtreeIndex (CVE-2019-16168 class)
        - ALTER TABLE: sqlite3AlterFinishAddColumn (CVE-2020-35527 class)
        - JSON parser: jsonParseFuncArg

Historical CVEs covered:
- CVE-2019-5018: UAF in window function (windowAggStep)
- CVE-2019-8457: Heap OOB in rtreenode()
- CVE-2019-16168: Division-by-zero in whereLoopAddBtreeIndex
- CVE-2020-35527: OOB in ALTER TABLE (sqlite3AlterFinishAddColumn)
- CVE-2022-35737: Large string in sqlite3_str_vappendf
- CVE-2023-7104: Stack OOB in sessions (sessionReadRecord)
- Scanning for NEW issues in 3.49.1

Run after API quota resets (08:00 BJT daily):
    python scripts/scan_sqlite_t1.py
    python scripts/scan_sqlite_t1.py --start 10   # resume from index 10
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "sqlite"

SCANS = [
    # ── Format String Handler (CVE-2022-35737 class) ──
    ("vprintf_A.c",       "sqlite3.c [sqlite3_str_vappendf A: format-loop+width+precision]"),
    ("vprintf_B.c",       "sqlite3.c [sqlite3_str_vappendf B: integer+radix flag setup]"),
    ("vprintf_C.c",       "sqlite3.c [sqlite3_str_vappendf C: integer-to-string+buf sizing]"),
    ("vprintf_D.c",       "sqlite3.c [sqlite3_str_vappendf D: numeric output+thousands sep]"),
    ("vprintf_E.c",       "sqlite3.c [sqlite3_str_vappendf E: float+EXP+GENERIC]"),
    ("vprintf_F.c",       "sqlite3.c [sqlite3_str_vappendf F: string+SQL-escape+TOKEN]"),
    # ── SQL Tokenizer ──
    ("tokenizer_A.c",     "sqlite3.c [sqlite3GetToken A: CC_ class switch method+blob+string]"),
    ("tokenizer_B.c",     "sqlite3.c [sqlite3GetToken B: keyword+ident+end-of-input]"),
    ("runparser.c",       "sqlite3.c [sqlite3RunParser: tokenize+parse loop entry]"),
    # ── Record Decoder (B-tree payload) ──
    ("serial_get_A.c",    "sqlite3.c [sqlite3VdbeSerialGet A: type decode + int serialization]"),
    ("serial_get_B.c",    "sqlite3.c [sqlite3VdbeSerialGet B: string/blob+float types]"),
    ("btree_mem_A.c",     "sqlite3.c [sqlite3VdbeMemFromBtree: payload fetch with bounds check]"),
    ("btree_mem_B.c",     "sqlite3.c [sqlite3VdbeMemFromBtreeZeroOffset: direct page access]"),
    # ── Window Function (CVE-2019-5018 class) ──
    ("window_agg_A.c",    "sqlite3.c [windowAggStep A: window-frame advance (CVE-2019-5018)]"),
    ("window_agg_B.c",    "sqlite3.c [windowAggStep B: aggregate call + return]"),
    # ── R-Tree Extension (CVE-2019-8457 class) ──
    ("rtree_A.c",         "sqlite3.c [rtreenode: R-Tree node parser (CVE-2019-8457)]"),
    ("rtree_B.c",         "sqlite3.c [rtreeCheckNode A: R-Tree node consistency + child walk]"),
    ("rtree_C.c",         "sqlite3.c [rtreeCheckNode B: bounding box + rtreeCheckTable]"),
    # ── Sessions Module (CVE-2023-7104 class) ──
    ("session_A.c",       "sqlite3.c [sessionReadRecord A: session change record (CVE-2023-7104)]"),
    ("session_B.c",       "sqlite3.c [sessionApplyChange: session record applier]"),
    # ── WHERE Optimizer (CVE-2019-16168 class) ──
    ("whereloop_A.c",     "sqlite3.c [whereLoopAddBtreeIndex A: index-scan+cost (CVE-2019-16168)]"),
    ("whereloop_B.c",     "sqlite3.c [whereLoopAddBtreeIndex B: nRow/nOut arithmetic]"),
    ("whereloop_C.c",     "sqlite3.c [whereLoopAddBtreeIndex C: multi-column+IN operator]"),
    ("whereloop_D.c",     "sqlite3.c [whereLoopAddBtreeIndex D: skip-scan+tail conditions]"),
    # ── ALTER TABLE (CVE-2020-35527 class) ──
    ("alter_table_A.c",   "sqlite3.c [sqlite3AlterFinishAddColumn A: schema rebuild (CVE-2020-35527)]"),
    ("alter_table_B.c",   "sqlite3.c [sqlite3AlterFinishAddColumn B: column validate+constraints]"),
    ("alter_table_C.c",   "sqlite3.c [sqlite3AlterFinishAddColumn C: default+affinity+codegen]"),
    # ── JSON Parser ──
    ("json_parse_A.c",    "sqlite3.c [jsonParseFuncArg: JSON input validation+cache lookup]"),
    ("json_parse_B.c",    "sqlite3.c [jsonParseFunc: JSON tree building+node allocation]"),
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
    api_key = os.environ.get("GLM_API_KEY", "")
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

    out = pathlib.Path("research/sqlite/t1_glm5_results.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump({"model": "glm-5.1", "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                   "target": "sqlite-3.49.1", "results": results}, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved → {out}")


if __name__ == "__main__":
    asyncio.run(main())
