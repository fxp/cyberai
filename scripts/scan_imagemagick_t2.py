#!/usr/bin/env python3
"""ImageMagick 7.1.1-44 T2 scan — GLM-5.1, function-level extracts.

Priority order: highest-risk segments first (codec decode paths, pixel buffer allocation).

Run:
    python scripts/scan_imagemagick_t2.py
    python scripts/scan_imagemagick_t2.py --start 5   # resume from segment index

Results → research/imagemagick/t2_glm5_results.json
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "imagemagick"

# Priority order: decode/decompress paths > allocation > write paths
SCANS = [
    # GIF decoder — classic LZW overflow target
    ("gif_DecodeImage.c",       "coders/gif.c [DecodeImage]"),
    ("gif_ReadGIFImage_A.c",    "coders/gif.c [ReadGIFImage-A]"),
    ("gif_ReadGIFImage_B.c",    "coders/gif.c [ReadGIFImage-B]"),
    # BMP reader — pixel buffer allocation with width/height arithmetic
    ("bmp_ReadBMPImage_A.c",    "coders/bmp.c [ReadBMPImage-A:header-parse]"),
    ("bmp_ReadBMPImage_B.c",    "coders/bmp.c [ReadBMPImage-B:color-table]"),
    ("bmp_ReadBMPImage_C.c",    "coders/bmp.c [ReadBMPImage-C:pixel-decode]"),
    ("bmp_ReadBMPImage_D.c",    "coders/bmp.c [ReadBMPImage-D:rle-decode]"),
    ("bmp_ReadBMPImage_E.c",    "coders/bmp.c [ReadBMPImage-E:row-read]"),
    ("bmp_ReadBMPImage_F.c",    "coders/bmp.c [ReadBMPImage-F:pixel-write]"),
    ("bmp_ReadBMPImage_G.c",    "coders/bmp.c [ReadBMPImage-G:final]"),
    ("bmp_ReadBMPImage_H.c",    "coders/bmp.c [ReadBMPImage-H]"),
    ("bmp_ReadBMPImage_I.c",    "coders/bmp.c [ReadBMPImage-I]"),
    ("bmp_ReadBMPImage_J.c",    "coders/bmp.c [ReadBMPImage-J]"),
    # BMP writer
    ("bmp_WriteBMPImage_A.c",   "coders/bmp.c [WriteBMPImage-A]"),
    ("bmp_WriteBMPImage_B.c",   "coders/bmp.c [WriteBMPImage-B]"),
    ("bmp_WriteBMPImage_C.c",   "coders/bmp.c [WriteBMPImage-C]"),
    ("bmp_WriteBMPImage_D.c",   "coders/bmp.c [WriteBMPImage-D]"),
    ("bmp_WriteBMPImage_E.c",   "coders/bmp.c [WriteBMPImage-E]"),
    ("bmp_WriteBMPImage_F.c",   "coders/bmp.c [WriteBMPImage-F]"),
    ("bmp_WriteBMPImage_G.c",   "coders/bmp.c [WriteBMPImage-G]"),
    # PSD layer reader — complex multi-channel compositing
    ("psd_ReadPSDLayer.c",      "coders/psd.c [ReadPSDLayer]"),
    ("psd_ReadPSDImage_A.c",    "coders/psd.c [ReadPSDImage-A]"),
    ("psd_ReadPSDImage_B.c",    "coders/psd.c [ReadPSDImage-B]"),
    ("psd_ReadPSDImage_C.c",    "coders/psd.c [ReadPSDImage-C]"),
    # TIFF reader — strip/tile reading
    ("tiff_ReadTIFFImage_A.c",  "coders/tiff.c [ReadTIFFImage-A]"),
    ("tiff_ReadTIFFImage_B.c",  "coders/tiff.c [ReadTIFFImage-B]"),
    ("tiff_ReadTIFFImage_C.c",  "coders/tiff.c [ReadTIFFImage-C]"),
    ("tiff_ReadTIFFImage_D.c",  "coders/tiff.c [ReadTIFFImage-D]"),
    ("tiff_ReadTIFFImage_E.c",  "coders/tiff.c [ReadTIFFImage-E]"),
    ("tiff_ReadTIFFImage_F.c",  "coders/tiff.c [ReadTIFFImage-F]"),
    ("tiff_ReadTIFFImage_G.c",  "coders/tiff.c [ReadTIFFImage-G]"),
    ("tiff_ReadTIFFImage_H.c",  "coders/tiff.c [ReadTIFFImage-H]"),
    ("tiff_ReadTIFFImage_I.c",  "coders/tiff.c [ReadTIFFImage-I]"),
    # HEIC / WebP
    ("heic_ReadHEICImage_A.c",  "coders/heic.c [ReadHEICImage-A]"),
    ("heic_ReadHEICImage_B.c",  "coders/heic.c [ReadHEICImage-B]"),
    ("webp_ReadWEBPImage.c",    "coders/webp.c [ReadWEBPImage]"),
    ("webp_WriteWEBPImage_A.c", "coders/webp.c [WriteWEBPImage-A]"),
    ("webp_WriteWEBPImage_B.c", "coders/webp.c [WriteWEBPImage-B]"),
    # PNG
    ("png_ReadPNGImage.c",      "coders/png.c [ReadPNGImage-A]"),
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
    api_key = os.environ.get("GLM_API_KEY", "GLM_KEY_REMOVED")
    agent = GLMAdapter(model_name="glm-5.1", api_key=api_key)

    end = args.end if args.end is not None else len(SCANS)
    scans = SCANS[args.start:end]
    print(f"ImageMagick T2 scan: {len(scans)} segments (idx {args.start}–{end-1}), {args.delay}s delay\n")

    # Load existing results if resuming
    out_path = pathlib.Path("research/imagemagick/t2_glm5_results.json")
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
        # Save after each scan
        with open(out_path, "w") as f:
            json.dump({"model": "glm-5.1", "target": "ImageMagick 7.1.1-44",
                       "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                       "results": results}, f, indent=2, ensure_ascii=False)

    # Summary
    total = sum(len(r["findings"]) for r in results)
    high_crit = sum(1 for r in results for f in r["findings"]
                    if f["severity"] in ("critical", "high") and f["confidence"] >= 0.65)
    print(f"\n{'='*60}")
    print(f"ImageMagick T2 complete: {len(results)} segments, {total} raw findings, {high_crit} high/critical ≥65%")
    print(f"Results → {out_path}")


if __name__ == "__main__":
    asyncio.run(main())
