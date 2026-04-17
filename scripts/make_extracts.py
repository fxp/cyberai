#!/usr/bin/env python3
"""
make_extracts.py — Extract function-level code segments from C source files.

For each target function:
  - Locate the function definition using a regex on the start line
  - Use brace-counting to extract the complete function body
  - If the body exceeds CHUNK_LIMIT chars, split at natural breakpoints (loop/if lines)
  - Write each chunk as a separate .c file under extracts/imagemagick/ or extracts/libtiff/
  - Generate scan_list.json with metadata for all created files
"""

import re
import os
import json
import textwrap
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

EXTRACTS_ROOT = Path("/Users/xiaopingfeng/Projects/cyberai/scripts/extracts")
SCAN_LIST_OUT = Path("/Users/xiaopingfeng/Projects/cyberai/scripts/scan_list.json")
CHUNK_LIMIT = 4500   # chars; split above this threshold

IMAGEMAGICK_DIR = Path("/tmp/ImageMagick-7.1.1-44/coders")
LIBTIFF_DIR     = Path("/tmp/tiff-4.7.0/libtiff")

# Each entry: (source_file, function_name, output_prefix, subdir, label)
# output_prefix is the base filename (no .c, no part suffix)
TARGETS = [
    # ---- ImageMagick -------------------------------------------------------
    (IMAGEMAGICK_DIR / "bmp.c",  "ReadBMPImage",  "bmp_ReadBMPImage",  "imagemagick",
     "ImageMagick bmp.c – ReadBMPImage"),
    (IMAGEMAGICK_DIR / "bmp.c",  "WriteBMPImage", "bmp_WriteBMPImage", "imagemagick",
     "ImageMagick bmp.c – WriteBMPImage"),

    (IMAGEMAGICK_DIR / "webp.c", "ReadWEBPImage",  "webp_ReadWEBPImage",  "imagemagick",
     "ImageMagick webp.c – ReadWEBPImage"),
    (IMAGEMAGICK_DIR / "webp.c", "WriteWEBPImage", "webp_WriteWEBPImage", "imagemagick",
     "ImageMagick webp.c – WriteWEBPImage"),

    (IMAGEMAGICK_DIR / "heic.c", "ReadHEICImage",  "heic_ReadHEICImage",  "imagemagick",
     "ImageMagick heic.c – ReadHEICImage"),

    (IMAGEMAGICK_DIR / "gif.c",  "ReadGIFImage",   "gif_ReadGIFImage",    "imagemagick",
     "ImageMagick gif.c – ReadGIFImage"),
    (IMAGEMAGICK_DIR / "gif.c",  "DecodeImage",    "gif_DecodeImage",     "imagemagick",
     "ImageMagick gif.c – DecodeImage"),

    # png.c ReadPNGImage — only capture first 200 lines of body (chunked)
    (IMAGEMAGICK_DIR / "png.c",  "ReadPNGImage",   "png_ReadPNGImage",    "imagemagick",
     "ImageMagick png.c – ReadPNGImage"),

    (IMAGEMAGICK_DIR / "psd.c",  "ReadPSDImage",   "psd_ReadPSDImage",    "imagemagick",
     "ImageMagick psd.c – ReadPSDImage"),
    (IMAGEMAGICK_DIR / "psd.c",  "ReadPSDLayer",   "psd_ReadPSDLayer",    "imagemagick",
     "ImageMagick psd.c – ReadPSDLayer"),

    (IMAGEMAGICK_DIR / "tiff.c", "ReadTIFFImage",  "tiff_ReadTIFFImage",  "imagemagick",
     "ImageMagick tiff.c – ReadTIFFImage"),

    # ---- LibTIFF -----------------------------------------------------------
    (LIBTIFF_DIR / "tif_pixarlog.c", "PixarLogDecode", "tif_pixarlog_decode", "libtiff",
     "LibTIFF tif_pixarlog.c – PixarLogDecode"),
    (LIBTIFF_DIR / "tif_pixarlog.c", "PixarLogEncode", "tif_pixarlog_encode", "libtiff",
     "LibTIFF tif_pixarlog.c – PixarLogEncode"),

    (LIBTIFF_DIR / "tif_getimage.c", "TIFFReadRGBAImage",  "tif_getimage_TIFFReadRGBAImage",  "libtiff",
     "LibTIFF tif_getimage.c – TIFFReadRGBAImage"),
    (LIBTIFF_DIR / "tif_getimage.c", "gtTileSeparate",     "tif_getimage_gtTileSeparate",      "libtiff",
     "LibTIFF tif_getimage.c – gtTileSeparate"),
    (LIBTIFF_DIR / "tif_getimage.c", "gtStripSeparate",    "tif_getimage_gtStripSeparate",     "libtiff",
     "LibTIFF tif_getimage.c – gtStripSeparate"),

    (LIBTIFF_DIR / "tif_luv.c", "LogLuvDecode32",    "tif_luv_LogLuvDecode32",    "libtiff",
     "LibTIFF tif_luv.c – LogLuvDecode32"),
    (LIBTIFF_DIR / "tif_luv.c", "LogLuvDecode24",    "tif_luv_LogLuvDecode24",    "libtiff",
     "LibTIFF tif_luv.c – LogLuvDecode24"),
    (LIBTIFF_DIR / "tif_luv.c", "LogLuvEncode32",    "tif_luv_LogLuvEncode32",    "libtiff",
     "LibTIFF tif_luv.c – LogLuvEncode32"),
    (LIBTIFF_DIR / "tif_luv.c", "LogLuvEncode24",    "tif_luv_LogLuvEncode24",    "libtiff",
     "LibTIFF tif_luv.c – LogLuvEncode24"),

    (LIBTIFF_DIR / "tif_dirread.c", "TIFFReadDirectory",  "tif_dirread_TIFFReadDirectory",  "libtiff",
     "LibTIFF tif_dirread.c – TIFFReadDirectory"),
    (LIBTIFF_DIR / "tif_dirread.c", "TIFFFetchNormalTag", "tif_dirread_TIFFFetchNormalTag", "libtiff",
     "LibTIFF tif_dirread.c – TIFFFetchNormalTag"),

    (LIBTIFF_DIR / "tif_read.c", "TIFFReadScanline",       "tif_read_TIFFReadScanline",       "libtiff",
     "LibTIFF tif_read.c – TIFFReadScanline"),
    (LIBTIFF_DIR / "tif_read.c", "TIFFReadEncodedStrip",   "tif_read_TIFFReadEncodedStrip",   "libtiff",
     "LibTIFF tif_read.c – TIFFReadEncodedStrip"),

    (LIBTIFF_DIR / "tif_zip.c", "ZIPDecode", "tif_zip_ZIPDecode", "libtiff",
     "LibTIFF tif_zip.c – ZIPDecode"),
    (LIBTIFF_DIR / "tif_zip.c", "ZIPEncode", "tif_zip_ZIPEncode", "libtiff",
     "LibTIFF tif_zip.c – ZIPEncode"),
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

# Regex to detect a function definition line.
# Matches lines where the token FunctionName appears as the identifier
# being defined (not called), i.e. followed by optional whitespace + '('
# and the line starts with a return type (possibly with static/ModuleExport).
FUNC_DEF_RE_TMPL = (
    r"^(?:(?:static|extern|ModuleExport|TIFF_DLL_EXPORT)\s+)*"          # optional qualifiers
    r"(?:const\s+)?"                                                      # optional const
    r"(?:unsigned\s+)?"                                                   # optional unsigned
    r"(?:ssize_t|int|void|size_t|tmsize_t|tdir_t|uint\w+"               # return types
    r"|MagickBooleanType|MagickExport|Image\s*\*?|toff_t"
    r"|[A-Z][a-zA-Z0-9_]*(?:\s*\*)?)"                                    # or any CamelCase type
    r"[\s\*]+"
    r"{func}\s*\("                                                        # function name + (
)


def make_func_regex(func_name: str) -> re.Pattern:
    # Escape the function name (should be plain alphanumeric)
    escaped = re.escape(func_name)
    pattern = FUNC_DEF_RE_TMPL.format(func=escaped)
    return re.compile(pattern)


def _has_open_brace_nearby(lines: list[str], start_idx: int, lookahead: int = 20) -> bool:
    """
    Return True if a '{' appears within `lookahead` lines starting at start_idx,
    before any ';' that would indicate a mere forward declaration.
    """
    for j in range(start_idx, min(start_idx + lookahead, len(lines))):
        stripped = lines[j].strip()
        if '{' in stripped:
            return True
        # A semicolon *outside* of a parameter list terminates a declaration
        # (heuristic: if the line ends with ';' and depth is 0, it's a decl)
        if stripped.endswith(';'):
            return False
    return False


def find_function_in_lines(lines: list[str], func_name: str) -> int:
    """
    Return the 0-based index of the line that starts the function definition.
    Returns -1 if not found.

    We look for a line matching the function-definition pattern.  Because some
    declarations span multiple lines (return type on one line, name on the next)
    we also handle the simpler case where the function name appears alone at the
    start of a line followed by '('.

    Forward declarations (ending in ';') are skipped — only the definition
    (which leads to a '{') is returned.
    """
    pattern = make_func_regex(func_name)
    # Also a fallback: line starts with the function name (for multi-line sigs)
    fallback = re.compile(r"^" + re.escape(func_name) + r"\s*\(")

    for i, line in enumerate(lines):
        matched_idx = None
        if pattern.match(line):
            matched_idx = i
        elif fallback.match(line):
            # Verify preceding line looks like a return-type line
            if i > 0 and re.search(r"\b(?:int|void|size_t|ssize_t|tmsize_t|static"
                                    r"|Image|MagickBooleanType|tdir_t|uint\w+)\b",
                                    lines[i - 1]):
                matched_idx = i - 1

        if matched_idx is not None:
            # Only accept if this is a definition (not a forward declaration)
            if _has_open_brace_nearby(lines, matched_idx):
                return matched_idx
            # else: it's a forward decl — keep scanning for the real definition

    return -1


def extract_function_body(lines: list[str], start_idx: int) -> tuple[str, int, int]:
    """
    Starting at start_idx, scan forward until we find the opening '{' and then
    count braces until balanced.  Returns (body_text, start_lineno, end_lineno)
    where line numbers are 1-based.
    """
    depth = 0
    body_lines = []
    found_open = False
    end_idx = start_idx

    for i in range(start_idx, len(lines)):
        line = lines[i]
        body_lines.append(line)

        for ch in line:
            if ch == '{':
                depth += 1
                found_open = True
            elif ch == '}':
                depth -= 1

        if found_open and depth == 0:
            end_idx = i
            break

    return "".join(body_lines), start_idx + 1, end_idx + 1  # 1-based


def find_split_point(body_lines: list[str], start: int, max_chars: int) -> int:
    """
    Find a good split point within body_lines[start:] such that the chars
    up to that point do not exceed max_chars.  Prefer splitting just before
    a line that starts an inner loop or major if-block (indented for/while/if).
    Returns the index (into body_lines) just before the chosen split line.
    """
    accumulated = 0
    last_good_split = start
    split_keywords = re.compile(r"^\s{4,}(?:for|while|if|switch|else)\b")

    for i in range(start, len(body_lines)):
        accumulated += len(body_lines[i])
        if accumulated > max_chars:
            # Try to find a natural breakpoint by scanning back a little
            for j in range(i, max(start, i - 30), -1):
                if split_keywords.match(body_lines[j]):
                    return j  # split before this line
            # No natural breakpoint found; split at current position
            return i
        if split_keywords.match(body_lines[i]):
            last_good_split = i
    return len(body_lines)  # no split needed


def make_header(src_file: str, func_name: str, start_line: int, end_line: int,
                part: str = "") -> str:
    part_str = f" (part {part})" if part else ""
    return (
        f"/* ===== EXTRACT: {src_file} ===== */\n"
        f"/* Function: {func_name}{part_str} */\n"
        f"/* Lines: {start_line}–{end_line} */\n"
        f"\n"
    )


def chunk_and_write(body: str, src_file: str, func_name: str,
                    body_start_line: int, body_end_line: int,
                    out_dir: Path, prefix: str) -> list[dict]:
    """
    Split body into CHUNK_LIMIT-sized chunks and write to out_dir.
    Returns list of metadata dicts for scan_list.json.
    """
    out_dir.mkdir(parents=True, exist_ok=True)
    parts = []

    if len(body) <= CHUNK_LIMIT:
        # Single file, no suffix
        fname = f"{prefix}.c"
        header = make_header(src_file, func_name, body_start_line, body_end_line)
        content = header + body
        (out_dir / fname).write_text(content, encoding="utf-8")
        parts.append(fname)
    else:
        body_lines = body.splitlines(keepends=True)
        part_labels = "ABCDEFGHIJ"
        chunk_start = 0
        part_idx = 0

        while chunk_start < len(body_lines):
            # Compute cumulative chars from chunk_start
            end = find_split_point(body_lines, chunk_start, CHUNK_LIMIT)
            if end <= chunk_start:
                end = chunk_start + 1  # safety: advance at least one line

            chunk_lines = body_lines[chunk_start:end]
            chunk_text = "".join(chunk_lines)

            # Compute approximate line numbers within the source file
            lines_before = sum(1 for _ in body_lines[:chunk_start])
            chunk_start_line = body_start_line + lines_before
            chunk_end_line = chunk_start_line + len(chunk_lines) - 1

            label = part_labels[part_idx] if part_idx < len(part_labels) else str(part_idx + 1)
            fname = f"{prefix}_{label}.c"
            header = make_header(src_file, func_name,
                                 chunk_start_line, chunk_end_line, label)
            content = header + chunk_text
            (out_dir / fname).write_text(content, encoding="utf-8")
            parts.append(fname)

            chunk_start = end
            part_idx += 1

            if part_idx >= len(part_labels):
                # Safety: dump the rest into one final chunk
                remaining = "".join(body_lines[chunk_start:])
                if remaining.strip():
                    fname = f"{prefix}_Z.c"
                    (out_dir / fname).write_text(
                        make_header(src_file, func_name,
                                    body_start_line + chunk_start,
                                    body_end_line, "Z") + remaining,
                        encoding="utf-8"
                    )
                    parts.append(fname)
                break

    return parts


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    scan_list: dict[str, list[dict]] = {"imagemagick": [], "libtiff": []}
    total_files = 0

    for (src_path, func_name, prefix, subdir, label) in TARGETS:
        if not src_path.exists():
            print(f"  [SKIP] {src_path} not found")
            continue

        lines = src_path.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)
        start_idx = find_function_in_lines(lines, func_name)

        if start_idx == -1:
            print(f"  [WARN] {func_name} not found in {src_path.name}")
            continue

        body, body_start, body_end = extract_function_body(lines, start_idx)

        if not body.strip():
            print(f"  [WARN] empty body for {func_name} in {src_path.name}")
            continue

        out_dir = EXTRACTS_ROOT / subdir
        created = chunk_and_write(
            body, src_path.name, func_name,
            body_start, body_end,
            out_dir, prefix
        )

        for fname in created:
            rel_path = f"extracts/{subdir}/{fname}"
            abs_path = out_dir / fname
            size = abs_path.stat().st_size
            entry = {
                "file": rel_path,
                "label": label + (f" [{fname.split('_')[-1].replace('.c','')}]"
                                  if len(created) > 1 else ""),
                "target": subdir,
                "size_bytes": size,
                "source_file": str(src_path),
                "function": func_name,
                "source_lines": f"{body_start}-{body_end}",
            }
            scan_list[subdir].append(entry)
            print(f"  [OK]  {rel_path}  ({size:,} bytes, lines {body_start}-{body_end})")
            total_files += 1

    # Write scan_list.json
    SCAN_LIST_OUT.parent.mkdir(parents=True, exist_ok=True)
    with open(SCAN_LIST_OUT, "w", encoding="utf-8") as f:
        json.dump(scan_list, f, indent=2)

    print(f"\nDone. {total_files} extract file(s) written.")
    print(f"Scan list: {SCAN_LIST_OUT}")


if __name__ == "__main__":
    main()
