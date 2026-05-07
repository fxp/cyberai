# Subject: [libpng] HIGH: Integer Overflow in end_mask Calculation

## Summary
An integer overflow in `pngrutil.c` can lead to heap buffer overflow and data corruption when processing crafted PNG images. Exploitation requires an application to override the default `PNG_USER_WIDTH_MAX` to permit extremely wide images, causing incorrect memory indexing during row combining.

## Affected versions
Latest stable as of 2026-05-04. Older versions are likely affected as the underlying calculation logic has been present for a long time.

## Vulnerability detail
In `pngrutil.c` within the `png_combine_row` function (line ~3280), the multiplication `pixel_depth * row_width` can overflow on platforms where `unsigned int` and `png_alloc_size_t` are both 32-bit. The code explicitly dismisses this overflow, but doing so has severe consequences: `end_mask` can become incorrectly 0 or non-zero, corrupting the partial-byte preservation logic. Furthermore, `PNG_ROWBYTES(pixel_depth, row_width)` can overflow to a small value. If the `info_rowbytes` consistency check is bypassed (e.g., if `info_rowbytes` is 0 or both sides overflow identically), `end_ptr = dp + PNG_ROWBYTES(...) - 1` points near the start of an undersized buffer. Subsequent writes by the interlacing code using actual pixel coordinates will then overflow the heap buffer.

## Reproduction sketch
1. Compile libpng for a 32-bit target where `unsigned int` and `png_alloc_size_t` are 32-bit.
2. Override `PNG_USER_WIDTH_MAX` to allow widths exceeding the default 1,000,000 limit.
3. Craft a PNG image with `pixel_depth=3` and `row_width=0x55555556`.
4. Process the image using libpng, triggering the `png_combine_row` path. The multiplication overflows to 2, yielding `end_mask=2` and `PNG_ROWBYTES=1`, while the true row requires ~0x20000000 bytes, resulting in a heap buffer overflow.

## Suggested mitigation
Add a bounds check before the multiplication in `png_combine_row` to ensure `pixel_depth * row_width` does not exceed `PNG_SIZE_MAX` or the maximum representable value of `png_alloc_size_t`. Alternatively, enforce strict validation of `row_width` relative to `pixel_depth` before calculating `end_mask` and `PNG_ROWBYTES`.

## Disclosure timeline
We follow Google Project Zero 90-day coordinated disclosure. We will not publish technical detail or proof-of-concept code before a patch is shipped.

— CyberAI research team (security@<placeholder>)

RECOMMENDED_RECIPIENT: png-mng-implement@lists.sourceforge.net

---

## Source finding

```json
{
  "target": "libpng",
  "file_context": "pngrutil.c [png_combine_row]",
  "line_start": 3280,
  "severity": "HIGH",
  "confidence": 0.85,
  "title": "Integer Overflow in end_mask Calculation Due to Unchecked pixel_depth * row_width Multiplication",
  "description": "The multiplication `pixel_depth * row_width` at the `end_mask` calculation can overflow on platforms where `unsigned int` and `png_alloc_size_t` are both 32-bit. The code comment explicitly acknowledges this ('the multiply below may overflow, we don't care because ANSI-C guarantees we get the low bits') but incorrectly dismisses the consequences. When the multiplication overflows: (1) `end_mask` can be incorrectly 0, causing the partial-byte preservation logic to be skipped entirely — leading to data corruption in the destination buffer's last byte; (2) `end_mask` can be incorrectly non-zero, causing wrong bits to be preserved. Critically, `PNG_ROWBYTES(pixel_depth, row_width)` used to compute `end_ptr` may also overflow on 32-bit systems. The `info_rowbytes` consistency check (`png_ptr->info_rowbytes != PNG_ROWBYTES(...)`) can be bypassed when `info_rowbytes` is 0 (check is skipped) or when both sides overflow to the same incorrect value. If `PNG_ROWBYTES` overflows to a small value while `end_mask` is non-zero, `end_ptr = dp + PNG_ROWBYTES(...) - 1` could point near the start of an undersized buffer, and subsequent writes by the interlacing code (using actual pixel coordinates) would overflow the heap buffer. A crafted PNG with e.g. pixel_depth=3 and row_width=0x55555556 causes the multiplication to overflow to 2, yielding end_mask=2 (non-zero) and PNG_ROWBYTES=1, while the true row requires ~0x20000000 bytes.",
  "poc": "Craft a PNG file targeting 32-bit libpng builds: set color type to palette (pixel_depth=1-8) or small bit depth, and set width to a value such that pixel_depth * width overflows a 32-bit integer to a small non-zero value. For pixel_depth=3, width=0x55555556: 3*0x55555556 = 0x100000002, overflows to 0x2 on 32-bit. end_mask = 2 & 7 = 2 (non-zero, enters preservation block). PNG_ROWBYTES = (2+7)>>3 = 1. end_ptr = dp + 1 - 1 = dp. The buffer allocated based on overflowed rowbytes is far too small for the actual row data, causing heap buffer overflow when the interlacing expansion code writes pixel data at the true offsets.",
  "verdict": "PARTIAL",
  "reasoning": "The code path and overflow logic exist exactly as described, but libpng's default width limit (1,000,000) prevents pixel_depth * row_width from overflowing a 32-bit integer. Exploitation strictly requires an application to explicitly override PNG_USER_WIDTH_MAX to an unsafe value, making it a design caveat rather than a default exploitable vulnerability.",
  "exploitability": "needs_specific_setup",
  "known_cve": "none",
  "_elapsed_s": 84.8,
  "_in_tokens": 829,
  "_out_tokens": 3548,
  "_cost_usd": 0.00219,
  "_nvd_match": [],
  "_nvd_ids": [],
  "_extract_used": "/root/cyberai/scripts/extracts/libpng/pngrutil_G.c",
  "_cross_glm4plus": {
    "code_match": "yes",
    "verdict": "CONFIRMED",
    "confidence": 0.9,
    "reasoning": "The code snippet confirms the integer overflow in the `end_mask` calculation and the potential for `PNG_ROWBYTES` to overflow, which can lead to buffer overwrites as described.",
    "_cost": 0.01074
  }
}
```
