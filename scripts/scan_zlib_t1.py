#!/usr/bin/env python3
"""zlib 1.3.1 T1 scan — GLM-5.1, function-level extracts.

Covers: inflate.c (decompression state machine, 1526 lines)
        inffast.c (fast-path inflate, 320 lines)
        inftrees.c (Huffman tree building, 299 lines)
        gzread.c (gzip read API, 602 lines)

Historical CVEs covered:
- CVE-2022-37434: heap buffer over-read in inflate() EXTRA state
  (gzip extra field parsing when head->extra is NULL, fixed in 1.2.13)
- CVE-2018-25032: memory corruption in deflate (out of range write)
- CVE-2016-9840/9841/9842/9843: various inflate OOB issues (fixed in 1.2.11)
- Scanning for NEW issues in 1.3.1

Run after API quota resets (08:00 BJT daily):
    python scripts/scan_zlib_t1.py
    python scripts/scan_zlib_t1.py --start 10   # resume from index 10
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "zlib"

SCANS = [
    # ── inflate.c: init/reset (potential OOB in state allocation) ──
    ("inflate_init.c",  "zlib/inflate.c [inflateResetKeep+inflateReset: state check]"),
    ("inflate_init2.c", "zlib/inflate.c [inflateReset2+inflateInit2_: window+state alloc]"),
    ("inflate_init3.c", "zlib/inflate.c [inflateInit_+inflateGetHeader+setDictionary]"),
    # ── inflate.c: main decompression state machine ──
    ("inflate_A.c",     "zlib/inflate.c [inflate() HEAD+ID+CM+FLAGS+TIME: gzip header parse]"),
    ("inflate_B.c",     "zlib/inflate.c [inflate() EXLEN+EXTRA: gzip extra field (CVE-2022-37434)]"),
    ("inflate_C.c",     "zlib/inflate.c [inflate() NAME+COMMENT+HCRC: gzip name/comment]"),
    ("inflate_D.c",     "zlib/inflate.c [inflate() TYPE+STORED+LEN: block type dispatch]"),
    ("inflate_E.c",     "zlib/inflate.c [inflate() CODELENS+LENS+DIST: Huffman decode]"),
    ("inflate_F.c",     "zlib/inflate.c [inflate() LEN_+MATCH+LIT: back-reference copy]"),
    ("inflate_G.c",     "zlib/inflate.c [inflate() CHECK+LENGTH+DONE: checksum+end states]"),
    ("inflate_H.c",     "zlib/inflate.c [inflateGetDictionary+setDictionary+Copy+Mark]"),
    ("inflate_I.c",     "zlib/inflate.c [inflateUndermine+inflateValidate+CodesUsed]"),
    # ── inffast.c: fast-path inflate (highest OOB risk) ──
    ("inffast_A.c",     "zlib/inffast.c [inflate_fast A: literal/length decode entry]"),
    ("inffast_B.c",     "zlib/inffast.c [inflate_fast B: back-reference copy loop A]"),
    ("inffast_D.c",     "zlib/inffast.c [inflate_fast C: copy + distance check]"),
    ("inffast_C.c",     "zlib/inffast.c [inflate_fast D: output write + end check]"),
    # ── inftrees.c: Huffman tree builder ──
    ("inftrees_A.c",    "zlib/inftrees.c [inflate_table A: code length counting]"),
    ("inftrees_A2.c",   "zlib/inftrees.c [inflate_table B: canonical codes + index]"),
    ("inftrees_B.c",    "zlib/inftrees.c [inflate_table C: table fill + bit-length dist]"),
    ("inftrees_C.c",    "zlib/inftrees.c [inflate_table D: incomplete code + oversubscribed check]"),
    # ── gzread.c: gzip read API ──
    ("gzread_A.c",      "zlib/gzread.c [gz_load+gz_avail: input buffer fill]"),
    ("gzread_A2.c",     "zlib/gzread.c [gz_look+gz_decomp: gzip header detect + decompress]"),
    ("gzread_B.c",      "zlib/gzread.c [gz_fetch+gz_skip: fetch buffer + newline scan]"),
    ("gzread_B2.c",     "zlib/gzread.c [gzread+gzfread: public read API]"),
    ("gzread_C.c",      "zlib/gzread.c [gzgetc+gzungetc+gzgets+gzrewind: string read API]"),
]


async def scan_one(agent, fname: str, label: str, timeout: int = 120) -> dict:
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
    parser.add_argument("--delay", type=int, default=25)
    parser.add_argument("--timeout", type=int, default=120)
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

    out = pathlib.Path("research/zlib/t1_glm5_results.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump({"model": "glm-5.1", "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                   "target": "zlib-1.3.1", "results": results}, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved → {out}")


if __name__ == "__main__":
    asyncio.run(main())
