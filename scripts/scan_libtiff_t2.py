#!/usr/bin/env python3
"""LibTIFF 4.7.0 T2 scan — GLM-5.1, function-level extracts.

Priority order: codec decode paths > directory parsing > read paths

Run:
    python scripts/scan_libtiff_t2.py
    python scripts/scan_libtiff_t2.py --start 5   # resume from segment index

Results → research/libtiff/t2_glm5_results.json
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "libtiff"

# Priority: codec decoders first (highest attack surface for malformed TIFF)
SCANS = [
    # ZIP/zlib codec — decompress into fixed-size strip buffer
    ("tif_zip_ZIPDecode_A.c",               "libtiff/tif_zip.c [ZIPDecode-A]"),
    ("tif_zip_ZIPDecode_B.c",               "libtiff/tif_zip.c [ZIPDecode-B]"),
    ("tif_zip_ZIPEncode_A.c",               "libtiff/tif_zip.c [ZIPEncode-A]"),
    ("tif_zip_ZIPEncode_B.c",               "libtiff/tif_zip.c [ZIPEncode-B]"),
    # PixarLog codec — floating-point log encoding with predictor
    ("tif_pixarlog_decode_A.c",     "libtiff/tif_pixarlog.c [PixarLogDecode-A]"),
    ("tif_pixarlog_decode_B.c",     "libtiff/tif_pixarlog.c [PixarLogDecode-B]"),
    ("tif_pixarlog_encode.c",       "libtiff/tif_pixarlog.c [PixarLogEncode]"),
    # LUV codec — LogLuv 24/32-bit decode
    ("tif_luv_LogLuvDecode24.c",            "libtiff/tif_luv.c [LogLuvDecode24]"),
    ("tif_luv_LogLuvDecode32.c",            "libtiff/tif_luv.c [LogLuvDecode32]"),
    ("tif_luv_LogLuvEncode24.c",            "libtiff/tif_luv.c [LogLuvEncode24]"),
    ("tif_luv_LogLuvEncode32.c",            "libtiff/tif_luv.c [LogLuvEncode32]"),
    # GetImage — RGBA compositing with per-sample arithmetic
    ("tif_getimage_TIFFReadRGBAImage.c",    "libtiff/tif_getimage.c [TIFFReadRGBAImage]"),
    ("tif_getimage_gtTileSeparate_A.c",     "libtiff/tif_getimage.c [gtTileSeparate-A]"),
    ("tif_getimage_gtTileSeparate_B.c",     "libtiff/tif_getimage.c [gtTileSeparate-B]"),
    ("tif_getimage_gtStripSeparate_A.c",    "libtiff/tif_getimage.c [gtStripSeparate-A]"),
    ("tif_getimage_gtStripSeparate_B.c",    "libtiff/tif_getimage.c [gtStripSeparate-B]"),
    # Read paths — strip/scanline
    ("tif_read_TIFFReadScanline.c",         "libtiff/tif_read.c [TIFFReadScanline]"),
    ("tif_read_TIFFReadEncodedStrip.c",     "libtiff/tif_read.c [TIFFReadEncodedStrip]"),
    # Directory parsing — integer overflow in tag value allocation
    ("tif_dirread_TIFFReadDirectory_A.c",   "libtiff/tif_dirread.c [TIFFReadDirectory-A]"),
    ("tif_dirread_TIFFReadDirectory_B.c",   "libtiff/tif_dirread.c [TIFFReadDirectory-B]"),
    ("tif_dirread_TIFFReadDirectory_C.c",   "libtiff/tif_dirread.c [TIFFReadDirectory-C]"),
    ("tif_dirread_TIFFReadDirectory_D.c",   "libtiff/tif_dirread.c [TIFFReadDirectory-D]"),
    ("tif_dirread_TIFFReadDirectory_E.c",   "libtiff/tif_dirread.c [TIFFReadDirectory-E]"),
    ("tif_dirread_TIFFReadDirectory_F.c",   "libtiff/tif_dirread.c [TIFFReadDirectory-F]"),
    ("tif_dirread_TIFFReadDirectory_G.c",   "libtiff/tif_dirread.c [TIFFReadDirectory-G]"),
    ("tif_dirread_TIFFReadDirectory_H.c",   "libtiff/tif_dirread.c [TIFFReadDirectory-H]"),
    ("tif_dirread_TIFFReadDirectory_I.c",   "libtiff/tif_dirread.c [TIFFReadDirectory-I]"),
    # Tag fetch — unbounded allocation from tag count field
    ("tif_dirread_TIFFFetchNormalTag_A.c",  "libtiff/tif_dirread.c [TIFFFetchNormalTag-A]"),
    ("tif_dirread_TIFFFetchNormalTag_B.c",  "libtiff/tif_dirread.c [TIFFFetchNormalTag-B]"),
    ("tif_dirread_TIFFFetchNormalTag_C.c",  "libtiff/tif_dirread.c [TIFFFetchNormalTag-C]"),
    ("tif_dirread_TIFFFetchNormalTag_D.c",  "libtiff/tif_dirread.c [TIFFFetchNormalTag-D]"),
    ("tif_dirread_TIFFFetchNormalTag_E.c",  "libtiff/tif_dirread.c [TIFFFetchNormalTag-E]"),
    ("tif_dirread_TIFFFetchNormalTag_F.c",  "libtiff/tif_dirread.c [TIFFFetchNormalTag-F]"),
    ("tif_dirread_TIFFFetchNormalTag_G.c",  "libtiff/tif_dirread.c [TIFFFetchNormalTag-G]"),
    ("tif_dirread_TIFFFetchNormalTag_H.c",  "libtiff/tif_dirread.c [TIFFFetchNormalTag-H]"),
    ("tif_dirread_TIFFFetchNormalTag_I.c",  "libtiff/tif_dirread.c [TIFFFetchNormalTag-I]"),
    ("tif_dirread_TIFFFetchNormalTag_J.c",  "libtiff/tif_dirread.c [TIFFFetchNormalTag-J]"),
    ("tif_dirread_TIFFFetchNormalTag_Z.c",  "libtiff/tif_dirread.c [TIFFFetchNormalTag-Z]"),
]


async def scan_one(agent, fname: str, label: str, timeout: int = 600) -> dict:
    fpath = EXTRACTS_DIR / fname
    if not fpath.exists():
        print(f"  SKIP (file not found): {fname}", flush=True)
        return {"file": label, "chars": 0, "status": "skip", "findings": []}
    content = fpath.read_text()
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
    parser.add_argument("--end", type=int, default=None, help="Stop after this index (exclusive)")
    parser.add_argument("--delay", type=int, default=30)
    parser.add_argument("--timeout", type=int, default=600)
    args = parser.parse_args()

    import os
    from cyberai.models.glm import GLMAdapter
    api_key = os.environ.get("GLM_API_KEY", "")
    agent = GLMAdapter(model_name="glm-5.1", api_key=api_key)

    end = args.end if args.end is not None else len(SCANS)
    scans = SCANS[args.start:end]
    print(f"LibTIFF T2 scan: {len(scans)} segments (idx {args.start}–{end-1}), {args.delay}s delay\n")

    out_path = pathlib.Path("research/libtiff/t2_glm5_results.json")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    existing = []
    if out_path.exists() and args.start > 0:
        with open(out_path) as f:
            data = json.load(f)
            existing = data.get("results", [])
        print(f"  Loaded {len(existing)} existing results\n")

    results = list(existing)
    for i, (fname, label) in enumerate(scans):
        if i > 0:
            print(f"  [waiting {args.delay}s...]", flush=True)
            await asyncio.sleep(args.delay)
        r = await scan_one(agent, fname, label, timeout=args.timeout)
        results.append(r)
        with open(out_path, "w") as f:
            json.dump({"model": "glm-5.1", "target": "LibTIFF 4.7.0",
                       "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                       "results": results}, f, indent=2, ensure_ascii=False)

    total = sum(len(r["findings"]) for r in results)
    high_crit = sum(1 for r in results for f in r["findings"]
                    if f["severity"] in ("critical", "high") and f["confidence"] >= 0.65)
    print(f"\n{'='*60}")
    print(f"LibTIFF T2 complete: {len(results)} segments, {total} raw findings, {high_crit} high/critical ≥65%")
    print(f"Results → {out_path}")


if __name__ == "__main__":
    asyncio.run(main())
