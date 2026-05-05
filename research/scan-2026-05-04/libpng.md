# libpng — GLM-5.1 Pipeline A scan (2026-05-05)

- **Model**: glm-5.1
- **Segments**: 14 (0 hit GLM API timeout)
- **Total findings**: 30
  - 🔴 **CRITICAL**: 4
  - 🟠 **HIGH**: 7
  - 🟡 **MEDIUM**: 10
  - 🔵 **LOW**: 6
  - ⚪ **INFO**: 3

## Per-segment summary

| Segment | Time (s) | Findings | Status |
|---|---:|---:|---|
| `pngrutil.c [png_handle_IHDR]` | 207.2 | 3 | ✅ ok |
| `pngrutil.c [png_handle_PLTE]` | 340.1 | 2 | ✅ ok |
| `pngrutil.c [png_inflate]` | 272.5 | 3 | ✅ ok |
| `pngrutil.c [png_decompress_chunk]` | 269.6 | 3 | ✅ ok |
| `pngrutil.c [png_handle_iCCP]` | 360.5 | 0 | ✅ ok |
| `pngrutil.c [png_handle_tRNS]` | 113.6 | 2 | ✅ ok |
| `pngrutil.c [png_handle_tEXt]` | 97.8 | 3 | ✅ ok |
| `pngrutil.c [png_handle_zTXt]` | 245.7 | 4 | ✅ ok |
| `pngrutil.c [png_handle_iTXt]` | 360.5 | 0 | ✅ ok |
| `pngrutil.c [png_handle_pCAL]` | 164.7 | 3 | ✅ ok |
| `pngrutil.c [png_handle_sCAL]` | 223.5 | 2 | ✅ ok |
| `pngrutil.c [png_combine_row]` | 284.4 | 2 | ✅ ok |
| `pngrtran.c [png_do_compose]` | 360.5 | 0 | ✅ ok |
| `pngpread.c [png_push_read_chunk]` | 334.9 | 3 | ✅ ok |

## 🔴 CRITICAL (4)

### `pngrutil.c [png_handle_tEXt]` — L2614 — conf=0.85
**Integer Overflow in length+1 Leading to Heap Buffer Overflow**

When length is 0xFFFFFFFF (maximum png_uint_32), the expression `length+1` overflows to 0 in 32-bit unsigned arithmetic. This 0 is then passed to png_read_buffer() as the allocation size. If png_read_buffer returns a non-NULL pointer for a zero-size allocation (which is permitted by the C standard for malloc(0)), the subsequent png_crc_read(png_ptr, buffer, length) will attempt to write 0xFFFFFFFF bytes into the undersized buffer, causing a massive heap buffer overflow. The only existing guard (PNG_MAX_MALLOC_64K) is conditionally compiled and not enabled on most modern systems.

*PoC sketch:* Craft a PNG file with a tEXt chunk whose length field in the chunk header is set to 0xFFFFFFFF. The chunk data can be minimal (a few bytes followed by end-of-file). When libpng processes this chunk: 1) length = 0xFFFFFFFF, 2) length+1 = 0 (overflow), 3) png_read_buffer allocates 0 bytes (may return …

### `pngrutil.c [png_handle_sCAL]` — L2468 — conf=0.75
**Integer overflow in length+1 allocation size computation**

The expression `length+1` is computed in `png_uint_32` (uint32_t) arithmetic before being implicitly widened to `size_t` for the `png_read_buffer` call. When `length` is 0xFFFFFFFF, the addition overflows to 0 in uint32_t arithmetic. This zero-sized allocation request may cause `png_read_buffer` to return a previously allocated (and potentially much smaller) buffer, or a zero-size allocation. Subsequently, `png_crc_read(png_ptr, buffer, length)` attempts to read 0xFFFFFFFF bytes into this undersized buffer, causing a massive heap buffer overflow. Additionally, `buffer[length] = 0` writes to bu…

*PoC sketch:* Craft a PNG file with an sCAL chunk whose declared length field is 0xFFFFFFFF. The chunk header in the PNG file would encode this as the 4-byte big-endian value FF FF FF FF. When libpng processes this chunk on a system where png_read_buffer returns a non-NULL pointer for a zero-size request (or retu…

### `pngrutil.c [png_handle_sCAL]` — L2475 — conf=0.7
**Out-of-bounds write via buffer[length] null termination after integer overflow**

The statement `buffer[length] = 0` writes a null byte at offset `length` from the buffer start. Under normal conditions, this is safe because the buffer was allocated with `length+1` bytes. However, if the integer overflow in `length+1` causes the buffer to be allocated with fewer than `length+1` bytes (e.g., 0 bytes due to overflow), this write is an out-of-bounds access. Even if `png_crc_read` is somehow bounded, this single byte out-of-bounds write could corrupt heap metadata or adjacent data, enabling further exploitation.

*PoC sketch:* Same as the integer overflow PoC: a PNG with sCAL chunk length 0xFFFFFFFF. After the overflow in allocation, buffer[length] writes to buffer[0xFFFFFFFF], a massive out-of-bounds offset that will segfault or corrupt memory.

### `pngrutil.c [png_inflate]` — L557 — conf=0.55
**Potential heap buffer overflow via missing avail_out cap when output is non-NULL**

The visible code shows the `output == NULL` branch capping `avail` to `sizeof local_buffer`, but the `output != NULL` branch is truncated. By structural analogy, if the non-NULL branch also fails to cap `avail` to `avail_out` before assigning `png_ptr->zstream.avail_out = avail`, then zlib is told it has ZLIB_IO_MAX (65535) bytes of output space when only `avail_out` bytes remain in the caller's buffer. zlib will then write up to 65535 bytes past the end of the heap-allocated output buffer. The inconsistency with the input-side bounds check (`if (avail_in < avail) avail = (uInt)avail_in;`) and…

*PoC sketch:* Provide a crafted PNG where png_inflate is called with a small output buffer (e.g., 100 bytes) but the decompressed data exceeds the buffer. If avail is set to ZLIB_IO_MAX without capping to avail_out, zlib writes past the 100-byte buffer boundary, corrupting heap metadata and enabling arbitrary cod…


## 🟠 HIGH (7)

### `pngrutil.c [png_handle_pCAL]` — L2370 — conf=0.95
**Missing return after benign error for unrecognized equation type leads to heap buffer over-read**

When `type >= PNG_EQUATION_LAST`, the code calls `png_chunk_benign_error()` but does NOT return, unlike the invalid parameter count case above it which does return. This allows execution to fall through to the parameter processing loop with an unvalidated `nparams` value (0-255). The subsequent loop iterates `nparams` times scanning for null-terminated parameter strings in the buffer. If `nparams` exceeds the actual number of parameter strings present, the scanner walks past the null terminator placed at `buffer[length]` and reads heap memory beyond the allocated buffer. Each iteration does `b…

*PoC sketch:* Craft a PNG file with a pCAL chunk where: (1) the purpose string is minimal (1 char + null), (2) X0, X1 are any values, (3) type = 4 (>= PNG_EQUATION_LAST, which is typically 4), (4) nparams = 255, (5) units is a minimal string, (6) only 0-2 parameter strings follow. The parser will not return after…

### `pngrutil.c [png_decompress_chunk]` — L645 — conf=0.85
**Integer Overflow in Limit Check Bypass via prefix_size + terminate**

The expression `prefix_size + (terminate != 0)` is computed in `png_uint_32` (32-bit unsigned) arithmetic before being compared against the `png_alloc_size_t limit`. When `prefix_size = 0xFFFFFFFF` and `terminate != 0`, this addition overflows to 0 in 32-bit, causing the check `limit >= 0` to always pass. This bypasses the intended limit enforcement. Subsequently, `buffer_size = prefix_size + new_size + (terminate != 0)` also overflows on 32-bit systems (where `png_alloc_size_t` is 32-bit), wrapping to a small value. This results in an undersized heap allocation via `png_malloc_base`, while th…

*PoC sketch:* Craft a PNG file with a compressed text chunk (e.g., zTXt or iTXt) where the prefix (keyword + separators) spans 0xFFFFFFFF bytes. This requires a chunk with chunklength >= 0xFFFFFFFF. The chunk length field in PNG is 32-bit, so this is format-legal. When processed on a 32-bit system: (1) prefix_siz…

### `pngrutil.c [png_handle_IHDR]` — L940 — conf=0.82
**Integer overflow in pixel_depth calculation before validation**

The expression `(png_byte)(png_ptr->bit_depth * png_ptr->channels)` can silently overflow/truncate when `bit_depth` (read directly from untrusted file input as `buf[8]`, range 0-255) is multiplied by `channels` (up to 4). For example, `bit_depth=128, color_type=RGB_ALPHA (channels=4)` yields `128*4=512`, which truncates to `0` when cast to `png_byte`. This causes `pixel_depth=0` and subsequently `rowbytes=0` via `PNG_ROWBYTES(0, width)`. These corrupted values are stored in `png_ptr` BEFORE `png_set_IHDR` performs validation. While `png_set_IHDR` will reject invalid combinations via `png_error…

*PoC sketch:* Craft a PNG file with IHDR chunk where bit_depth=128 and color_type=6 (RGB_ALPHA). The multiplication 128*4=512 overflows png_byte to 0. pixel_depth=0, rowbytes=0. png_set_IHDR rejects this, but png_ptr->rowbytes is already 0. If the application's longjmp error handler accesses png_ptr->rowbytes (e.…

### `pngrutil.c [png_decompress_chunk]` — L695 — conf=0.8
**Integer Overflow in buffer_size Calculation on 32-bit Systems**

The calculation `buffer_size = prefix_size + new_size + (terminate != 0)` can overflow `png_alloc_size_t` on 32-bit systems when `prefix_size` is large. Even without the limit check bypass (finding 1), if `prefix_size` is sufficiently large (e.g., 0x80000001) and `new_size` is within the (incorrectly computed) limit, the sum can wrap around to a small value. For example, prefix_size=0xFFFFFF00, new_size=0xFF, terminate=1 yields buffer_size=0xFFFFFF00+0xFF+1=0x100000000 which truncates to 0 on 32-bit, causing a zero-size or small allocation followed by massive heap overflow when data is written…

*PoC sketch:* On a 32-bit system, provide a chunk with prefix_size close to 0xFFFFFF00 and compressed data that decompresses to ~0xFF bytes. The limit check computes prefix_size+1 in 32-bit (no overflow at 0xFFFFFF01), limit reduces to ~0xFE, new_size capped to ~0xFE. Then buffer_size = 0xFFFFFF00 + 0xFE + 1 = 0x…

### `pngrutil.c [png_combine_row]` — L3280 — conf=0.75
**Integer Overflow in end_mask Calculation Due to Unchecked pixel_depth * row_width Multiplication**

The multiplication `pixel_depth * row_width` at the `end_mask` calculation can overflow on platforms where `unsigned int` and `png_alloc_size_t` are both 32-bit. The code comment explicitly acknowledges this ('the multiply below may overflow, we don't care because ANSI-C guarantees we get the low bits') but incorrectly dismisses the consequences. When the multiplication overflows: (1) `end_mask` can be incorrectly 0, causing the partial-byte preservation logic to be skipped entirely — leading to data corruption in the destination buffer's last byte; (2) `end_mask` can be incorrectly non-zero, …

*PoC sketch:* Craft a PNG file targeting 32-bit libpng builds: set color type to palette (pixel_depth=1-8) or small bit depth, and set width to a value such that pixel_depth * width overflows a 32-bit integer to a small non-zero value. For pixel_depth=3, width=0x55555556: 3*0x55555556 = 0x100000002, overflows to …

### `pngrutil.c [png_handle_zTXt]` — L2737 — conf=0.72
**Heap Buffer Overflow (Off-by-One) in Null Termination Write**

After decompression, the code writes a null terminator at `buffer[uncompressed_length+(keyword_length+2)] = 0` without independently verifying that this index falls within the allocated bounds of `png_ptr->read_buffer`. The safety of this write depends entirely on `png_decompress_chunk` correctly allocating an extra byte when the `terminate` parameter is 1. If `png_decompress_chunk` allocates only `keyword_length + 2 + uncompressed_length` bytes (failing to account for the null terminator), this write constitutes a 1-byte heap buffer overflow. A 1-byte heap overflow can corrupt adjacent heap m…

*PoC sketch:* Craft a PNG file with a zTXt chunk where: (1) keyword is 1-79 valid chars, (2) compression type is 0 (PNG_COMPRESSION_TYPE_BASE), (3) the zlib-compressed text data decompresses to fill the output buffer exactly to its allocated boundary. The null terminator write at `buffer[uncompressed_length + key…

### `pngpread.c [png_push_read_chunk]` — L247 — conf=0.65
**Missing Duplicate IHDR Validation**

The IHDR chunk handling does not verify whether IHDR has already been processed (i.e., whether PNG_HAVE_IHDR is already set) before invoking png_handle_IHDR. This contrasts with the IDAT handling, which explicitly checks for PNG_HAVE_IHDR. A crafted PNG with two IHDR chunks could cause png_handle_IHDR to be called twice, re-initializing image dimensions, color type, and bit depth after memory has already been allocated based on the first IHDR's parameters. This mismatch between allocated buffer sizes and re-initialized dimensions can lead to heap buffer overflows when subsequent image data is …

*PoC sketch:* Craft a PNG file with chunk sequence: IHDR(13 bytes: 64x64, 8-bit RGB) → IDAT(valid compressed data for 64x64) → IHDR(13 bytes: 4096x4096, 8-bit RGB) → IDAT(data sized for 4096x4096). If the second IHDR re-initializes the state, buffers allocated for 64x64 pixels will be overwritten with data intend…


## 🟡 MEDIUM (10)

### `pngrutil.c [png_handle_tRNS]` — L1829 — conf=0.92
**Uninitialized stack buffer passed to png_set_tRNS in GRAY/RGB paths**

The local variable `readbuf` (png_byte[PNG_MAX_PALETTE_LENGTH]) is declared at the top of the function but is only populated via png_crc_read() in the PALETTE color type branch. In the GRAY and RGB branches, `readbuf` remains uninitialized, yet `png_ptr->num_trans` is set to 1. After the branch, the common code path calls `png_set_tRNS(png_ptr, info_ptr, readbuf, png_ptr->num_trans, &(png_ptr->trans_color))`, passing the uninitialized `readbuf` with `num_trans=1`. The `png_set_tRNS` function will see `num_trans > 0` and `trans != NULL` (stack address), so it allocates a 1-byte buffer and memcp…

*PoC sketch:* Craft a PNG file with color_type=0 (GRAY) or color_type=2 (RGB) containing a valid tRNS chunk (2 bytes for GRAY, 6 bytes for RGB). When libpng processes this file, png_handle_tRNS will set num_trans=1 without writing to readbuf, then pass the uninitialized readbuf to png_set_tRNS. The application ca…

### `pngrutil.c [png_handle_zTXt]` — L2723 — conf=0.92
**Denial of Service via Unbounded Decompression (Zip Bomb)**

`uncompressed_length` is initialized to `PNG_SIZE_MAX` before being passed to `png_decompress_chunk`. This effectively provides no application-level limit on the size of decompressed zTXt data. A malicious PNG can contain a tiny compressed zTXt payload (e.g., a few hundred bytes of zlib data) that decompresses to an enormous size (gigabytes), causing memory exhaustion and process termination. The TODO comment in the code acknowledges that memory limits for text chunks need improvement.

*PoC sketch:* Create a PNG file with a zTXt chunk containing a zip bomb payload: a small zlib-compressed stream that expands to multiple gigabytes. For example, a 500-byte compressed payload that decompresses to 4GB+ of repetitive data. When libpng processes this chunk, it will attempt to allocate a buffer of the…

### `pngrutil.c [png_handle_PLTE]` — L1199 — conf=0.85
**Inconsistent error handling order in png_handle_sBIT allows stream corruption**

In png_handle_sBIT, when the length validation check fails (`length != truelen || length > 4`), `png_chunk_benign_error()` is called BEFORE `png_crc_finish()`. This is inconsistent with all other error paths in the same function (lines 1175-1179 for 'out of place' and lines 1184-1188 for 'duplicate') and with other chunk handlers like png_handle_gAMA (lines 1129-1133), which correctly call `png_crc_finish()` first. If the application configures libpng to treat benign errors as fatal (via `png_set_benign_errors()`), `png_chunk_benign_error()` will invoke `png_error()` which performs a longjmp, …

*PoC sketch:* Craft a PNG file with an sBIT chunk whose declared length does not match the expected truelen for the image's color type (e.g., color_type=2/RGB expects truelen=3, but set sBIT chunk length=5). In an application that calls png_set_benign_errors(png_ptr, PNG_HANDLE_ERROR_ALWAYS) and uses setjmp/longj…

### `pngrutil.c [png_handle_pCAL]` — L2385 — conf=0.85
**No bounds validation on parameter string count vs. remaining buffer size**

Even for valid equation types (where nparams is constrained to 2-4), the code never verifies that the buffer actually contains enough null-terminated parameter strings after the units string. The check `endptr - buf <= 12` only ensures room for the fixed fields (X0, X1, type, nparams, units), not for the parameter strings. If a pCAL chunk declares nparams=2 (LINEAR) but the buffer only contains 0 or 1 parameter strings after units, the scanning loop will advance `buf` past `buffer[length]` (the safety null terminator) and read out-of-bounds heap memory. This is a more limited over-read than th…

*PoC sketch:* Craft a PNG with a pCAL chunk: type=0 (LINEAR), nparams=2, units='U\0', and only 0 or 1 parameter strings after units. The buffer has room for the fixed fields but insufficient parameter strings. The loop scans past buffer[length] looking for the second null-terminated string, reading 1-few bytes of…

### `pngrutil.c [png_inflate]` — L545 — conf=0.82
**Missing avail_out bounds check causes integer underflow in output size tracking**

When `output == NULL`, the code caps `avail` to `sizeof local_buffer` but does NOT cap it to `avail_out` (remaining output space). The input side correctly has `if (avail_in < avail) avail = (uInt)avail_in;` (line ~530), but the output side lacks the equivalent `if (avail_out < avail) avail = (uInt)avail_out;`. This causes `avail_out -= avail` to underflow when `avail_out < sizeof local_buffer`, wrapping `avail_out` (an unsigned `png_alloc_size_t`) to a near-maximum value. The code comment even says 'up to remaining_space' but the implementation never checks `avail_out`. The corrupted `avail_o…

*PoC sketch:* Craft a PNG with an IDAT chunk where png_inflate is called with output=NULL and *output_size_ptr set to a small value (e.g., 1). On the first loop iteration, avail=sizeof(local_buffer) (e.g., 8192) exceeds avail_out=1, so avail_out -= 8192 underflows to ~2^64-8192. The returned *output_size_ptr is c…

### `pngrutil.c [png_handle_IHDR]` — L941 — conf=0.7
**Integer overflow in rowbytes calculation for large width values**

The `PNG_ROWBYTES(pixel_depth, width)` macro computes `(pixel_bits) * ((width) + 7) >> 3`. For valid PNG inputs where pixel_depth=64 (16-bit RGBA) and width is large (up to PNG_UINT_31_MAX = 2^31-1), the intermediate multiplication `64 * (2^31-1 + 7)` ≈ 2^37 overflows a 32-bit `png_uint_32`, producing a small wrapped value for `rowbytes`. While the default `PNG_USER_WIDTH_MAX` (1,000,000) prevents this, applications that call `png_set_user_limits()` with large values (or builds with increased defaults) can trigger this overflow with a valid PNG that passes all checks in `png_check_IHDR`. The r…

*PoC sketch:* 1. Build libpng with PNG_USER_WIDTH_MAX set to 0x7FFFFFFF (or call png_set_user_limits with large values). 2. Craft a PNG with IHDR: width=134217728 (0x08000000), height=1, bit_depth=16, color_type=6 (RGBA). pixel_depth=64. PNG_ROWBYTES(64, 134217728) = 64 * (134217728+7) >> 3 = 64*134217735>>3 = 85…

### `pngrutil.c [png_decompress_chunk]` — L660 — conf=0.7
**Missing Validation of prefix_size Against chunklength**

The function computes `lzsize = chunklength - prefix_size` without validating that `prefix_size <= chunklength`. If a caller passes `prefix_size > chunklength`, this unsigned subtraction underflows, producing an extremely large `lzsize`. This value is then passed to `png_inflate` as the input buffer size, along with the pointer `png_ptr->read_buffer + prefix_size` which already points beyond the allocated read buffer. This results in an out-of-bounds read during decompression. While current callers in libpng validate this relationship, the function itself lacks the defensive check, making it v…

*PoC sketch:* Call png_decompress_chunk with chunklength=10 and prefix_size=20. Then lzsize = 10 - 20 = 0xFFFFFFFA (32-bit underflow). png_inflate attempts to read 0xFFFFFFFA bytes from read_buffer+20, causing a massive out-of-bounds read and likely a crash.

### `pngrutil.c [png_combine_row]` — L3270 — conf=0.65
**Incomplete info_rowbytes Validation Allows Overflow to Propagate**

The consistency check `if (png_ptr->info_rowbytes != 0 && png_ptr->info_rowbytes != PNG_ROWBYTES(pixel_depth, row_width))` is skipped entirely when `info_rowbytes` is 0. This means that if `info_rowbytes` has not been properly initialized (e.g., `png_read_update_info()` was not called, or a code path fails to set it), no validation of the row size calculation occurs. On 32-bit systems where `PNG_ROWBYTES` overflows, this missing check allows the overflowed value to be used unchecked for buffer pointer arithmetic (`end_ptr = dp + PNG_ROWBYTES(pixel_depth, row_width) - 1`), potentially leading t…

*PoC sketch:* In an application that does not call png_read_update_info() before reading rows, info_rowbytes remains 0. Combined with a crafted PNG that triggers integer overflow in PNG_ROWBYTES on a 32-bit build, the consistency check is bypassed and the overflowed row size is used for pointer arithmetic.

### `pngpread.c [png_push_read_chunk]` — L237 — conf=0.45
**Zero-Length IDAT Early Return Skips CRC Consumption**

When a zero-length IDAT chunk appears after a previous IDAT with no intervening non-IDAT chunks, the function returns early (line 240) without consuming the 4-byte CRC. At this point, PNG_HAVE_CHUNK_HEADER remains set and PNG_HAVE_IDAT has NOT been set. The process_mode is changed to PNG_READ_IDAT_MODE, delegating CRC handling to the IDAT mode handler. If that handler does not correctly process the CRC for zero-length IDATs (e.g., skips CRC read when push_length==0), the unconsumed CRC bytes will be misinterpreted as the next chunk's length+tag, causing parser desynchronization and potential b…

*PoC sketch:* Craft PNG: IHDR → IDAT(valid data + CRC) → IDAT(length=0, valid CRC) → IEND. The zero-length second IDAT triggers the early return. If the IDAT mode handler fails to consume the CRC for zero-length chunks, the 4 CRC bytes are parsed as the next chunk header (e.g., a bogus length + tag), desynchroniz…

### `pngpread.c [png_push_read_chunk]` — L256 — conf=0.4
**Missing IEND Chunk Length Validation**

Unlike IHDR which explicitly validates push_length == 13, the IEND handling performs no validation that push_length == 0 as required by the PNG specification. A malformed IEND chunk with a non-zero length passes through to png_handle_IEND with an unexpected length. While png_check_chunk_length (called earlier at line 210) may provide some validation, the absence of an explicit zero-length check in the dispatch code means a non-zero-length IEND could reach the handler. If the handler trusts the length or uses it in calculations, this could cause out-of-bounds reads or unexpected memory operatio…

*PoC sketch:* Craft PNG: IHDR → IDAT(valid) → IEND(length=0x7FFFFFF0 instead of 0). The oversized length causes PNG_PUSH_SAVE_BUFFER_IF_FULL to attempt buffering an enormous amount of data. If the length+4 calculation overflows in signed arithmetic within the macro (0x7FFFFFF0+4 = 0x7FFFFFF4 unsigned, but negativ…


## 🔵 LOW (6)

### `pngrutil.c [png_handle_zTXt]` — L2703 — conf=0.95
**Missing Keyword Content Validation Enables Injection**

The keyword in the zTXt chunk is only validated for length (1-79 characters) but not for character content. The PNG specification requires keywords to contain only printable Latin-1 characters (32-126, 161-255) and spaces, with no leading/trailing/consecutive spaces. The code's own TODO comment acknowledges this gap: 'TODO: also check that the keyword contents match the spec!'. Malicious keywords containing control characters, null bytes (beyond the terminator), or other invalid characters could cause unexpected behavior in downstream applications that trust libpng's validation — including log…

*PoC sketch:* Create a PNG with a zTXt chunk where the keyword contains control characters or non-Latin-1 bytes. For example, keyword bytes: 0x01 0x02 0x03 (control chars) followed by 0x00 (null terminator). This passes the length check (keyword_length=3, within 1-79) but violates the PNG spec. Downstream applica…

### `pngrutil.c [png_inflate]` — L527 — conf=0.9
**Inconsistent bounds checking between input and output chunking**

The input chunking code at lines ~527-535 correctly prevents underflow with `if (avail_in < avail) avail = (uInt)avail_in;` before `avail_in -= avail;`. The output chunking code lacks this symmetric guard. This asymmetry indicates a defensive programming lapse: the input path was hardened but the output path was not. While the direct impact requires specific caller conditions (small *output_size_ptr), the pattern violation suggests other output-path bounds checks may also be missing throughout the function.

*PoC sketch:* Static analysis: compare lines 529-534 (input: check then subtract) with lines 545-556 (output: no check before subtract). The structural mismatch is the root cause of finding #1.

### `pngrutil.c [png_handle_IHDR]` — L866 — conf=0.65
**Premature PNG_HAVE_IHDR mode flag set before validation**

The line `png_ptr->mode |= PNG_HAVE_IHDR` is executed immediately after the length check but BEFORE the IHDR field values are validated by `png_set_IHDR`. If `png_set_IHDR` rejects the IHDR (e.g., invalid bit_depth/color_type combination) and calls `png_error`, the longjmp to the application's error handler leaves `PNG_HAVE_IHDR` set in `png_ptr->mode` even though the IHDR was invalid. Applications that catch errors via setjmp/longjmp and attempt to continue processing will see the flag as set, potentially allowing processing of subsequent chunks (PLTE, IDAT, etc.) against an inconsistent/corr…

*PoC sketch:* Craft a PNG with an invalid IHDR (e.g., bit_depth=3, color_type=2). The PNG_HAVE_IHDR flag is set at line 866. png_set_IHDR calls png_error which longjmps. If the application error handler does not destroy the png_struct and instead attempts to re-use it, subsequent chunk processing will see PNG_HAV…

### `pngrutil.c [png_handle_PLTE]` — L1191 — conf=0.6
**Potential stack buffer overflow if png_ptr->channels exceeds 4 in sBIT handler**

In png_handle_sBIT, `truelen` is set to `png_ptr->channels` for non-palette color types, and data is read into `png_byte buf[4]` via `png_crc_read(png_ptr, buf, truelen)`. While the check `length != truelen || length > 4` indirectly prevents overflow (since length must equal truelen AND be <=4), the code does not explicitly validate that `truelen <= sizeof(buf)`. If `png_ptr->channels` were ever corrupted or set to a value >4 through a prior bug or memory corruption, the indirect length check would still catch it — but the defense is fragile and relies on the attacker-controlled `length` field…

*PoC sketch:* Not directly exploitable with valid PNG color types. Would require prior memory corruption of png_ptr->channels to a value >4, combined with a crafted sBIT chunk length matching that value — at which point the length>4 check would still block the overflow. Demonstrates fragile defense-in-depth rathe…

### `pngrutil.c [png_handle_pCAL]` — L2328 — conf=0.5
**Integer overflow in length+1 buffer allocation size computation**

The expression `length+1` is computed in `png_uint_32` (32-bit unsigned) arithmetic. If `length` is 0xFFFFFFFF, `length+1` wraps to 0. This zero is then passed to `png_read_buffer()` as the allocation size. If `png_read_buffer` returns a non-NULL pointer for a zero-size allocation (implementation-defined behavior of malloc(0)), the subsequent `png_crc_read(png_ptr, buffer, length)` would attempt to write 0xFFFFFFFF bytes into the undersized buffer — a catastrophic heap buffer overflow. In practice, libpng's `png_read_buffer` includes internal size validation that likely returns NULL for size 0…

*PoC sketch:* Craft a PNG with a pCAL chunk whose length field is 0xFFFFFFFF. If the PNG reader does not reject this chunk size before calling png_handle_pCAL, the length+1 computation overflows to 0, potentially leading to a zero-size allocation followed by a massive overwrite.

### `pngrutil.c [png_handle_zTXt]` — L2737 — conf=0.35
**Potential Integer Overflow in Buffer Index Calculation**

The expression `uncompressed_length + (keyword_length + 2)` used as a buffer index could theoretically overflow if `uncompressed_length` (type `png_alloc_size_t`, typically `size_t`) is close to `SIZE_MAX`. While practically difficult to achieve due to memory constraints, if `png_decompress_chunk` returns a very large `uncompressed_length` without proper validation, the wrapped-around index would cause the null terminator to be written to an unintended memory location. This is a defense-in-depth concern — the code should validate that `uncompressed_length + keyword_length + 2` does not overflo…

*PoC sketch:* Theoretical: On a 32-bit system where SIZE_MAX = 0xFFFFFFFF, if png_decompress_chunk were to set uncompressed_length = 0xFFFFFF7D (4294967165) and keyword_length = 79, then uncompressed_length + keyword_length + 2 = 0xFFFFFF7D + 81 = 0xFFFFFFCE (no overflow in this case, but values closer to SIZE_MA…


## ⚪ INFO (3)

### `pngrutil.c [png_handle_tEXt]` — L2622 — conf=0.75
**CRC Check Occurs After Buffer Overflow Window**

The CRC validation via png_crc_finish() is performed AFTER png_crc_read() has already written data into the buffer. If the integer overflow in finding #1 is triggered, the heap corruption occurs during png_crc_read() before the CRC is ever validated. This means the CRC integrity check cannot prevent the buffer overflow — the damage is already done. This is a defense-in-depth failure where the integrity check ordering is incorrect for security purposes.

*PoC sketch:* Same as finding #1 — the overflow in png_crc_read happens before png_crc_finish can reject the malformed chunk.

### `pngrutil.c [png_handle_tEXt]` — L2626 — conf=0.7
**Missing Key Length Validation**

The PNG specification requires tEXt chunk keywords to be 1-79 bytes long and must contain at least one non-null byte before the null separator. The code does not validate: (a) that the key is non-empty (first byte could be null), (b) that the key length is <= 79 bytes. An empty key (buffer starting with 0x00) causes text to be set to key+1, and the empty key string is passed to png_set_text_2, which may cause undefined behavior or crashes in downstream consumers that assume a valid non-empty key. A key longer than 79 bytes violates the spec and may overflow fixed-size buffers in applications p…

*PoC sketch:* Craft a PNG with a tEXt chunk where: (a) the first byte is 0x00 (empty key), causing key='' and text to point to key+1; or (b) the key is 200+ bytes with no null separator until the end, creating an oversized key that violates the PNG spec 79-byte limit.

### `pngrutil.c [png_handle_tRNS]` — L1949 — conf=0.65
**Potential use-after-free via shared tRNS buffer pointer between png_struct and png_info**

The TODO comment in the source explicitly acknowledges this design flaw: 'the png_struct ends up with a pointer to the tRNS buffer owned by the png_info.' After png_set_tRNS allocates and stores the transparency buffer in info_ptr->trans_alpha, png_ptr may also hold a reference to the same buffer. If info_ptr is freed (e.g., via png_destroy_info) while png_ptr continues processing, png_ptr's dangling pointer to the freed trans_alpha buffer becomes a use-after-free. Subsequent reads or writes through png_ptr's reference could corrupt heap metadata or leak freed heap data.

*PoC sketch:* 1. Parse a PNG with a PALETTE color type and a tRNS chunk. 2. Call png_destroy_info() to free info_ptr and its trans_alpha buffer. 3. Continue using png_ptr (e.g., reading more chunks or rows), which may dereference the dangling pointer to the freed trans_alpha buffer. The exact exploitability depen…

