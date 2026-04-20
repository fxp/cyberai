#!/usr/bin/env python3
"""libpng 1.6.45 V3 scan — 2800-char single-function extracts."""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))
EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "libpng_v3"

SCANS = [
    ("ihdr.c",        "pngrutil.c [png_handle_IHDR]"),
    ("plte.c",        "pngrutil.c [png_handle_PLTE]"),
    ("inflate.c",     "pngrutil.c [png_inflate]"),
    ("decompress.c",  "pngrutil.c [png_decompress_chunk]"),
    ("iccp.c",        "pngrutil.c [png_handle_iCCP]"),
    ("trns.c",        "pngrutil.c [png_handle_tRNS]"),
    ("text.c",        "pngrutil.c [png_handle_tEXt]"),
    ("ztxt.c",        "pngrutil.c [png_handle_zTXt]"),
    ("itxt.c",        "pngrutil.c [png_handle_iTXt]"),
    ("pcal.c",        "pngrutil.c [png_handle_pCAL]"),
    ("scal.c",        "pngrutil.c [png_handle_sCAL]"),
    ("combine_row.c", "pngrutil.c [png_combine_row]"),
    ("compose.c",     "pngrtran.c [png_do_compose]"),
    ("push_chunk.c",  "pngpread.c [png_push_read_chunk]"),
]

async def scan_one(agent, fname, label, timeout=90):
    content = (EXTRACTS_DIR / fname).read_text()
    print(f"[{time.strftime('%H:%M:%S')}] {label} ({len(content):,} chars)...", flush=True)
    t0 = time.time()
    try:
        result = await asyncio.wait_for(
            agent.scan_file(content, label, timeout=float(timeout)), timeout=timeout+30)
        elapsed = time.time() - t0
        print(f"  Done {elapsed:.0f}s — {result.vuln_count} findings", flush=True)
        for f in result.findings:
            print(f"  [{f.severity.value.upper()}] {f.title} (L{f.line_start}, conf={f.confidence:.0%})", flush=True)
        if result.error:
            print(f"  Error: {result.error}", flush=True)
        return {"file": label, "chars": len(content), "elapsed_s": round(elapsed, 1), "status": "ok",
                "findings": [{"severity": f.severity.value, "title": f.title,
                               "line_start": f.line_start, "confidence": f.confidence,
                               "description": f.description, "poc": f.proof_of_concept}
                              for f in result.findings]}
    except asyncio.TimeoutError:
        print(f"  TIMEOUT {time.time()-t0:.0f}s", flush=True)
        return {"file": label, "chars": len(content), "status": "timeout", "findings": []}
    except Exception as e:
        print(f"  Error: {e}", flush=True)
        return {"file": label, "chars": len(content), "status": f"error:{e}", "findings": []}

async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--start", type=int, default=0)
    parser.add_argument("--delay", type=int, default=20)
    parser.add_argument("--timeout", type=int, default=90)
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

    out = pathlib.Path("research/libpng/t1_v3_results.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump({"model": "glm-5.1", "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                   "results": results}, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved → {out}")

if __name__ == "__main__":
    asyncio.run(main())
