#!/usr/bin/env python3
"""FreeType 2.13.3 T1 scan — GLM-5.1, function-level extracts.

Covers: src/truetype/ttgload.c (2740 lines) — glyph loader (simple + composite)
        src/sfnt/ttload.c      (1496 lines) — table/name/OS/2 loader
        src/sfnt/sfwoff2.c     (2380 lines) — WOFF2 reconstruction
        src/sfnt/ttcmap.c      (3902 lines) — cmap4 validation + binary search
        src/cff/cffload.c      (2570 lines) — CFF charset/encoding/font loader
        src/psaux/cffdecode.c  (2411 lines) — CFF charstring interpreter

Historical CVEs targeted:
- CVE-2023-2004: heap buffer overflow in tt_hvadvance_adjust (sfnt/ttsbit.c)
- CVE-2022-27404: heap buffer overflow in sfnt_init_face
- CVE-2022-27405: NULL ptr dereference in FT_New_Face
- CVE-2022-27406: segfault in FT_Done_Size
- CVE-2020-15999: heap buffer overflow in Load_SBit_Png (libpng interaction)
- Scanning for NEW issues in 2.13.3

Run after API quota resets (08:00 BJT daily):
    python scripts/scan_freetype_t1.py
    python scripts/scan_freetype_t1.py --start 20   # resume from index 20
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "freetype"

SCANS = [
    # ── ttgload.c: TT_Load_Simple_Glyph ──
    ("ttgload_simple.c",       "freetype/ttgload.c [TT_Load_Simple_Glyph: contour/flags/coords L342-546]"),
    # ── ttgload.c: TT_Load_Composite_Glyph ──
    ("ttgload_composite_A.c",  "freetype/ttgload.c [TT_Load_Composite_Glyph A: subglyph parse L549-645]"),
    ("ttgload_composite_B.c",  "freetype/ttgload.c [TT_Load_Composite_Glyph B: offset+scale L646-737]"),
    # ── ttgload.c: TT_Process_Composite_Component ──
    ("ttgload_compproc_A.c",   "freetype/ttgload.c [TT_Process_Composite_Component A: matrix L1033-1110]"),
    ("ttgload_compproc_B.c",   "freetype/ttgload.c [TT_Process_Composite_Component B: scale+adjust L1111-1200]"),
    # ── ttgload.c: load_truetype_glyph (main recursive loader) ──
    ("ttgload_load_A.c",       "freetype/ttgload.c [load_truetype_glyph A: setup+metrics L1429-1577]"),
    ("ttgload_load_B.c",       "freetype/ttgload.c [load_truetype_glyph B: variation setup L1578-1700]"),
    ("ttgload_load_C.c",       "freetype/ttgload.c [load_truetype_glyph C: composite loop L1701-1837]"),
    ("ttgload_load_D.c",       "freetype/ttgload.c [load_truetype_glyph D: process+hinting L1838-1966]"),
    # ── ttgload.c: TT_Load_Glyph (public entry point) ──
    ("ttgload_glyph_A.c",      "freetype/ttgload.c [TT_Load_Glyph A: bitmap+strike select L2423-2510]"),
    ("ttgload_glyph_B.c",      "freetype/ttgload.c [TT_Load_Glyph B: SVG+sbix load L2511-2600]"),
    ("ttgload_glyph_C.c",      "freetype/ttgload.c [TT_Load_Glyph C: scalable+outline load L2601-2740]"),
    # ── ttload.c: check_table_dir (checksum+offset validation) ──
    ("ttload_table_dir_A.c",   "freetype/ttload.c [check_table_dir A: tag+checksum L59-170]"),
    ("ttload_table_dir_B.c",   "freetype/ttload.c [check_table_dir B: offset+length bounds L171-342]"),
    # ── ttload.c: tt_face_load_font_dir (table directory) ──
    ("ttload_font_dir_A.c",    "freetype/ttload.c [tt_face_load_font_dir A: header+table count L344-470]"),
    ("ttload_font_dir_B.c",    "freetype/ttload.c [tt_face_load_font_dir B: required tables+glyf L471-638]"),
    # ── ttload.c: tt_face_load_name (name table) ──
    ("ttload_name_A.c",        "freetype/ttload.c [tt_face_load_name A: name record parse L840-945]"),
    ("ttload_name_B.c",        "freetype/ttload.c [tt_face_load_name B: lang tag+storage bounds L946-1046]"),
    # ── ttload.c: tt_face_load_os2 + pclt ──
    ("ttload_os2_A.c",         "freetype/ttload.c [tt_face_load_os2 A: sTypo+xAvgCharWidth L1107-1215]"),
    ("ttload_os2_B.c",         "freetype/ttload.c [tt_face_load_os2 B: panose+coderange+pclt L1216-1373]"),
    # ── sfwoff2.c: reconstruct_font (loca/glyf stream decoding) ──
    ("sfwoff2_reconstruct_A.c","freetype/sfwoff2.c [reconstruct_font A: loca+glyf streams L828-990]"),
    ("sfwoff2_reconstruct_B.c","freetype/sfwoff2.c [reconstruct_font B: composites+fixup L991-1120]"),
    ("sfwoff2_reconstruct_C.c","freetype/sfwoff2.c [reconstruct_font C: hmtx+instructions L1121-1285]"),
    # ── sfwoff2.c: woff2_open_font (CVE-2023-2004 area) ──
    ("sfwoff2_open_A.c",       "freetype/sfwoff2.c [woff2_open_font A: header+length sanity L1858-1980]"),
    ("sfwoff2_open_B.c",       "freetype/sfwoff2.c [woff2_open_font B: table dirs+offset calc L1981-2090]"),
    ("sfwoff2_open_C.c",       "freetype/sfwoff2.c [woff2_open_font C: brotli decompress+streams L2091-2220]"),
    ("sfwoff2_open_D.c",       "freetype/sfwoff2.c [woff2_open_font D: build face+finalize L2221-2380]"),
    # ── ttcmap.c: cmap4_validate (idRangeOffset OOB check) ──
    ("ttcmap_cmap4_validate_A.c","freetype/ttcmap.c [cmap4_validate A: segCount+idRangeOffset L902-1040]"),
    ("ttcmap_cmap4_validate_B.c","freetype/ttcmap.c [cmap4_validate B: glyphId array bounds L1041-1160]"),
    # ── ttcmap.c: tt_cmap4_char_map_binary ──
    ("ttcmap_cmap4_binary_A.c","freetype/ttcmap.c [tt_cmap4_char_map_binary A: binary search L1236-1381]"),
    ("ttcmap_cmap4_binary_B.c","freetype/ttcmap.c [tt_cmap4_char_map_binary B: range index calc L1382-1484]"),
    # ── cffload.c: cff_charset_load (SID + range check) ──
    ("cffload_charset_A.c",    "freetype/cffload.c [cff_charset_load A: format0+1 SID parse L912-1005]"),
    ("cffload_charset_B.c",    "freetype/cffload.c [cff_charset_load B: format2 range bounds L1006-1096]"),
    # ── cffload.c: cff_blend_doBlend (delta blending) ──
    ("cffload_blend_A.c",      "freetype/cffload.c [cff_blend_doBlend A: delta array alloc L1280-1380]"),
    ("cffload_blend_B.c",      "freetype/cffload.c [cff_blend_doBlend B: blend computation L1381-1512]"),
    # ── cffload.c: cff_encoding_load ──
    ("cffload_encoding_A.c",   "freetype/cffload.c [cff_encoding_load A: format0+1 parse L1632-1800]"),
    ("cffload_encoding_B.c",   "freetype/cffload.c [cff_encoding_load B: supplement parse L1801-1900]"),
    # ── cffload.c: cff_font_load (top-level CFF font) ──
    ("cffload_font_A.c",       "freetype/cffload.c [cff_font_load A: header+index parse L2182-2285]"),
    ("cffload_font_B.c",       "freetype/cffload.c [cff_font_load B: subfonts+private dict L2286-2390]"),
    ("cffload_font_C.c",       "freetype/cffload.c [cff_font_load C: charsets+encoding+SFNT L2391-2524]"),
    # ── cffdecode.c: cff_decoder_parse_charstrings (CFF interpreter) ──
    ("cffdecode_setup_A.c",    "freetype/cffdecode.c [cff_decoder_parse_charstrings A: setup+dispatch L501-660]"),
    ("cffdecode_setup_B.c",    "freetype/cffdecode.c [cff_decoder_parse_charstrings B: width+stack bounds L661-840]"),
    ("cffdecode_stem.c",       "freetype/cffdecode.c [cff_decoder_parse_charstrings C: hstem+vstem+move L841-964]"),
    ("cffdecode_hint.c",       "freetype/cffdecode.c [cff_decoder_parse_charstrings D: hintmask+rlineto L965-1133]"),
    ("cffdecode_curve_A.c",    "freetype/cffdecode.c [cff_decoder_parse_charstrings E: rrcurveto+vhcurveto L1134-1310]"),
    ("cffdecode_curve_B.c",    "freetype/cffdecode.c [cff_decoder_parse_charstrings F: rlinecurve+rcurveline L1311-1460]"),
    ("cffdecode_flex.c",       "freetype/cffdecode.c [cff_decoder_parse_charstrings G: hflex1+flex+flex1 L1461-1619]"),
    ("cffdecode_seac.c",       "freetype/cffdecode.c [cff_decoder_parse_charstrings H: seac+endchar L1620-1760]"),
    ("cffdecode_math.c",       "freetype/cffdecode.c [cff_decoder_parse_charstrings I: abs+div+roll+put L1761-1918]"),
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

    out = pathlib.Path("research/freetype/t1_glm5_results.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump({"model": "glm-5.1", "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                   "target": "freetype-2.13.3", "results": results}, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved → {out}")


if __name__ == "__main__":
    asyncio.run(main())
