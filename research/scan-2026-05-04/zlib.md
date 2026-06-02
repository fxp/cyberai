# zlib — GLM-5.1 Pipeline A scan (2026-05-04)

- **Model**: glm-5.1
- **Segments**: 25 (0 hit GLM API timeout)
- **Total findings**: 39
  - 🔴 **CRITICAL**: 1
  - 🟠 **HIGH**: 11
  - 🟡 **MEDIUM**: 12
  - 🔵 **LOW**: 12
  - ⚪ **INFO**: 3

## Per-segment summary

| Segment | Time (s) | Findings | Status |
|---|---:|---:|---|
| `zlib/inflate.c [inflateResetKeep+inflateReset: state check]` | 9.5 | 0 | ✅ ok |
| `zlib/inflate.c [inflateReset2+inflateInit2_: window+state alloc]` | 360.5 | 0 | ✅ ok |
| `zlib/inflate.c [inflateInit_+inflateGetHeader+setDictionary]` | 172.9 | 3 | ✅ ok |
| `zlib/inflate.c [inflate() HEAD+ID+CM+FLAGS+TIME: gzip header parse]` | 360.5 | 0 | ✅ ok |
| `zlib/inflate.c [inflate() EXLEN+EXTRA: gzip extra field (CVE-2022-37434)]` | 360.5 | 0 | ✅ ok |
| `zlib/inflate.c [inflate() NAME+COMMENT+HCRC: gzip name/comment]` | 360.4 | 0 | ✅ ok |
| `zlib/inflate.c [inflate() TYPE+STORED+LEN: block type dispatch]` | 147.2 | 3 | ✅ ok |
| `zlib/inflate.c [inflate() CODELENS+LENS+DIST: Huffman decode]` | 360.5 | 0 | ✅ ok |
| `zlib/inflate.c [inflate() LEN_+MATCH+LIT: back-reference copy]` | 360.5 | 0 | ✅ ok |
| `zlib/inflate.c [inflate() CHECK+LENGTH+DONE: checksum+end states]` | 207.3 | 4 | ✅ ok |
| `zlib/inflate.c [inflateGetDictionary+setDictionary+Copy+Mark]` | 360.4 | 0 | ✅ ok |
| `zlib/inflate.c [inflateUndermine+inflateValidate+CodesUsed]` | 113.4 | 3 | ✅ ok |
| `zlib/inffast.c [inflate_fast A: literal/length decode entry]` | 87.1 | 4 | ✅ ok |
| `zlib/inffast.c [inflate_fast B: back-reference copy loop A]` | 360.5 | 0 | ✅ ok |
| `zlib/inffast.c [inflate_fast C: copy + distance check]` | 125.4 | 3 | ✅ ok |
| `zlib/inffast.c [inflate_fast D: output write + end check]` | 360.5 | 0 | ✅ ok |
| `zlib/inftrees.c [inflate_table A: code length counting]` | 31.2 | 0 | ✅ ok |
| `zlib/inftrees.c [inflate_table B: canonical codes + index]` | 95.4 | 3 | ✅ ok |
| `zlib/inftrees.c [inflate_table C: table fill + bit-length dist]` | 196.0 | 3 | ✅ ok |
| `zlib/inftrees.c [inflate_table D: incomplete code + oversubscribed check]` | 285.8 | 2 | ✅ ok |
| `zlib/gzread.c [gz_load+gz_avail: input buffer fill]` | 360.5 | 0 | ✅ ok |
| `zlib/gzread.c [gz_look+gz_decomp: gzip header detect + decompress]` | 158.9 | 4 | ✅ ok |
| `zlib/gzread.c [gz_fetch+gz_skip: fetch buffer + newline scan]` | 360.5 | 0 | ✅ ok |
| `zlib/gzread.c [gzread+gzfread: public read API]` | 126.8 | 4 | ✅ ok |
| `zlib/gzread.c [gzgetc+gzungetc+gzgets+gzrewind: string read API]` | 243.2 | 3 | ✅ ok |

## 🔴 CRITICAL (1)

### `zlib/inftrees.c [inflate_table D: incomplete code + oversubscribed check]` — L14 — conf=0.95
**Heap Buffer Overflow due to Missing Oversubscribed Tree Check**

The code fails to properly reject oversubscribed Huffman trees. In the sub-table creation loop (lines 48-53), if `left` becomes negative (indicating an oversubscribed tree), the loop breaks but does not return an error. This leaves `curr` at a value that is too small to accommodate the current code length `len`. When the code subsequently processes symbols of length `len > curr + drop`, the index calculation `huff >> drop` exceeds the allocated sub-table size `1 << curr`. This results in an out-of-bounds write on line 14 (`next[(huff >> drop) + fill] = here;`), leading to a heap buffer overflo…

*PoC sketch:* Craft a DEFLATE stream with a dynamic Huffman tree where the number of codes at a certain length exceeds the available code space (oversubscribed). For example, set `count[10]` such that `left` becomes negative in the sub-table creation loop. Then include codes of length 11 or greater. When these lo…


## 🟠 HIGH (11)

### `zlib/inflate.c [inflateInit_+inflateGetHeader+setDictionary]` — L10 — conf=0.92
**Incorrect wrap field calculation in inflateReset2**

The wrap field is calculated as `(windowBits >> 4) + 5` instead of the correct `(windowBits >> 4) + 1` (as in the genuine zlib 1.3.1 source). This produces incorrect wrap values: 5 for zlib streams (should be 1), 6 for gzip streams (should be 2), and 7 for auto-detect (should be 3). The wrap field controls header parsing and CRC/Adler32 integrity verification throughout inflate.c. With wrap=6 (binary 110) for gzip streams, `wrap & 1` is 0, disabling CRC integrity checks and allowing corrupted or maliciously crafted compressed data to be accepted without verification. With wrap=5 (binary 101) f…

*PoC sketch:* 1. Initialize inflate with windowBits=31 (gzip mode): wrap becomes 6 instead of 2. 2. Provide a gzip-compressed payload with a corrupted CRC32 trailer. 3. Because wrap=6 (wrap & 1 == 0), the CRC check is skipped, and the corrupted data is accepted.  Alternatively, with windowBits=15 (zlib mode), wra…

### `zlib/inflate.c [inflate() TYPE+STORED+LEN: block type dispatch]` — L893 — conf=0.92
**PKZIP_BUG_WORKAROUND disables critical bounds check on nlen/ndist**

When PKZIP_BUG_WORKAROUND is defined at compile time, the bounds check 'state->nlen > 286 || state->ndist > 30' is entirely skipped. This allows nlen up to 288 and ndist up to 32 (nlen+ndist up to 320), which can fill the entire state->lens[320] array with no margin. More critically, downstream Huffman table construction (inflate_table for LENGTHS/DISTANCES) is sized for max 286/30 symbols respectively, so values exceeding those limits cause out-of-bounds reads/writes during table building and symbol decoding. A crafted deflate stream with nlen=288 or ndist=32 triggers this when the workaround…

*PoC sketch:* Craft a deflate stream with a dynamic block header where BITS(5) = 31 (nlen = 288) or BITS(5) = 31 (ndist = 32). Compile zlib with -DPKZIP_BUG_WORKAROUND. The stream passes the disabled check and proceeds to CODELENS, where state->lens is filled up to index 319. Subsequent inflate_table() calls for …

### `zlib/inftrees.c [inflate_table D: incomplete code + oversubscribed check]` — L68 — conf=0.9
**Off-by-one Heap Buffer Overflow due to Incomplete Code**

If the Huffman code is incomplete, the code attempts to fill the remaining table entry at `next[huff]` (line 74). Since `next` points to `*table + used`, and `huff` can be 1, this results in a write to `(*table)[used + 1]`. If `used` is exactly equal to `ENOUGH_LENS` or `ENOUGH_DISTS`, this write goes one entry past the end of the allocated buffer. The check on lines 57-59 only returns an error if `used > ENOUGH`, allowing `used == ENOUGH` to pass. This leads to an off-by-one heap buffer overflow.

*PoC sketch:* Craft a DEFLATE stream with an incomplete dynamic Huffman tree that causes `used` to be exactly `ENOUGH_LENS` or `ENOUGH_DISTS`. This will trigger the off-by-one write on line 74 when the incomplete code handling block is executed.

### `zlib/inflate.c [inflateInit_+inflateGetHeader+setDictionary]` — L10 — conf=0.88
**Gzip integrity check bypass via incorrect wrap value**

A direct consequence of the `+ 5` wrap calculation bug: when windowBits=31 (gzip mode), wrap is set to 6 (binary 110) instead of 2 (binary 010). The inflate engine checks `state->wrap & 1` to determine whether to verify the CRC32 trailer on gzip streams. Since 6 & 1 == 0, the CRC32 verification is silently disabled. An attacker can craft a gzip payload with arbitrary data and an invalid CRC32, and the decompressor will accept it without detecting the corruption. This undermines the integrity guarantee of gzip compression and could enable injection of malicious decompressed content in applicati…

*PoC sketch:* import zlib import struct  # Craft a gzip stream with invalid CRC data = b'Hello World' # Proper gzip compression compressed = zlib.compress(data, wbits=31)  # gzip mode # Corrupt the CRC32 trailer (last 8 bytes: CRC32 + ISIZE) corrupted = compressed[:-8] + struct.pack('<II', 0xDEADBEEF, len(data)) …

### `zlib/inftrees.c [inflate_table B: canonical codes + index]` — L55 — conf=0.85
**Unchecked lens[] index causes out-of-bounds write on count[] array**

The loop `count[lens[sym]]++` uses `lens[sym]` as an index into the `count[]` array without validating that `lens[sym] <= MAXBITS`. The comment explicitly acknowledges this: 'This routine assumes, but does not check, that all of the entries in lens[] are in the range 0..MAXBITS.' If a malformed deflate stream provides a code length value exceeding MAXBITS (typically 15), this results in an out-of-bounds write on the stack-allocated `count[]` array (declared as `count[MAXBITS+1]`, i.e., 16 entries for indices 0-15). An attacker crafting a malicious compressed payload could control `lens[]` valu…

*PoC sketch:* Craft a deflate stream with a dynamic Huffman block where one of the code length values in the HLIT/HDIST code length alphabet exceeds 15 (MAXBITS). For example, set a code length of 16-255 for a symbol. When inflate_table processes this, `count[lens[sym]]++` writes beyond the `count[16]` array boun…

### `zlib/inffast.c [inflate_fast A: literal/length decode entry]` — L82 — conf=0.75
**Unsigned integer underflow in input boundary calculation**

The calculation `last = in + (strm->avail_in - 5)` uses unsigned arithmetic. If `strm->avail_in` is less than 6 (violating the documented entry assumption), the subtraction underflows, wrapping `last` to a pointer far beyond the input buffer. This eliminates the loop's input bounds check, causing subsequent `*in++` reads to access memory far past the allocated input buffer. While entry assumptions document `avail_in >= 6`, there is no runtime validation, and a caller bug or memory corruption could violate this invariant.

*PoC sketch:* Craft a z_stream where avail_in = 0..5 and call inflate_fast. The unsigned subtraction wraps, e.g., avail_in=3 yields (3-5) = 0xFFFFFFFE (on 32-bit), making `last` point ~4GB past `in`. The loop then reads far beyond the input buffer.

### `zlib/inffast.c [inflate_fast A: literal/length decode entry]` — L85 — conf=0.75
**Unsigned integer underflow in output boundary calculation**

The calculation `end = out + (strm->avail_out - 257)` uses unsigned arithmetic. If `strm->avail_out` is less than 258 (violating the documented entry assumption), the subtraction underflows, wrapping `end` to a pointer far beyond the output buffer. This eliminates the loop's output bounds check, allowing writes far past the allocated output buffer. No runtime validation of the entry assumption exists.

*PoC sketch:* Craft a z_stream where avail_out < 258 and call inflate_fast. The unsigned subtraction wraps, making `end` point far past the output buffer. The loop then writes match/literal data beyond the output allocation, causing heap corruption.

### `zlib/inffast.c [inflate_fast C: copy + distance check]` — L238 — conf=0.75
**Missing distance bounds check in direct output copy path**

In the 'copy direct from output' else branch, `from = out - dist` is computed without validating that `dist <= (out - output_start)`. A malformed compressed stream with a distance code referencing bytes before the start of the output buffer causes `from` to point out-of-bounds. The subsequent unrolled copy loop (`do { *out++ = *from++; ... } while (len > 2)`) then reads from invalid memory, leading to an out-of-bounds read (information disclosure) or crash (denial of service). The same issue exists at line 207 (`from = out - dist`) and line 224 (`from = out - dist`) in the overlapping-copy and…

*PoC sketch:* Craft a deflate stream where a length/distance pair specifies a distance larger than the number of bytes already written to the output buffer. For example, emit a distance code of 256 when only 10 bytes have been decompressed. The inflate_fast function will compute `from = out - 256`, pointing 246 b…

### `zlib/inftrees.c [inflate_table C: table fill + bit-length dist]` — L131 — conf=0.75
**Out-of-bounds array access via unvalidated lens[sym] index into offs[]**

The loop `for (sym = 0; sym < codes; sym++) if (lens[sym] != 0) work[offs[lens[sym]]++] = (unsigned short)sym;` uses `lens[sym]` as an index into the `offs` array without validating that `lens[sym] <= MAXBITS`. The `offs` array has only `MAXBITS+1` entries (indices 0..MAXBITS). If a malformed compressed stream provides a code length value exceeding MAXBITS (15), `offs[lens[sym]]` performs an out-of-bounds read, yielding a garbage value that is then used as a write index into `work`, causing an out-of-bounds write. This can corrupt heap metadata or adjacent data, potentially leading to arbitrar…

*PoC sketch:* Craft a deflate stream where the Huffman code length table contains a value > 15 (MAXBITS) for a distance or literal/length symbol. For example, set lens[i] = 18 for some symbol i. When inflate_table is called with type=DISTS or LENS, the count[] array would already have been corrupted (OOB write in…

### `zlib/gzread.c [gzgetc+gzungetc+gzgets+gzrewind: string read API]` — L440 — conf=0.75
**Insufficient bounds check in gzungetc allows heap buffer underflow**

The gzungetc() function's 'out of room' check `state->x.have == (state->size << 1)` only validates the total count of available bytes against the buffer size. It does NOT account for the position of `state->x.next` within the buffer. When decompressed data occupies the beginning of the output buffer (first half), `state->x.next` points near `state->out`. If the user calls gzungetc() more times than bytes have been consumed from the decompressed data, the code (after the shown check) would decrement `state->x.next` below `state->out`, writing the pushed-back character before the allocated heap …

*PoC sketch:* 1. Open a crafted gzip file for reading. 2. Call gzread(buf, 1) — this decompresses data into the internal buffer starting at state->out, returns 1 byte, leaving state->x.next = state->out + 1, state->x.have = N-1. 3. Call gzungetc(c1, file) — state->x.next becomes state->out, state->x.have = N. Che…

### `zlib/inflate.c [inflate() CHECK+LENGTH+DONE: checksum+end states]` — L69 — conf=0.7
**inflateGetDictionary: Unsigned integer underflow in circular buffer copy if wnext > whave**

The expression `state->whave - state->wnext` is computed without validating that `state->wnext <= state->whave`. Both fields are unsigned. If internal state corruption (e.g., via a separate bug, memory safety issue in a caller, or fuzzer-driven input) causes wnext to exceed whave, the subtraction wraps to a very large unsigned value, causing zmemcpy to read far beyond the window buffer and write far beyond the dictionary buffer — a critical heap buffer over-read/over-write.

*PoC sketch:* 1. Craft or corrupt an inflate_state where state->wnext = 1000 and state->whave = 500. 2. Call inflateGetDictionary with any valid dictionary buffer. 3. state->whave - state->wnext underflows to ~4294966796 (on 32-bit) or ~18446744073709550616 (on 64-bit). 4. zmemcpy attempts to copy that many bytes…


## 🟡 MEDIUM (12)

### `zlib/inflate.c [inflateInit_+inflateGetHeader+setDictionary]` — L95 — conf=0.85
**Race condition in fixedtables with BUILDFIXED**

When BUILDFIXED is defined, the fixedtables() function uses a static `virgin` flag as a one-time initialization guard without any synchronization. In a multi-threaded application, two threads can simultaneously observe `virgin == 1`, both enter the initialization block, and concurrently write to the shared `fixed[]`, `lenfix`, and `distfix` static arrays. This can result in one thread reading partially-initialized Huffman decoding tables while another is writing them, leading to incorrect decompression, out-of-bounds table lookups, or crashes. The code comment acknowledges this: 'may not be th…

*PoC sketch:* 1. Compile zlib with -DBUILDFIXED. 2. Create two threads that simultaneously call inflate() on separate z_stream instances, both triggering fixedtables() for the first time. 3. Thread A begins building the fixed tables, writing to `fixed[]`. 4. Thread B also sees `virgin == 1`, starts overwriting `f…

### `zlib/inflate.c [inflate() CHECK+LENGTH+DONE: checksum+end states]` — L68 — conf=0.85
**inflateGetDictionary: Missing buffer size validation enables buffer overflow**

The inflateGetDictionary() function copies state->whave bytes to the user-provided dictionary buffer without accepting or validating a buffer size parameter. A caller that allocates an undersized buffer (e.g., based on a stale or assumed dictLength) will suffer a heap or stack buffer overflow. The function design requires a two-call pattern (first with dictionary=NULL to query length, then with allocated buffer), but nothing enforces this, making it a persistent footgun for downstream consumers.

*PoC sketch:* 1. Initialize inflate stream and decompress data that produces a 32768-byte dictionary. 2. Call inflateGetDictionary with a dictionary buffer smaller than 32768 bytes. 3. zmemcpy writes beyond the allocated buffer, corrupting adjacent heap memory.

### `zlib/inflate.c [inflateUndermine+inflateValidate+CodesUsed]` — L1468 — conf=0.75
**Missing bounds check for state->next pointer adjustment in inflateCopy**

In the inflateCopy function, the `lencode` and `distcode` pointers are only adjusted to point into the copied `codes` array if `lencode` falls within the bounds of the original `codes` array (checked against `state->codes + ENOUGH - 1`). However, the `next` pointer is unconditionally adjusted via `copy->next = copy->codes + (state->next - state->codes)` without any bounds validation. If `state->next` does not point within `state->codes` (e.g., due to state corruption from a prior vulnerability or malformed input), the pointer arithmetic `state->next - state->codes` produces an invalid offset, …

*PoC sketch:* 1. Craft a zlib stream that, during inflation, causes state corruption such that state->next points outside state->codes (e.g., via a prior heap buffer overflow in Huffman decoding). 2. Call inflateCopy() on the corrupted stream. 3. The copy->next pointer will be computed as copy->codes + (invalid_o…

### `zlib/inftrees.c [inflate_table B: canonical codes + index]` — L18 — conf=0.7
**Poison values in lext/dext arrays enable buffer over-read if invalid codes are decoded**

The `lext[]` array contains poison values at indices 29 and 30 (values 203 and 77), and `dext[]` contains poison value 64 at indices 30 and 31. These correspond to invalid deflate codes (length codes 286-287 and distance codes 30-31). If a malformed stream causes these codes to be assigned valid Huffman codewords (because the shown code does not reject symbols beyond the valid range — it only checks for zero-length codes), decoding would attempt to read 203, 77, or 64 extra bits from the input stream, causing a massive buffer over-read. The counting logic at lines 55-56 and the max-finding loo…

*PoC sketch:* Construct a dynamic Huffman block specifying code lengths for length symbol 286 or 287 (beyond the valid 257-285 range), or distance symbols 30-31 (beyond valid 0-29). The inflate_table function will assign them valid Huffman codes. When the decoder later encounters these codes, it reads lext[29]=20…

### `zlib/gzread.c [gz_look+gz_decomp: gzip header detect + decompress]` — L47 — conf=0.7
**Unchecked memcpy in raw I/O copy path relies on fragile buffer size assumption**

The memcpy in gz_look copies strm->avail_in bytes from the input buffer to state->out without validating that strm->avail_in does not exceed the output buffer capacity. The code comment explicitly acknowledges this relies on the assumption that 'the output buffer is larger than the input buffer.' If this assumption is violated (e.g., through code modification, custom builds, or corruption of strm->avail_in), a heap buffer overflow occurs. In the standard build, state->out is allocated with state->want+1 bytes while state->in gets state->want bytes, so the +1 margin is the only guard — and it e…

*PoC sketch:* Open a gz file where the first two bytes are NOT the gzip magic (0x1f 0x8b), triggering the COPY path. If the implementation is built with equal-sized input/output buffers (violating the assumption), providing input >= output buffer size causes heap overflow. Even in standard builds, if strm->avail_…

### `zlib/gzread.c [gzread+gzfread: public read API]` — L44 — conf=0.7
**Missing NULL buffer validation in gzfread**

The gzfread function does not validate that the 'buf' parameter is non-NULL before passing it to gz_read(). If a caller passes NULL as buf with a non-zero len (after the overflow check passes), gz_read will attempt to write to a NULL pointer, causing a crash or potential memory corruption. Unlike fread() which has undefined behavior for NULL buffers, a security-conscious library should validate this input.

*PoC sketch:* gzfread(NULL, 1, 100, file); // file is a valid, open gzFile -- causes NULL dereference in gz_read

### `zlib/gzread.c [gzread+gzfread: public read API]` — L14 — conf=0.7
**Missing NULL buffer validation in gzread**

The gzread function does not validate that the 'buf' parameter is non-NULL before passing it to gz_read(). If a caller passes NULL as buf with len > 0, gz_read will attempt to write decompressed data to address 0, causing a segmentation fault or potential exploitation depending on the platform's memory layout.

*PoC sketch:* gzread(file, NULL, 100); // file is a valid, open gzFile -- causes NULL dereference in gz_read

### `zlib/inftrees.c [inflate_table B: canonical codes + index]` — L55 — conf=0.65
**No validation that codes parameter matches valid symbol range for lbase/lext/dbase/dext arrays**

The function accepts a `codes` parameter that determines how many symbols to process from `lens[]`, but never validates that `codes` falls within the valid range for the corresponding table type (length codes: 257-285, distance codes: 0-29). The max-finding loop only checks for non-zero counts but doesn't reject symbols at invalid indices. Combined with the poison values in lext/dext, this means the function will happily build decode table entries for invalid codes 286, 287 (length) or 30, 31 (distance), which when later used for decoding produce undefined behavior.

*PoC sketch:* Call inflate_table with TYPE=CODES (lengths) and codes=288 (instead of max 286 valid), providing non-zero lens[] entries for symbols 286-287. Or call with TYPE=DIST and codes=32 with non-zero entries for symbols 30-31. The function processes them without error, creating decode entries that reference…

### `zlib/inffast.c [inflate_fast A: literal/length decode entry]` — L109 — conf=0.55
**Potential unsigned integer underflow in bits counter at dolen label**

At the `dolen:` label, `bits -= op` is performed where both are unsigned. While the initial code path (lines 103-107) ensures `bits >= 15` before the first table lookup, the `dolen:` label can be reached via `goto` from subtable resolution paths where `bits` may be significantly smaller. If `op` (from `here->bits`) exceeds the current `bits` count — possible with corrupted Huffman tables or malformed compressed data — the subtraction underflows, making `bits` a very large value. This causes subsequent `hold += (unsigned long)(*in++) << bits` to invoke undefined behavior (shift exceeding type w…

*PoC sketch:* Provide a crafted deflate stream with a malformed Huffman table where a subtable entry's `bits` field exceeds the remaining bit count. When the `dolen:` path is reached via subtable goto, `bits -= op` underflows. Subsequent shift operations become undefined behavior, and the loop reads past the inpu…

### `zlib/inffast.c [inflate_fast C: copy + distance check]` — L230 — conf=0.55
**Unrolled copy loop lacks output buffer bounds check**

The unrolled copy loop `while (len > 2) { *out++ = *from++; *out++ = *from++; *out++ = *from++; len -= 3; }` and the tail copy `if (len) { *out++ = *from++; if (len > 1) *out++ = *from++; }` do not check whether `out` has reached the end of the output buffer. If the remaining output space is less than `len` bytes, this results in a heap buffer overflow write. While the outer inflate_fast loop typically ensures sufficient space, a crafted stream with a maximal-length match (258 bytes) near the output buffer boundary could overflow by up to 257 bytes.

*PoC sketch:* Provide a deflate stream that decompresses to fill the output buffer almost completely, then emits a length/distance pair with length 258 when fewer than 258 bytes of output space remain. The unrolled copy will write past the allocated output buffer.

### `zlib/inftrees.c [inflate_table C: table fill + bit-length dist]` — L121 — conf=0.55
**Incomplete Huffman code set allowed for DISTS when max==1 bypasses safety check**

The incomplete-set check `if (left > 0 && (type == CODES || max != 1)) return -1;` deliberately allows incomplete code sets when type is DISTS (or LENS) and max==1. While this is intentional for single-symbol distance codes per the deflate spec, it means that a crafted stream with a single distance code of length 1 will produce a table where half the entries (for the unused bit pattern) must be filled by the post-loop invalid-marker code. If that post-loop fill is skipped or has a bug, uninitialized table entries could be dereferenced during decompression, leading to use of uninitialized value…

*PoC sketch:* Construct a deflate dynamic block with exactly one distance code of bit-length 1 (e.g., distance code 0 with length 1, all others length 0). The left value after the Kraft check will be 1 (> 0), but the incomplete-set check is bypassed because max==1 and type==DISTS. If the post-loop fill that write…

### `zlib/inffast.c [inflate_fast C: copy + distance check]` — L202 — conf=0.4
**do-while loop with unsigned op counter risks infinite loop on zero**

The `do { *out++ = *from++; } while (--op)` pattern executes at least once and decrements `op` (likely an unsigned type). If `op` is ever 0 when entering this loop—due to an edge case in the length/distance decoding—`--op` wraps to UINT_MAX (or 65535 for 16-bit), causing the loop to iterate far beyond intended bounds. This results in a massive buffer over-read and over-write. While deflate match lengths are nominally >= 3, a malformed stream with corrupted length codes could potentially reach this path with op=0.

*PoC sketch:* Craft a deflate stream where the length code decoding produces an op value of 0 through integer arithmetic edge cases (e.g., len=0 after prior subtractions). The do-while loop will underflow op and iterate ~65535 or ~4 billion times, corrupting heap memory.


## 🔵 LOW (12)

### `zlib/inflate.c [inflateUndermine+inflateValidate+CodesUsed]` — L1490 — conf=0.7
**State modification on error return path in inflateUndermine**

When `INFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR` is not defined, `inflateUndermine` sets `state->sane = 1` and then returns `Z_DATA_ERROR`. This modifies the stream state even though the function returns an error code. Callers that check the return value for errors may assume no state mutation occurred on failure, violating the principle that error paths should not have observable side effects. While setting `sane = 1` is the safe default (enabling distance validation), this unexpected state change could interfere with error recovery logic in callers that attempt to continue using the stream …

*PoC sketch:* strm = inflateInit(...); // Set state->sane = 0 somehow (if INFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR is defined) ret = inflateUndermine(strm, 1); // Even if ret == Z_DATA_ERROR, state->sane has been silently changed to 1 // Caller may not expect state mutation on error return

### `zlib/gzread.c [gzgetc+gzungetc+gzgets+gzrewind: string read API]` — L449 — conf=0.7
**Potential off_t underflow in gzungetc when called at file position 0**

In gzungetc(), when the output buffer is empty (state->x.have == 0), the code unconditionally decrements state->x.pos (line 450: `state->x.pos--`). If gzungetc() is called when the file position is 0 (beginning of file, no bytes yet read), state->x.pos underflows. Since x.pos is of type off_t (signed), it becomes -1. This corrupts the internal position tracking, which could cause incorrect behavior in subsequent seek operations (gzseek) or position queries (gztell). While not directly exploitable for code execution, it violates invariants and could cascade into further logic errors.

*PoC sketch:* 1. Open a gzip file for reading. 2. Without reading any data, call gzungetc('A', file). 3. state->x.pos decrements from 0 to -1. 4. Subsequent gztell() returns -1 (incorrect). gzseek() calculations based on pos may behave unexpectedly.

### `zlib/inflate.c [inflate() CHECK+LENGTH+DONE: checksum+end states]` — L82 — conf=0.65
**inflateSetDictionary: Permissive state check when wrap==0 allows dictionary injection in unexpected modes**

The guard `if (state->wrap != 0 && state->mode != DICT) return Z_STREAM_ERROR;` only restricts dictionary setting when wrap is non-zero. When wrap==0 (raw deflate), the dictionary can be set in ANY mode — including DONE, BAD, or mid-decompression states like LEN. This could allow an attacker to inject a crafted dictionary after partial decompression has occurred, potentially altering the output of already-processed data in subsequent calls, or corrupting internal state via updatewindow when the stream is in an inconsistent mode.

*PoC sketch:* 1. Initialize inflate with windowBits = -15 (raw deflate, wrap==0). 2. Partially decompress data until mode is LEN or COPY. 3. Call inflateSetDictionary with a crafted dictionary. 4. Continue decompression — output for remaining data uses the injected dictionary, potentially producing attacker-contr…

### `zlib/inflate.c [inflate() CHECK+LENGTH+DONE: checksum+end states]` — L107 — conf=0.6
**inflateGetHeader: Stored raw pointer to user memory creates use-after-free risk**

inflateGetHeader stores a raw pointer to the caller's gz_header structure in state->head without lifetime management. If the caller frees or scopes out the head structure before inflate completes header parsing, subsequent writes to state->head fields (e.g., head->done, head->extra, etc.) during inflate become use-after-free. While this is a standard C API pattern, the lack of any documentation-enforced lifetime contract makes it a latent risk, especially in wrapper libraries that may not preserve the header object's lifetime.

*PoC sketch:* 1. Allocate gz_header on the stack in a helper function. 2. Call inflateGetHeader(strm, &head). 3. Return from the helper function (head is now dangling). 4. Call inflate, which writes to state->head->done — use-after-free on stack memory.

### `zlib/inflate.c [inflateUndermine+inflateValidate+CodesUsed]` — L1465 — conf=0.6
**distcode pointer adjusted without independent bounds check in inflateCopy**

In inflateCopy, the bounds check only validates that `state->lencode` falls within the `codes` array range. If the check passes, both `lencode` AND `distcode` are adjusted to point into `copy->codes`. However, `distcode` is never independently validated. If `lencode` is within bounds but `distcode` is not (pointing outside `state->codes`), the offset `state->distcode - state->codes` would be negative or exceed the array size, causing `copy->distcode` to point outside `copy->codes`. This inconsistency could lead to out-of-bounds memory access when the distance Huffman table is used during infla…

*PoC sketch:* If a state exists where lencode points within codes but distcode points outside (e.g., distcode points to a fixed table at a different base address), and the lencode bounds check passes, then: copy->distcode = copy->codes + (state->distcode - state->codes) This produces an invalid pointer since the …

### `zlib/inftrees.c [inflate_table C: table fill + bit-length dist]` — L112 — conf=0.6
**Unchecked table write when max==0 may overflow minimal table allocation**

When max==0 (no symbols to code), the code unconditionally writes two `here` entries via `*(*table)++ = here` without verifying that the caller-allocated table has space for at least 2 entries. The `used` accounting and the ENOUGH_LENS/ENOUGH_DISTS bounds check (lines 155-157) are never reached because the function returns early at line 118. If a caller allocates a table with fewer than 2 entries (e.g., based on a root of 0), this would be a heap buffer overflow. In practice, current zlib callers always allocate sufficient space, making this a defense-in-depth gap rather than an actively explo…

*PoC sketch:* Call inflate_table with a lens array where all entries are 0 (no symbols). The function writes 2 entries to *table without any capacity check. A custom caller that allocates exactly 1 entry for the table would experience a 1-entry heap overflow.

### `zlib/gzread.c [gz_look+gz_decomp: gzip header detect + decompress]` — L62 — conf=0.6
**Unhandled Z_BUF_ERROR in decompression loop enables potential infinite loop / denial of service**

In gz_decomp, the decompression loop handles Z_STREAM_ERROR, Z_NEED_DICT, Z_MEM_ERROR, and Z_DATA_ERROR as fatal conditions, but does NOT handle Z_BUF_ERROR. If inflate() returns Z_BUF_ERROR while both strm->avail_in > 0 and strm->avail_out > 0 (which can occur with certain malformed deflate streams), the loop condition (strm->avail_out && ret != Z_STREAM_END) remains true and the loop re-iterates without making progress, resulting in an infinite loop. This is a denial-of-service condition.

*PoC sketch:* Craft a gzip file where the compressed payload causes inflate() to return Z_BUF_ERROR repeatedly while both avail_in and avail_out remain non-zero. This can occur with truncated or subtly corrupted deflate blocks that don't trigger Z_DATA_ERROR but prevent inflate from making forward progress.

### `zlib/gzread.c [gzread+gzfread: public read API]` — L30 — conf=0.6
**Implementation-defined integer conversion in gzread overflow check**

The check `if ((int)len < 0)` in gzread relies on implementation-defined behavior when converting an unsigned int value exceeding INT_MAX to signed int. While this works correctly on all two's complement platforms (which is essentially all modern hardware), the C standard (pre-C23) does not guarantee this behavior. On a hypothetical sign-magnitude or one's complement system, or a system that raises a signal on overflow, this check could fail or cause unexpected behavior. C23 mandates two's complement, making this safe going forward.

*PoC sketch:* On a standard platform: gzread(file, buf, 0x80000000) -- the check correctly catches this as not fitting in an int. On a hypothetical non-two's-complement platform, the behavior is undefined.

### `zlib/gzread.c [gzread+gzfread: public read API]` — L100 — conf=0.5
**Ignored return value of gz_look in gzungetc**

In gzungetc, the return value of gz_look(state) is explicitly cast to void and ignored. If gz_look fails (e.g., due to memory allocation failure or I/O error), the internal state may not be properly initialized. Subsequent operations on the state could then operate on uninitialized or inconsistent data, potentially leading to undefined behavior or information disclosure.

*PoC sketch:* Open a gz file, trigger a memory exhaustion condition, then call gzungetc(). The gz_look call may fail silently, leaving state in an inconsistent condition for subsequent reads.

### `zlib/gzread.c [gzgetc+gzungetc+gzgets+gzrewind: string read API]` — L427 — conf=0.5
**Ignored return value of gz_look() in gzungetc may bypass error detection**

In gzungetc(), the call to gz_look() at line 428 has its return value explicitly cast to void and ignored: `(void)gz_look(state);`. If gz_look() fails (returns -1), the function continues execution without detecting the failure. While subsequent error state checks may catch some failure modes, there is a window between the gz_look() call and the error check where internal state may be inconsistent. If gz_look() partially modifies state before failing, the subsequent checks might not fully account for the inconsistent state, potentially leading to undefined behavior.

*PoC sketch:* 1. Craft a malformed gzip file that causes gz_look() to fail after partially modifying internal state. 2. Open the file and call gzungetc() — gz_look() fails but return value is ignored. 3. Function continues with potentially inconsistent state, possibly leading to incorrect behavior or crash.

### `zlib/inffast.c [inflate_fast A: literal/length decode entry]` — L103 — conf=0.4
**Unbounded input reads without per-iteration bounds check**

The bit-filling code `hold += (unsigned long)(*in++) << bits` reads two bytes unconditionally when `bits < 15`, relying solely on the `last` pointer for bounds. If `last` is corrupted or miscalculated (e.g., due to the integer underflow in finding #1), these reads access memory beyond the input buffer. Even with correct `last`, the loop condition checks `in < last` only at the loop boundary, not before each byte read within the iteration, creating a window where reads can exceed the buffer by up to the bytes consumed in one iteration.

*PoC sketch:* With a carefully sized input buffer where avail_in is exactly 6, `last = in + 1`. After one iteration consumes bytes, `in` may equal `last`, but the loop body still reads `*in++` twice before the next loop condition check, reading 1-2 bytes past the valid input.

### `zlib/gzread.c [gz_look+gz_decomp: gzip header detect + decompress]` — L78 — conf=0.4
**Potential integer truncation in decompressed size calculation**

The calculation `state->x.have = had - strm->avail_out` computes the number of decompressed bytes as an unsigned subtraction (both `had` and `strm->avail_out` are unsigned). The result is assigned to `state->x.have`, which is typically a signed `int`. If the unsigned result exceeds INT_MAX, the assignment causes implementation-defined behavior (or undefined behavior per strict interpretation). Subsequently, `state->x.next = strm->next_out - state->x.have` uses this potentially corrupted value in pointer arithmetic, which could point outside the output buffer. While practically impossible with …

*PoC sketch:* Set state->want to a value near UINT_MAX via gzbuffer() before reading. If inflate produces output exceeding INT_MAX bytes across loop iterations, the unsigned-to-signed truncation corrupts state->x.have, and the subsequent pointer arithmetic in state->x.next = strm->next_out - state->x.have yields …


## ⚪ INFO (3)

### `zlib/inflate.c [inflate() TYPE+STORED+LEN: block type dispatch]` — L935 — conf=0.65
**Missing bounds check in CODELENS repeat codes can overflow lens array**

In the CODELENS case, repeat codes 16 (copy=3+BITS(2), up to 5), 17 (copy=3+BITS(3), up to 10), and 18 (copy=11+BITS(7), up to 138) can write multiple entries to state->lens[state->have++] in a fill loop. The visible code sets 'copy' and 'len' but does not show any check that state->have + copy <= state->nlen + state->ndist before filling. If a repeat code appears near the end of the code length sequence, the fill could push state->have beyond the allocated lens[320] boundary. For example, with nlen=286, ndist=30 (total 316), if state->have=310 and a code 18 yields copy=138, the fill would wri…

*PoC sketch:* Craft a dynamic deflate block where the code length sequence ends with a code-18 symbol (repeat 11-138 zeros) when state->have is near nlen+ndist. For instance: set nlen=257, ndist=1 (total 258), provide 256 literal code lengths, then emit a code-18 with BITS(7)=127 (copy=138). This attempts to writ…

### `zlib/inflate.c [inflate() TYPE+STORED+LEN: block type dispatch]` — L948 — conf=0.45
**Insufficient validation of code length repeat code 16 references prior entry**

When here.val == 16 (repeat previous code length), the code checks 'state->have == 0' to prevent accessing lens[-1], but it does NOT validate that state->lens[state->have - 1] was actually set to a meaningful value in the current CODELENS iteration. If the previous entry was filled by an earlier repeat code that itself produced an invalid length, the repeated value propagates without validation. Combined with the missing upper bound check on the repeat count, this allows amplification of any corrupted length value throughout the lens array.

*PoC sketch:* In a crafted stream, set an initial code length to an unusual value (e.g., 15), then use code 16 repeats to propagate it across many entries. While 15 is technically valid for code lengths, the pattern demonstrates unvalidated propagation. More impactful when combined with the repeat-count overflow …

### `zlib/gzread.c [gz_look+gz_decomp: gzip header detect + decompress]` — L30 — conf=0.3
**inflateReset on gzip magic detection without consuming header leaves stale input state**

When gzip magic bytes (0x1f, 0x8b) are detected, inflateReset(strm) is called but the magic bytes are NOT consumed from the input buffer. The function returns with state->how = GZIP and strm->next_in still pointing at the magic bytes. While inflate() with windowBits=31 will re-parse the gzip header on the next gz_decomp call, this design means the magic byte detection and the actual inflate header parsing are decoupled. If inflateReset changes internal state in a way inconsistent with the remaining input (e.g., on a second gzip stream after Z_STREAM_END), there could be subtle state mismatches…

*PoC sketch:* Provide a concatenated gzip stream: a complete valid gzip stream followed by another gzip stream. After the first stream ends (Z_STREAM_END), gz_look is re-entered. The inflateReset is called again, but the inflate state from the previous stream's end may interact unexpectedly with the new stream's …

