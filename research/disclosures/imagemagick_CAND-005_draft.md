# ImageMagick CAND-005 Disclosure Draft

**To**: security@imagemagick.org  
**CC**: https://github.com/ImageMagick/ImageMagick/security/advisories (GitHub Private Advisory)  
**Subject**: [Code Quality] HeapOverflowSanityCheck wrong parameters in ReadTIFFImage — tiff.c L2008  
**Date**: 2026-04-26  
**Severity**: Low — Code quality issue; actual exploitation blocked by upstream LibTIFF overflow protection

---

## Summary

In `coders/tiff.c`, the `HeapOverflowSanityCheck()` macro call at approximately line 2008 passes incorrect
parameters that make the check logically ineffective. While no exploitable vulnerability exists in current
64-bit builds due to LibTIFF's upstream `_TIFFCastUInt64ToSSize()` overflow protection, the incorrectly
parameterized sanity check means the intended safety net never fires independently.

This is a **code quality / defensive hygiene** issue rather than an exploitable vulnerability. Fixing it
adds a meaningful independent check against future regressions.

---

## Affected Version

- ImageMagick 7.1.1-44 (confirmed)
- Likely affects other recent versions

---

## Technical Details

### Location

**File**: `coders/tiff.c`  
**Approximate Line**: 2008  
**Context**: `ReadTIFFImage()` tiled TIFF path

### Current Code

```c
// Approximate pattern (from code review):
HeapOverflowSanityCheck(rows, sizeof(*tile_pixels));
```

### Issue

`HeapOverflowSanityCheck(a, b)` is intended to verify that `a * b` does not overflow before a heap
allocation of that size. However, `rows` is not the only dimension of the allocation — the actual
allocated size is `rows * stride` (or `rows * length`), where `stride = TIFFTileRowSize()` can be
a large per-row byte count. Passing only `sizeof(*tile_pixels)` (typically 1–8 bytes) instead of
`stride` makes the check trivially pass for any realistic `rows` value, defeating its purpose.

### Why Not Currently Exploitable

LibTIFF's `TIFFTileRowSize()` / `_TIFFCastUInt64ToSSize()` return 0 on overflow, and the subsequent
allocation guard catches zero-size. However, this is an **upstream dependency** for safety — the
ImageMagick-level check should be independently correct.

### Suggested Fix

```c
// Before allocation of (rows * stride) bytes:
HeapOverflowSanityCheck(rows, stride);   // or rows, length — whichever matches the allocation
```

If `stride` or `length` is not in scope at that point, the check should be restructured so the
correct second operand is available, or the existing check removed and replaced with an explicit
`if (rows > SIZE_MAX / stride) ThrowReaderException(...)`.

---

## Impact Assessment

- **Exploitability**: Not exploitable in 64-bit builds due to LibTIFF upstream protection.
- **Risk if LibTIFF protection changes**: Low-to-medium — incorrect parameter could allow allocating
  fewer bytes than needed, leading to a heap buffer overflow during tile write.
- **CVSS**: N/A (code quality; no confirmed exploit path)

---

## Recommended Action

1. Correct the `HeapOverflowSanityCheck` parameters in `coders/tiff.c` to use the actual allocation
   stride/size rather than `sizeof(*tile_pixels)`.
2. Review other `HeapOverflowSanityCheck` callsites in TIFF/image coders for similar mismatches.
3. Add a comment noting what the check is guarding against to prevent future regressions.

---

## Discovery

Discovered via automated LLM-based code scanning (CyberAI project, GLM-5.1 pipeline) and confirmed
by manual code audit on 2026-04-18. No PoC exploit developed; impact bounded to code quality.

---

## Disclosure Timeline

- **2026-04-18**: Discovered and classified during automated scan
- **2026-04-26**: Disclosure draft prepared
- **Disclosure**: Coordinated — sending via GitHub Private Security Advisory

---

*CyberAI Security Research · fxp007@gmail.com*
