# freetype — `tt_cmap4_char_map_binary` OOB reads — ❌ FALSE POSITIVE (refuted by code semantics, 2026-05-20)

Two findings from the 2026-05-04 scan (Pipeline A H verdict CONFIRMED conf 1.0/0.99,
both J3 = NO_EXTRACT so they were silently dropped from survivors) flagged
`tt_cmap4_char_map_binary` in `src/sfnt/ttcmap.c`:

- **#8** — "Heap OOB Read via conditional `if (next && p > limit)` skipped on
  `FT_Get_Char_Index` path." Model claimed correspondence to CVE-2021-36976.
- **#14** — "OOB read in overlapping-segment backward search loop:
  `for (i = max; i > 0; i--)` reads with no per-iteration limit check."

Both are refuted by reading the surrounding load-time validator
(`tt_cmap4_validate`, same file, lines 900–1099).

## Why #14 is safe

Validation enforces (line 952):
```c
if ( length < 16 + num_segs * 2 * 4 )  /* i.e. length >= 16 + 8*num_segs */
    FT_INVALID_TOO_SHORT;
```
The backward loop's maximum pointer offset is `14 + (i-1)*2 + 2 + 6*num_segs`,
which for `i ≤ num_segs` is at most `12 + 8*num_segs`. Plus the 2-byte
`TT_PEEK_USHORT` read: `13 + 8*num_segs < 16 + 8*num_segs ≤ length`. The loop
stays within the validated cmap region.

## Why #8 is safe (the more interesting case)

`tt_cmap4_validate` runs at face-load time at `FT_VALIDATE_DEFAULT` level
(invocation at `ttcmap.c:3838`):
```c
ft_validator_init(FT_VALIDATOR(&valid), cmap, limit, FT_VALIDATE_DEFAULT);
...
error = clazz->validate(cmap, FT_VALIDATOR(&valid));
```
At default level, for every segment except a last segment of `(0xFFFF, 0xFFFF)`,
validation rejects the cmap unless (`ttcmap.c:1059-1061`):
```c
if ( p < glyph_ids ||
     p + ( end - start + 1 ) * 2 > valid->limit )
    FT_INVALID_DATA;
```
At runtime in `tt_cmap4_char_map_binary` line 1405:
```c
p += offset + ( charcode - start ) * 2;
```
Since the matching binary search guarantees `start <= charcode <= end`,
`(charcode - start) * 2 < (end - start + 1) * 2`. Combining with validation's
constraint, `p + offset + (charcode - start) * 2 < valid->limit ≤ limit`.
**Therefore `p > limit` cannot occur at line 1408 for a face that passed
validation, except via the bad-last-segment path.**

The bad-last-segment `(0xFFFF, 0xFFFF)` case is handled at **runtime** by
lines 1281–1289 — before the offset is consumed:
```c
if ( mid >= num_segs - 1 && start == 0xFFFFU && end == 0xFFFFU )
{
    if ( offset && p + offset + 2 > limit )
    {
        delta  = 1;
        offset = 0;   /* forces the else branch below */
    }
}
```
With `offset == 0`, control falls through to the `else` branch at line 1419 and
the `TT_PEEK_USHORT(p)` at line 1411 is never executed.

So `if ( next && p > limit )` at line 1408 is **defense-in-depth**, not the
primary protection — the primary protection is the load-time validator plus the
runtime fixup for the last-segment edge case. The model's reasoning that "next=0
bypasses the bounds check, allowing OOB read" is correct in isolation but
unreachable because `p > limit` cannot occur on the `next=0` path for a
validated face.

### Note on CVE-2021-36976

The Pipeline A reasoning claimed "this corresponds to CVE-2021-36976." That CVE
*was* in this exact code path historically, and the fix that introduced the
load-time validation invariant (the very check that now refutes our finding) is
what closed it. The current upstream code is the post-fix code; the model
flagged the residual pattern without noticing the validator that makes it safe.

## Provenance

- Pipeline A flags: glm-5.1, scan run 2026-05-04, two findings on `ttcmap.c`
- H adversarial verify: glm-5.1, both CONFIRMED at conf 1.0 / 0.99
- J3 cross-model: glm-4-plus, both **NO_EXTRACT** (J2 failed to locate
  `freetype/ttcmap.c [tt_cmap4_char_map_binary B: range index calc L1382-1484]`)
- These never reached J3 cross-check or J5 draft because J2 grounding lookup
  failed — improvement #5 in AGENTS.md.
- Refutation: whole-file code-semantics verification against
  `freetype/master:src/sfnt/ttcmap.c`, 2026-05-20.

## Status

Negative result. Not disclosed.
