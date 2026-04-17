#!/usr/bin/env python3
"""libpng 1.6.45 T1 scan — GLM-5.1, function-level extracts.

Covers: pngrutil.c, pngrtran.c, pngpread.c (chunk handlers + transformations)

Run after API quota resets (08:00 BJT daily):
    python scripts/scan_libpng_t1.py
    python scripts/scan_libpng_t1.py --start 3   # resume from segment index 3
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "libpng"

SCANS = [
    ("pngrutil_A.c",  "pngrutil.c [png_handle_IHDR+png_handle_PLTE]"),
    ("pngrutil_B.c",  "pngrutil.c [png_handle_iCCP+png_handle_sPLT]"),
    ("pngrutil_C.c",  "pngrutil.c [png_inflate+png_decompress_chunk]"),
    ("pngrutil_D.c",  "pngrutil.c [png_handle_tRNS+png_handle_bKGD]"),
    ("pngrutil_E.c",  "pngrutil.c [png_handle_tEXt+png_handle_zTXt+png_handle_iTXt]"),
    ("pngrutil_F.c",  "pngrutil.c [png_handle_pCAL+png_handle_sCAL]"),
    ("pngrtran_A.c",  "pngrtran.c [png_set_quantize+gamma_functions]"),
    ("pngrtran_B.c",  "pngrtran.c [png_do_compose]"),
    ("pngrutil_G.c",  "pngrutil.c [png_combine_row+png_do_read_interlace]"),
    ("pngpread_A.c",  "pngpread.c [png_push_read_chunk]"),
]


async def scan_one(agent, fname: str, label: str, timeout: int = 240) -> dict:
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
    parser.add_argument("--timeout", type=int, default=240)
    args = parser.parse_args()

    import os
    from cyberai.models.glm import GLMAdapter
    api_key = os.environ.get("GLM_API_KEY", "GLM_KEY_REMOVED")
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

    out = pathlib.Path("research/libpng/t1_glm5_results.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump({"model": "glm-5.1", "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                   "results": results}, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved → {out}")


if __name__ == "__main__":
    asyncio.run(main())
