# openssl — GLM-5.1 Pipeline A scan (2026-05-04)

- **Model**: glm-5.1
- **Segments**: 32 (0 hit GLM API timeout)
- **Total findings**: 57
  - 🔴 **CRITICAL**: 1
  - 🟠 **HIGH**: 11
  - 🟡 **MEDIUM**: 21
  - 🔵 **LOW**: 19
  - ⚪ **INFO**: 5

## Per-segment summary

| Segment | Time (s) | Findings | Status |
|---|---:|---:|---|
| `openssl/tasn_dec.c [ASN1_item_d2i + asn1_item_embed_d2i entry]` | 51.6 | 0 | ✅ ok |
| `openssl/tasn_dec.c [asn1_item_embed_d2i A: SEQUENCE decode]` | 162.4 | 3 | ✅ ok |
| `openssl/tasn_dec.c [asn1_item_embed_d2i B: SET/CHOICE decode]` | 360.5 | 0 | ✅ ok |
| `openssl/tasn_dec.c [asn1_item_embed_d2i C: template+length]` | 143.7 | 3 | ✅ ok |
| `openssl/tasn_dec.c [asn1_item_embed_d2i D: EXPLICIT+primitive]` | 352.7 | 3 | ✅ ok |
| `openssl/tasn_dec.c [asn1_template_ex_d2i+noexp_d2i]` | 360.5 | 0 | ✅ ok |
| `openssl/tasn_dec.c [asn1_d2i_ex_primitive: DER primitive decoder]` | 147.5 | 3 | ✅ ok |
| `openssl/tasn_dec.c [asn1_ex_c2i: content byte decode]` | 360.4 | 0 | ✅ ok |
| `openssl/tasn_dec.c [asn1_find_end+asn1_collect: indefinite length]` | 360.4 | 0 | ✅ ok |
| `openssl/tasn_dec.c [asn1_check_tlen+collect_data: tag+len check]` | 360.5 | 0 | ✅ ok |
| `openssl/a_int.c [ASN1_INTEGER_cmp+c2i_ASN1_INTEGER entry]` | 360.5 | 0 | ✅ ok |
| `openssl/a_int.c [c2i_ASN1_INTEGER body+sign handling]` | 360.4 | 0 | ✅ ok |
| `openssl/a_int.c [asn1_get_uint64+asn1_get_int64: overflow guards]` | 360.5 | 0 | ✅ ok |
| `openssl/a_int.c [asn1_string_get/set_uint64+i2c_ASN1_INTEGER]` | 360.4 | 0 | ✅ ok |
| `openssl/a_int.c [ASN1_INTEGER_set+bn conversions]` | 83.5 | 3 | ✅ ok |
| `openssl/ssl3_cbc.c [CBC padding+Lucky13 header]` | 114.8 | 4 | ✅ ok |
| `openssl/ssl3_cbc.c [ssl3_cbc_digest_record A: HMAC setup]` | 87.3 | 4 | ✅ ok |
| `openssl/ssl3_cbc.c [ssl3_cbc_digest_record B: MAC blocks]` | 257.7 | 4 | ✅ ok |
| `openssl/ssl3_cbc.c [ssl3_cbc_digest_record C: output]` | 360.5 | 0 | ✅ ok |
| `openssl/ssl3_cbc.c [ssl3_cbc_digest_record D: padding verify]` | 360.5 | 0 | ✅ ok |
| `openssl/dh_key.c [DH_compute_key+DH_compute_key_padded (CVE-2023-5678)]` | 45.1 | 2 | ✅ ok |
| `openssl/dh_key.c [DH_compute_key body+public key validation]` | 132.2 | 4 | ✅ ok |
| `openssl/dh_key.c [dh_bn_mod_exp+DH_generate_key entry]` | 111.6 | 3 | ✅ ok |
| `openssl/dh_key.c [generate_key: parameter checks]` | 145.3 | 2 | ✅ ok |
| `openssl/dh_key.c [generate_key: FIPS validation+BN ops]` | 142.2 | 4 | ✅ ok |
| `openssl/extensions.c [tls_parse_extension+tls_parse_all_extensions A]` | 166.6 | 2 | ✅ ok |
| `openssl/extensions.c [tls_parse_all_extensions B: context check loop]` | 105.1 | 4 | ✅ ok |
| `openssl/extensions.c [tls_collect_extensions+tls_check_client_hello]` | 139.2 | 3 | ✅ ok |
| `openssl/statem_lib.c [tls_get_message_header: handshake header parse]` | 180.9 | 3 | ✅ ok |
| `openssl/statem_lib.c [tls_get_message_body A: message body read]` | 318.4 | 3 | ✅ ok |
| `openssl/statem_lib.c [ssl3_finish_mac+ssl3_take_mac: MAC ops]` | 20.3 | 0 | ✅ ok |
| `openssl/statem_lib.c [tls_get_message_body B: CCS+verify]` | 66.5 | 0 | ✅ ok |

## 🔴 CRITICAL (1)

### `openssl/ssl3_cbc.c [ssl3_cbc_digest_record A: HMAC setup]` — L56 — conf=0.6
**Uninitialized function pointers when hash algorithm is unrecognized**

The hash algorithm detection uses an if-else-if chain (MD5, SHA1, SHA2-224, SHA2-256, SHA2-384, SHA2-512...). If the provided `md` does not match any recognized algorithm, the function does not return early, and the variables `md_final_raw` and `md_transform` remain uninitialized (declared on lines 18-19 but never assigned). When these function pointers are later dereferenced and called, this leads to arbitrary code execution. The `md_size` variable also remains uninitialized (zero or garbage), potentially causing further memory corruption in downstream length calculations.

*PoC sketch:* Provide an EVP_MD object that does not match any of the checked algorithm names (e.g., SHA3-256 or a custom/engine-provided digest). The function will fall through all else-if branches without initializing md_final_raw or md_transform. When the function later calls md_transform(md_state.c, block) or…


## 🟠 HIGH (11)

### `openssl/dh_key.c [generate_key: parameter checks]` — L39 — conf=0.92
**Missing check on partial validation result in ossl_dh_buf2key allows invalid public keys**

In ossl_dh_buf2key(), the function ossl_dh_check_pub_key_partial() is called to validate the public key per RFC 8446 Section 4.2.8.1 (preventing small subgroup attacks). However, only the function's return value (0 = error performing check, 1 = check completed) is examined. The 'ret' output parameter, which contains the actual validation result flags (e.g., DH_CHECK_PUBKEY_TOO_SMALL for keys 0 or 1, DH_CHECK_PUBKEY_INVALID for keys >= p-1), is never checked. This means public keys that are 0, 1, or p-1 will be accepted despite failing the partial validation, directly violating the RFC 8446 man…

*PoC sketch:* 1. Establish a TLS connection using DHE key exchange. 2. As a malicious server (or MITM), send a ServerKeyExchange with the DH public key Y = 1 (encoded as a single 0x01 byte, or zero-padded to the size of p). 3. The client calls ossl_dh_buf2key() which calls ossl_dh_check_pub_key_partial(). The fun…

### `openssl/extensions.c [tls_collect_extensions+tls_check_client_hello]` — L25 — conf=0.92
**Missing return after SSLfatal in hostname allocation failure leads to NULL dereference and continued handshake**

When OPENSSL_strdup fails (returns NULL) while copying the SNI hostname from s->ext.hostname to s->session->ext.hostname, the code calls SSLfatal() but does NOT return 0. Execution falls through to the switch statement, which returns 1 (success) for SSL_TLSEXT_ERR_OK. This means: (1) the handshake continues despite a fatal error being marked, (2) s->session->ext.hostname is NULL while s->ext.hostname is valid, creating inconsistent state, (3) any subsequent code that dereferences s->session->ext.hostname will trigger a NULL pointer dereference (crash/DoS), and (4) the session may be stored wit…

*PoC sketch:* 1. Configure an OpenSSL server with an SNI callback (servername_cb) that returns SSL_TLSEXT_ERR_OK. 2. Force memory allocation failure for OPENSSL_strdup — e.g., by setting a very long SNI hostname near allocation limits, or by using an LD_PRELOAD malloc interposition library that fails the Nth allo…

### `openssl/tasn_dec.c [asn1_item_embed_d2i C: template+length]` — L386 — conf=0.85
**Missing trailing data check after SEQUENCE field decoding loop**

After the for-loop that decodes each template field in a SEQUENCE, there is no check that all bytes within the SEQUENCE's declared length have been consumed (i.e., `len == 0` when `seq_nolen` is not set). Extra trailing bytes inside a definite-length SEQUENCE are silently ignored. This violates DER encoding rules (which require exact length matching) and creates a parser differential vulnerability: a maliciously crafted ASN.1 structure can contain arbitrary trailing data that is accepted by OpenSSL but may be interpreted differently by a stricter parser. This class of vulnerability has histori…

*PoC sketch:* Craft a DER-encoded SEQUENCE where the outer TLV length is larger than the sum of the encoded inner fields. For example, a SEQUENCE of length 20 containing only a single INTEGER of length 2 (total 4 bytes inner), leaving 16 bytes of trailing garbage. OpenSSL will decode the INTEGER successfully and …

### `openssl/tasn_dec.c [asn1_d2i_ex_primitive: DER primitive decoder]` — L790 — conf=0.85
**Integer truncation from long to int when calling asn1_ex_c2i**

The local variable `len` is declared as `long` and is computed from various sources that can exceed INT_MAX: (1) `len = plen` for primitive types, (2) `len = p - cont + plen` for SEQUENCE/SET/OTHER, (3) `len = buf.length` for constructed strings (where `buf.length` is `size_t`). This `long` value is then passed to `asn1_ex_c2i()` whose `len` parameter is declared as `int`, causing silent truncation of the upper 32 bits on LP64 systems (64-bit Linux/Unix). If the truncated `int` value becomes negative, and is subsequently used in operations that implicitly convert it to `size_t` (e.g., memcpy, …

*PoC sketch:* Craft a DER-encoded ASN.1 structure with a constructed OCTET STRING containing many small chunks whose total concatenated length exceeds 2^31 bytes. On a 64-bit system with sufficient memory, `asn1_collect` will build a BUF_MEM with `buf.length > INT_MAX`. When `len = buf.length` is passed to `asn1_…

### `openssl/dh_key.c [generate_key: FIPS validation+BN ops]` — L325 — conf=0.85
**Missing minimum private key length validation in non-FIPS q=NULL path**

When q is NULL and not in FIPS mode, the code only validates that dh->length < BN_num_bits(p) (upper bound) but enforces no lower bound. An application or attacker who can influence dh->length (e.g., via configuration, parameter injection in protocols accepting custom DH params) can set it to an arbitrarily small value (e.g., 1 or 8). With BN_RAND_TOP_ONE, dh->length=1 always produces priv_key=1, making the private key trivially guessable. Even dh->length=64 yields only ~63 bits of entropy, far below acceptable security levels. Compare with the q!=NULL path which uses ossl_ffc_generate_private…

*PoC sketch:* DH *dh = DH_new(); BIGNUM *p = BN_new(), *g = BN_new(); BN_set_word(p, <large_prime>); BN_set_word(g, 2); DH_set0_pqg(dh, p, NULL, g); dh->length = 1; /* No minimum check! */ DH_generate_key(dh); /* priv_key will always be 1 */

### `openssl/extensions.c [tls_parse_extension+tls_parse_all_extensions A]` — L785 — conf=0.82
**Heap Buffer Over-Read/Over-Write via Custom Extension Index Out-of-Bounds**

In `tls_parse_all_extensions`, `numexts` is computed as `OSSL_NELEM(ext_defs) + s->cert->custext.meths_count`, and the loop iterates from 0 to `numexts - 1`, passing each index to `tls_parse_extension`. However, `tls_parse_extension` unconditionally accesses `exts[idx]` at line 742 (`RAW_EXTENSION *currext = &exts[idx]`) before any bounds check. The `exts` array is typically allocated with only `OSSL_NELEM(ext_defs)` entries (for built-in extensions). When `idx >= OSSL_NELEM(ext_defs)` (i.e., for custom extension indices), this results in an out-of-bounds heap access. The consequences are: (1)…

*PoC sketch:* 1. Configure an OpenSSL server (or client) with one or more custom TLS extensions registered via SSL_CTX_add_custom_ext() (setting meths_count > 0). 2. Initiate any TLS handshake with this endpoint. 3. During extension parsing, tls_parse_all_extensions will iterate beyond OSSL_NELEM(ext_defs), causi…

### `openssl/ssl3_cbc.c [ssl3_cbc_digest_record B: MAC blocks]` — L278 — conf=0.8
**Missing bounds validation on data_plus_mac_plus_padding_size before arithmetic**

The function performs multiple arithmetic operations on `data_plus_mac_plus_padding_size` without validating that it is large enough to prevent underflows in downstream calculations. Specifically, there is no check that `data_plus_mac_plus_padding_size + header_length >= md_size + 1` before computing `max_mac_bytes`. This is a defense-in-depth issue: while callers may validate record sizes, the function itself is vulnerable to any regression in caller validation. This is especially dangerous for SHA-384/SHA-512 where md_size (48/64) can exceed the minimum possible decrypted record size.

*PoC sketch:* Call ssl3_cbc_digest_record with SHA-512 (md_size=64, md_block_size=128) and data_plus_mac_plus_padding_size=16 (minimum single AES block). No validation prevents the subsequent underflow in max_mac_bytes = (16+13) - 64 - 1.

### `openssl/ssl3_cbc.c [ssl3_cbc_digest_record A: HMAC setup]` — L6 — conf=0.75
**Missing bounds check on mac_secret_length allows hmac_pad buffer overflow**

The parameter `mac_secret_length` is accepted without any validation against `MAX_HASH_BLOCK_SIZE` (the size of the `hmac_pad` buffer on line 35). In subsequent code (not shown but implied by the HMAC setup pattern), the MAC secret is copied/XORed into `hmac_pad`. If `mac_secret_length > MAX_HASH_BLOCK_SIZE`, this results in a stack buffer overflow. While TLS cipher suites normally constrain the MAC secret length, a malicious or misconfigured caller could pass an oversized value, leading to stack corruption and potential code execution.

*PoC sketch:* Call ssl3_cbc_digest_record with mac_secret_length set to a value greater than MAX_HASH_BLOCK_SIZE (e.g., 256 when MAX_HASH_BLOCK_SIZE is 128). The subsequent memcpy/memset of mac_secret into hmac_pad will write past the end of the stack-allocated buffer, corrupting the return address or other stack…

### `openssl/ssl3_cbc.c [ssl3_cbc_digest_record B: MAC blocks]` — L278 — conf=0.75
**Integer underflow in max_mac_bytes calculation**

The calculation `max_mac_bytes = len - md_size - 1` can underflow when `len < md_size + 1`. Since all variables are unsigned, this wraps to a very large value (~4 billion for 32-bit). This occurs when `data_plus_mac_plus_padding_size` is small relative to the MAC algorithm's digest size — e.g., with SHA-512 (md_size=64), if data_plus_mac_plus_padding_size < 52, then len = data_plus_mac_plus_padding_size + 13 < 65, causing underflow. The resulting huge max_mac_bytes propagates into `num_blocks`, potentially causing massive buffer overreads in subsequent block-processing loops. A remote attacker…

*PoC sketch:* Send a TLS 1.0-1.2 CBC record with SHA-512 cipher suite where the encrypted payload decrypts to fewer than 52 bytes (e.g., a single AES block = 16 bytes plaintext). This gives data_plus_mac_plus_padding_size=16, len=29, max_mac_bytes=29-64-1 which underflows to 0xFFFFFFCC (4294967244). The subsequen…

### `openssl/tasn_dec.c [asn1_item_embed_d2i D: EXPLICIT+primitive]` — L530 — conf=0.72
**Integer underflow in EXPLICIT tag length tracking (len -= p - q)**

In asn1_template_ex_d2i, after calling asn1_template_noexp_d2i to decode the inner content of an EXPLICIT tag, the remaining length is computed as `len -= p - q`. There is no validation that `p - q <= len` before this subtraction. If asn1_template_noexp_d2i consumes more bytes than the EXPLICIT tag's declared content length (e.g., due to an inner primitive type whose own length field exceeds the outer boundary), `len` underflows to a negative value. In the NDEF/indefinite-length encoding path (exp_eoc == true), this negative `len` is passed directly to asn1_check_eoc(&p, len), which may interp…

*PoC sketch:* Craft an ASN.1 structure with an EXPLICIT constructed tag wrapping a primitive type (e.g., OCTET STRING) where the inner type's length field exceeds the outer EXPLICIT tag's content length. For NDEF encoding: [EXPLICIT_TAG|0x80] [INNER_TAG|0x84|0x00010000] [DATA...]. The inner length (65536) exceeds…

### `openssl/statem_lib.c [tls_get_message_body A: message body read]` — L1657 — conf=0.55
**Integer underflow in message body read size computation leading to heap buffer overflow**

The computation `n = s->s3.tmp.message_size - s->init_num` can underflow if `s->init_num` exceeds `s->s3.tmp.message_size`. Since `n` is `size_t` (unsigned), the underflow produces a near-maximum value, causing the while-loop to attempt reading a massive amount of data into `&p[s->init_num]`, resulting in a heap buffer overflow. This is compounded by a signed/unsigned type mismatch: `s->init_num` is `int` while `s->s3.tmp.message_size` is likely `size_t`/`unsigned int`. If `init_num` is negative (e.g., from overflow in `init_num += readbytes` at line 1667), implicit conversion to unsigned prod…

*PoC sketch:* Triggering requires s->init_num > s->s3.tmp.message_size when tls_get_message_body is entered. This can arise if: (1) the TLS state machine calls tls_get_message_body with a stale init_num from a prior partial read where message_size was subsequently reduced for a new message; (2) a CCS message path…


## 🟡 MEDIUM (21)

### `openssl/dh_key.c [DH_compute_key+DH_compute_key_padded (CVE-2023-5678)]` — L72 — conf=0.95
**Missing constant-time flag on DH private key when DH_FLAG_CACHE_MONT_P is not set (CVE-2023-5678)**

The BN_FLG_CONSTTIME flag is only set on dh->priv_key inside the conditional block guarded by `if (dh->flags & DH_FLAG_CACHE_MONT_P)`. When this flag is not set (e.g., when using certain DH method implementations that don't set DH_FLAG_CACHE_MONT_P), the private key exponent is NOT marked for constant-time computation. The subsequent bn_mod_exp call on line 80 will then use a non-constant-time modular exponentiation algorithm, which leaks timing information about the private key through a side-channel. An attacker performing a local timing side-channel attack could recover the DH private key b…

*PoC sketch:* 1. Create a DH object using a method that does NOT set DH_FLAG_CACHE_MONT_P (e.g., a custom DH_METHOD or certain engine-backed methods). 2. Perform DH key exchange with a victim. 3. Measure the time taken for DH_compute_key / ossl_dh_compute_key across many invocations with varying pub_key values. 4…

### `openssl/dh_key.c [DH_compute_key body+public key validation]` — L127 — conf=0.95
**Timing Side-Channel Leak in DH_compute_key via Key-Dependent memmove/memset**

DH_compute_key strips leading zero bytes from the shared secret using memmove and memset with key-dependent lengths (npad and ret). While the leading-zero counting loop uses a constant-time mask pattern to touch all bytes, the subsequent memmove(key, key + npad, ret) and memset(key + ret, 0, npad) have execution times proportional to npad/ret, which depend on the shared secret value. This leaks the number of leading zero bytes in the shared secret through timing, reducing the effective entropy of the DH key exchange. This is the same class of vulnerability exploited in the Raccoon attack (CVE-…

*PoC sketch:* An attacker can measure the execution time of DH_compute_key across multiple handshakes. When the shared secret has many leading zero bytes (small value), npad is large and ret is small, making memmove fast and memset large. When the shared secret is near full-size, npad is small and ret is large, m…

### `openssl/a_int.c [ASN1_INTEGER_set+bn conversions]` — L575 — conf=0.85
**ASN1_ENUMERATED_get returns magic value 0xffffffffL for oversized input**

When the ASN1_ENUMERATED length exceeds sizeof(long), the function returns the magic value 0xffffffffL instead of an error indicator. On 64-bit systems, this value (4294967295) is a valid positive long and is indistinguishable from a legitimate return value. An attacker who controls ASN1 input can craft an oversized ASN1_ENUMERATED to inject this value into callers that use it for security-relevant decisions (e.g., permission levels, counters, identifiers). On 32-bit systems, 0xffffffffL equals -1, colliding with the error return value, making error handling impossible.

*PoC sketch:* Craft an ASN1 ENUMERATED with length > sizeof(long) (e.g., 9+ bytes on 64-bit, 5+ bytes on 32-bit). The function will return 0xffffffffL (4294967295 on 64-bit) instead of signaling an error. A caller like: long perm = ASN1_ENUMERATED_get(a); if (perm >= 0) grant_access(perm); would grant access with…

### `openssl/dh_key.c [dh_bn_mod_exp+DH_generate_key entry]` — L233 — conf=0.75
**Const Cast Violation Enables Unsynchronized Mutation of Shared DH Object**

In `ossl_dh_generate_public_key`, the parameter `dh` is declared `const DH *`, implying the function will not modify it. However, the code explicitly casts away const to obtain a mutable pointer to `dh->method_mont_p` and potentially mutates it via `BN_MONT_CTX_set_locked`. While the locked setter provides internal synchronization for the Montgomery context itself, any caller or concurrent thread that reads `dh->method_mont_p` directly (without going through `dh->lock`) will have a data race. The developer comment 'it should be fine...' with trailing ellipsis acknowledges uncertainty. If the D…

*PoC sketch:* 1. Create a DH object and share it as `const DH *` across two threads. 2. Thread A calls `ossl_dh_generate_public_key` which writes to `dh->method_mont_p` via the const cast. 3. Thread B directly reads `dh->method_mont_p` without acquiring `dh->lock` (believing the object is const/immutable). 4. Thr…

### `openssl/extensions.c [tls_parse_extension+tls_parse_all_extensions A]` — L742 — conf=0.75
**Unconditional Out-of-Bounds Access in tls_parse_extension Before Index Validation**

In `tls_parse_extension`, the pointer `currext = &exts[idx]` is computed and dereferenced (reading `currext->present`, `currext->parsed`, and writing `currext->parsed = 1`) BEFORE the check `if (idx < OSSL_NELEM(ext_defs))` at line 755. This means that for any caller passing an idx >= the size of the exts array, out-of-bounds memory is accessed unconditionally. The bounds check comes too late — the damage (read and conditional write) has already occurred. This is a defense-in-depth failure: the index should be validated before any array access, or the exts array should be guaranteed to be larg…

*PoC sketch:* Call tls_parse_extension with idx = OSSL_NELEM(ext_defs) (or larger) and an exts array of size OSSL_NELEM(ext_defs). The access to exts[idx] occurs before the idx < OSSL_NELEM(ext_defs) check, resulting in an immediate out-of-bounds read of the present and parsed fields, and a conditional out-of-bou…

### `openssl/tasn_dec.c [asn1_item_embed_d2i C: template+length]` — L393 — conf=0.7
**Incomplete structure accepted on early loop termination without mandatory field validation**

The field-decoding for-loop can terminate early via `break` when `!len` (line 393) or when an EOC is encountered (line 398). When this happens, remaining template fields are never processed, leaving them at their default/NULL initialized values. There is no post-loop validation that all mandatory (non-OPTIONAL) fields were successfully decoded. An attacker can craft a truncated SEQUENCE that causes the loop to exit before reaching mandatory fields, resulting in an incomplete but apparently valid structure being returned to the caller. If the caller does not independently verify the presence of…

*PoC sketch:* Construct a SEQUENCE containing an ASN.1 structure with two mandatory fields (e.g., a version INTEGER and a signature OCTET STRING). Encode only the version field, then set the SEQUENCE length to cover just that one field. The parser will decode the version, hit `!len`, break out of the loop, and re…

### `openssl/statem_lib.c [tls_get_message_header: handshake header parse]` — L1600 — conf=0.7
**Missing excessive message size validation in SSLv2 backward-compatible ClientHello path**

The normal TLS handshake path validates that the parsed message length does not exceed INT_MAX - SSL3_HM_HEADER_LENGTH (line ~1615: `if (l > (INT_MAX - SSL3_HM_HEADER_LENGTH))`), but the SSLv2 backward-compatible ClientHello path computes `l = s->rlayer.tlsrecs[0].length + SSL3_HM_HEADER_LENGTH` without any equivalent bounds check. If the record layer length field is manipulated or a future bug allows an unexpectedly large value, this missing validation could cause an integer overflow when BUF_MEM_grow is later called with `message_size + SSL3_HM_HEADER_LENGTH` (since BUF_MEM_grow takes an int…

*PoC sketch:* Craft an SSLv2-compatible ClientHello record where the record-layer length field is set to a value near INT_MAX (e.g., 0x7FFFFFFB). If the record layer does not independently enforce a reasonable maximum, the computed `l` would be 0x7FFFFFFB + 4 = 0x7FFFFFFF (INT_MAX), and subsequent buffer growth i…

### `openssl/tasn_dec.c [asn1_item_embed_d2i A: SEQUENCE decode]` — L10 — conf=0.65
**Use-after-free via unexpected free on optional-field-absent return (-1)**

In `asn1_item_ex_d2i_intern`, when `asn1_item_embed_d2i` returns -1 (indicating an OPTIONAL field is not present), the condition `rv <= 0` is true, causing `ASN1_item_ex_free(pval, it)` to be called. A caller that interprets a -1 return as 'nothing was decoded, *pval is unchanged' may continue using *pval, resulting in a use-after-free. The -1 return code has distinct semantics from a hard error (0), but both trigger the same cleanup path. This is especially dangerous for callers of `ASN1_item_ex_d2i` who pass opt=1 and expect *pval to remain valid on -1.

*PoC sketch:* 1. Allocate a valid ASN1_VALUE and set *pval to point to it. 2. Call ASN1_item_ex_d2i(pval, in, len, it, tag, aclass, 1, ctx) with input that causes the optional field to be absent. 3. Function returns -1, but *pval has been freed by ASN1_item_ex_free. 4. Caller checks rv == -1, assumes *pval is sti…

### `openssl/ssl3_cbc.c [ssl3_cbc_digest_record B: MAC blocks]` — L283 — conf=0.65
**Integer overflow in num_blocks intermediate addition**

The calculation `(max_mac_bytes + 1 + md_length_size + md_block_size - 1) / md_block_size` involves an intermediate sum that can overflow when max_mac_bytes is large (e.g., due to the underflow in finding #1). For example, with max_mac_bytes=0xFFFFFFCC, the sum 0xFFFFFFCC + 1 + 16 + 128 - 1 = 0x10000005C overflows to 0x5C for 32-bit unsigned, yielding num_blocks = 0x5C/128 = 0. This causes zero blocks to be processed, producing an incorrect MAC that could theoretically bypass verification under specific overflow conditions.

*PoC sketch:* With SHA-512 and data_plus_mac_plus_padding_size=40: len=53, max_mac_bytes underflows to 4294967284. Intermediate sum = 4294967284 + 144 = 4294967428, which overflows 32-bit to 132. num_blocks = 132/128 = 1. This small num_blocks causes processing of only 1 block when the actual data spans fewer byt…

### `openssl/tasn_dec.c [asn1_d2i_ex_primitive: DER primitive decoder]` — L783 — conf=0.6
**Potential integer overflow in BUF_MEM_grow_clean size calculation**

In the constructed string path, after `asn1_collect` populates `buf`, the code calls `BUF_MEM_grow_clean(&buf, len + 1)` to append a null terminator. The variable `len` is `long` and equals `buf.length`. If `len` equals `LONG_MAX`, then `len + 1` overflows (signed integer overflow is undefined behavior in C). On typical implementations this wraps to `LONG_MIN`, which when cast to `size_t` becomes an astronomically large value, causing `BUF_MEM_grow_clean` to fail. While this primarily results in a denial of service (the function returns an error), the signed integer overflow itself is undefine…

*PoC sketch:* Provide a crafted DER input with a constructed string type where `asn1_collect` accumulates content such that `buf.length` is very close to `LONG_MAX` or `SIZE_MAX`. The `len + 1` computation overflows. On a system where allocation of nearly `SIZE_MAX` bytes fails gracefully, this is a DoS. On a sys…

### `openssl/ssl3_cbc.c [ssl3_cbc_digest_record B: MAC blocks]` — L278 — conf=0.6
**Integer overflow in len calculation from unvalidated data_plus_mac_plus_padding_size**

The calculation `len = data_plus_mac_plus_padding_size + header_length` can overflow if `data_plus_mac_plus_padding_size` is close to UINT_MAX. Since `len` is unsigned, overflow wraps to a small value, which then causes the `max_mac_bytes` underflow described above. While TLS record size limits (2^14+2048=18432) normally prevent this, the function itself performs no validation, making it fragile against future caller regressions or alternative code paths.

*PoC sketch:* If data_plus_mac_plus_padding_size = 0xFFFFFFF3 (UINT_MAX - 12) and header_length = 13, then len = 0xFFFFFFF3 + 13 = 0x00000000 (overflow to 0). Then max_mac_bytes = 0 - md_size - 1 underflows severely.

### `openssl/extensions.c [tls_parse_all_extensions B: context check loop]` — L950 — conf=0.6
**Server-side renegotiation info check skipped during initial handshake**

In final_renegotiate(), the server-side check for the Renegotiation Info (RI) extension is gated on 's->renegotiate' being true. During the initial handshake, s->renegotiate is 0, so the server accepts connections from clients that do not advertise safe renegotiation support (no RI extension). This allows an attacker to establish a session without RI, and if the server later performs renegotiation, the absence of the RI extension in the original handshake means the server cannot verify the renegotiation is bound to the original session. This weakens protection against CVE-2009-3555 (renegotiat…

*PoC sketch:* 1. Connect to an OpenSSL server with a client that does not send the renegotiation_info extension (e.g., a very old TLS client or a modified client). 2. The server accepts the connection because s->renegotiate is false during the initial handshake. 3. An attacker positioned as a MITM can inject data…

### `openssl/tasn_dec.c [asn1_item_embed_d2i C: template+length]` — L352 — conf=0.55
**ASN1_AFLG_BROKEN length recalculation potential integer underflow**

When the ASN1_AFLG_BROKEN flag is set, `len` is recalculated as `len = tmplen - (p - *in)`. If `p` has advanced beyond `*in + tmplen` due to a malformed or adversarially crafted tag/length encoding in `asn1_check_tlen`, this subtraction underflows. Since `len` is typically a signed `long`, a negative value would bypass the `!len` check (negative != 0) and be passed to sub-decoders, potentially causing them to interpret a huge positive length (if cast to unsigned) or leading to buffer over-reads. Additionally, `seq_nolen = 1` disables all subsequent length validation for this SEQUENCE, compound…

*PoC sketch:* Target an ASN.1 item with ASN1_AFLG_BROKEN set (e.g., certain PKCS#7 or CMS structures). Craft the input so that the SEQUENCE tag+length header consumes more bytes than `tmplen` allows (e.g., using a multi-byte length encoding that consumes extra bytes). This makes `p - *in > tmplen`, causing `len` …

### `openssl/tasn_dec.c [asn1_item_embed_d2i D: EXPLICIT+primitive]` — L527 — conf=0.55
**Missing ret == -1 check after asn1_template_noexp_d2i in EXPLICIT path**

After calling asn1_template_noexp_d2i(val, &p, len, tt, 0, ...), only the `ret == 0` case is checked. The function can also return -1 (indicating an optional field was not found). While opt=0 makes this unlikely, certain template configurations or ADB (discriminated) types could cause asn1_template_noexp_d2i to return -1 through internal recursive calls. If -1 is returned, execution falls through to `len -= p - q` with p potentially not properly advanced, leading to incorrect length arithmetic and subsequent processing of stale/uninitialized pointer positions. Compare with the asn1_check_tlen …

*PoC sketch:* Construct an ASN.1 structure with an EXPLICIT tag wrapping an ADB (ASN.1 Discriminated By) type where the discriminator value causes the inner template resolution to fail in a way that returns -1 from asn1_template_noexp_d2i. This would bypass the `!ret` check and proceed with incorrect len computat…

### `openssl/dh_key.c [dh_bn_mod_exp+DH_generate_key entry]` — L268 — conf=0.55
**Potential Timing Side-Channel in generate_key Missing BN_FLG_CONSTTIME**

The function `ossl_dh_generate_public_key` explicitly wraps the private key with `BN_with_flags(prk, priv_key, BN_FLG_CONSTTIME)` before calling `bn_mod_exp`, ensuring constant-time modular exponentiation to prevent timing side-channels. However, the `generate_key` function (which is the primary entry point via `DH_generate_key`) does not visibly apply `BN_FLG_CONSTTIME` to the private key before performing modular exponentiation in the shown code. If the full `generate_key` implementation calls `bn_mod_exp` without the constant-time flag, the modular exponentiation `g^priv_key mod p` could le…

*PoC sketch:* 1. Generate a DH key using DH_generate_key() which calls generate_key(). 2. Measure the time taken by the modular exponentiation for multiple private keys of varying Hamming weight. 3. Correlate timing with private key bit patterns to recover key material. 4. Compare with ossl_dh_generate_public_key…

### `openssl/extensions.c [tls_parse_all_extensions B: context check loop]` — L897 — conf=0.55
**Potential out-of-bounds write on extflags array via extension construction loop**

The loop iterates over all elements of ext_defs using index i (0 to OSSL_NELEM(ext_defs)-1) and writes to s->ext.extflags[i]. If the extflags array is not guaranteed to have at least OSSL_NELEM(ext_defs) elements—e.g., due to conditional compilation adding extensions to ext_defs without correspondingly resizing extflags—this results in an out-of-bounds OR-write (|= SSL_EXT_FLAG_SENT). This could corrupt adjacent heap memory, potentially leading to code execution or denial of service. The risk is elevated when new extensions are added to ext_defs in future OpenSSL versions without updating the …

*PoC sketch:* 1. Build OpenSSL with a configuration that adds an entry to ext_defs (e.g., via a custom extension definition or conditional compilation) without updating the extflags array size in the SSL_CONNECTION structure. 2. Trigger a TLS handshake where the client or server constructs extensions in the SSL_E…

### `openssl/extensions.c [tls_collect_extensions+tls_check_client_hello]` — L23 — conf=0.55
**Potential use-after-free if session hostname aliases SSL hostname during renegotiation**

The code frees s->session->ext.hostname and then immediately reads s->ext.hostname via OPENSSL_strdup. If s->session->ext.hostname and s->ext.hostname point to the same allocation (pointer aliasing), the OPENSSL_free would invalidate both pointers, and the subsequent OPENSSL_strdup(s->ext.hostname) would read freed memory. While in a typical fresh handshake these are separate allocations, during certain renegotiation or session reuse scenarios where the SSL object's ext.hostname was set to reference the session's hostname string (rather than a copy), this aliasing could occur. The !s->hit guar…

*PoC sketch:* 1. Establish a TLS connection where s->ext.hostname is set to point to the same allocation as s->session->ext.hostname (possible through specific session reuse or renegotiation code paths where the pointer is shared rather than copied). 2. Trigger a renegotiation where the SNI callback returns SSL_T…

### `openssl/ssl3_cbc.c [CBC padding+Lucky13 header]` — L118 — conf=0.5
**Incomplete ssl3_cbc_digest_record — Lucky13 mitigation logic not auditable**

The file is truncated at the signature of ssl3_cbc_digest_record, the core function responsible for constant-time MAC verification of CBC-padded TLS records (Lucky13 mitigation). The 'data_size' parameter is documented as 'secret' — its value must not leak through timing. Without the function body, the constant-time guarantees cannot be verified. Historical CVEs (CVE-2013-0169, CVE-2016-2107) demonstrate that subtle timing leaks in this exact function enable padding oracle attacks leading to plaintext recovery. The truncated code prevents assessment of whether the mitigation is correctly imple…

*PoC sketch:* N/A — function body is not present in the provided file. A complete audit of ssl3_cbc_digest_record is required to verify constant-time handling of the secret data_size parameter across all padding lengths and hash algorithms.

### `openssl/ssl3_cbc.c [ssl3_cbc_digest_record A: HMAC setup]` — L27 — conf=0.5
**Misleading 'at most 18 bits' comment on bits variable suggests latent integer truncation**

The variable `bits` is declared as `size_t` with a comment stating 'at most 18 bits'. However, the only guard on input size is `data_plus_mac_plus_padding_size < 1024*1024` (~1MB). Converting to bits: 1MB * 8 = ~8M bits, requiring 23 bits to represent. This exceeds the documented 18-bit maximum. If downstream code assumes `bits` fits in 18 bits (e.g., when encoding into `length_bytes[MAX_HASH_BIT_COUNT_BYTES]`), truncation could produce an incorrect MAC, enabling forgery attacks against the CBC MAC verification.

*PoC sketch:* Send a TLS record with data_plus_mac_plus_padding_size close to 1MB (e.g., 900000 bytes). The computed bits value (~7200000) exceeds 2^18=262144. If the length encoding truncates to 18 bits, the MAC will be computed over an incorrect length, potentially allowing a padding oracle or MAC forgery.

### `openssl/extensions.c [tls_collect_extensions+tls_check_client_hello]` — L48 — conf=0.45
**Inconsistent session state when ticket option is toggled by SNI callback without session object**

When the SNI callback disables tickets (SSL_OP_NO_TICKET) after they were previously enabled, and this is not a session resumption (!s->hit), the code calls SSL_get_session(ssl) to obtain the session object. If SSL_get_session returns NULL (which can happen in edge cases during early handshake phases), the code calls SSLfatal and returns 0. However, the ticket-expected flag has already been cleared (s->ext.ticket_expected = 0) before the NULL check. If the session object exists but is in an inconsistent state (e.g., partially initialized), the code proceeds to free the ticket and generate a ne…

*PoC sketch:* 1. Configure server with SNI callback that switches SSL_CTX (changing ticket options from enabled to disabled). 2. Send ClientHello with SNI and session ticket extension. 3. The callback switches context, disabling tickets. 4. Code clears ticket_expected and generates new session ID under the new co…

### `openssl/statem_lib.c [tls_get_message_body A: message body read]` — L1652 — conf=0.4
**Signed-to-unsigned cast of init_num in CCS path enables length confusion and OOB read**

At line 1652, `*len = (unsigned long)s->init_num` casts the signed `int` init_num to `unsigned long` for the CCS early-return path. If init_num were negative (achievable via integer overflow from accumulated `init_num += readbytes` operations), this cast produces a value near ULONG_MAX. The caller interprets *len as the message body size and uses it to index into buffers, leading to out-of-bounds reads in downstream processing. The CCS path also skips all MAC computation, meaning any length confusion here bypasses integrity verification for subsequent handshake messages.

*PoC sketch:* If prior loop iterations cause init_num to overflow past INT_MAX (possible if n underflows and ssl_read_bytes returns large readbytes), init_num wraps negative. On a CCS message, *len receives (unsigned long)(negative_int) ≈ 2^63-1 on 64-bit. Downstream code using *len to iterate over the message bo…


## 🔵 LOW (19)

### `openssl/a_int.c [ASN1_INTEGER_set+bn conversions]` — L540 — conf=0.9
**Ambiguous error return in ASN1_INTEGER_get and ASN1_ENUMERATED_get**

Both ASN1_INTEGER_get() and ASN1_ENUMERATED_get() return -1 to indicate errors (overflow, parse failure), but -1 is also a valid legitimate return value. Callers cannot distinguish between an actual value of -1 and an error condition. This could cause callers to misinterpret error conditions as valid negative values, potentially leading to incorrect security decisions (e.g., treating a malformed certificate field as a valid negative value).

*PoC sketch:* Provide a valid ASN1_INTEGER encoding the value -1. The function returns -1. Now provide a malformed or overflowing ASN1_INTEGER. The function also returns -1. The caller cannot differentiate: long val = ASN1_INTEGER_get(a); /* val == -1: is this an error or the actual value? */

### `openssl/ssl3_cbc.c [ssl3_cbc_digest_record A: HMAC setup]` — L23 — conf=0.85
**Variable name 'c' collides with union member 'md_state.c' creating confusion risk**

A local variable `c` of type `size_t` is declared on line 23 in the same scope where `md_state.c` (the union member used for hash state) is actively accessed (e.g., line 62: `MD5_Init((MD5_CTX *)md_state.c)`). While C does not technically shadow the member access, this naming collision creates significant programmer confusion and increases the risk of future maintenance bugs where `c` is used when `md_state.c` was intended, or vice versa. A mistaken substitution could leak hash internal state or corrupt the hash computation.

*PoC sketch:* No direct exploit; this is a code quality issue that increases the likelihood of future vulnerabilities during maintenance. A developer might accidentally write `c` instead of `md_state.c` in a later modification, causing the hash state to be read from/written to an incorrect location.

### `openssl/dh_key.c [DH_compute_key+DH_compute_key_padded (CVE-2023-5678)]` — L99 — conf=0.85
**Non-constant-time leading zero byte stripping in DH_compute_key (RFC 5246 padding)**

The DH_compute_key function uses RFC 5246 (8.1.2) padding style which strips leading zero bytes from the shared secret before returning it. As noted in the code comment on line 100-101, this is 'inherently not constant time.' The number of leading zero bytes in the shared secret depends on the secret's value, and the variable-length output creates a timing side channel. An attacker who can measure the processing time or observe the output length can infer information about the shared secret's magnitude. While the code attempts some mitigation with volatile variables (npad, mask), the fundament…

*PoC sketch:* 1. Establish a DH key exchange session using DH_compute_key (not the padded variant). 2. Observe the length of the returned key across multiple exchanges. 3. Shorter keys indicate more leading zero bytes in the shared secret Z. 4. This information narrows the range of possible Z values, aiding a bru…

### `openssl/dh_key.c [generate_key: FIPS validation+BN ops]` — L336 — conf=0.75
**Incorrect quadratic non-residue detection for generator 2 clears secret bit when p%8==1**

The check `!BN_is_bit_set(p, 2)` is true when p%8 ∈ {0,1,2,3}. The code intends to clear bit 0 of the private key only when g=2 is a quadratic non-residue (p%8==3 or p%8==5), since the LSB is then leaked via the Legendre symbol of the public key. However, when p%8==1, g=2 is a quadratic residue, meaning the LSB IS secret — yet the code still clears it, unnecessarily losing 1 bit of private key entropy. Conversely, when p%8==5, g=2 is a QNR and the LSB is leaked, but the code does NOT clear it, leaving a known bit in the key. While 1 bit is negligible in practice, the logic is incorrect for non…

*PoC sketch:* Construct DH params with g=2 and a prime p where p%8==1 (e.g., p where bit 2 is clear but p≡1 mod 8). Generate key via the q=NULL non-FIPS path. The private key's bit 0 is forcibly cleared even though it carries secret entropy, reducing the effective key space by half.

### `openssl/dh_key.c [DH_compute_key body+public key validation]` — L195 — conf=0.7
**Race Condition on Global default_DH_method Pointer**

DH_set_default_method writes to the global pointer default_DH_method without any synchronization, while DH_get_default_method reads it concurrently. In a multi-threaded application, one thread could be reading default_DH_method via DH_new() while another thread is writing to it via DH_set_default_method(), resulting in a torn read (reading a partially-updated pointer). This could cause a thread to use an invalid method table pointer, potentially leading to a crash or code execution if the pointer lands on attacker-controlled memory.

*PoC sketch:* Thread A: while(1) { DH *dh = DH_new(); /* reads default_DH_method */ DH_free(dh); } Thread B: DH_set_default_method(attacker_controlled_method); /* writes default_DH_method without lock */ Under high concurrency, Thread A may read a corrupted pointer value, leading to a crash when dereferencing dh-…

### `openssl/dh_key.c [dh_bn_mod_exp+DH_generate_key entry]` — L219 — conf=0.7
**Missing Synchronization on Global default_DH_method in DH_set_default_method**

The function `DH_set_default_method` assigns to the global variable `default_DH_method` without any locking or atomic operation. If two threads concurrently call this function (e.g., during library initialization or reconfiguration), a data race occurs on the global pointer. A concurrent reader calling `DH_get_default_method` could read a torn/partially-updated pointer, leading to type confusion or use of an invalid method table. This is outside the FIPS module (`#ifndef FIPS_MODULE`), so it affects the default code path.

*PoC sketch:* 1. Thread A: DH_set_default_method(meth_a); 2. Thread B: DH_set_default_method(meth_b);  // concurrent write, no lock 3. Thread C: DH_new() -> internally calls DH_get_default_method() -> may read corrupted pointer 4. Crash or type confusion when using the returned method table.

### `openssl/dh_key.c [generate_key: parameter checks]` — L22 — conf=0.7
**Generic error code overwrites specific error in generate_key error path**

In the generate_key error path, ERR_raise(ERR_LIB_DH, ERR_R_BN_LIB) is always raised regardless of the actual failure cause. If the error originated from ossl_ffc_params_simple_validate (invalid parameters) or ossl_ffc_generate_private_key (key generation failure), the specific error information is obscured by the generic ERR_R_BN_LIB error pushed on top of the error stack. While OpenSSL uses an error stack (so the original error is still accessible), the top-level error is misleading. This could cause callers to misdiagnose the root cause of failures, potentially leading to incorrect handling…

*PoC sketch:* 1. Create a DH object with invalid parameters (e.g., p is not prime, or g is invalid). 2. Call DH_generate_key(). 3. Retrieve the error with ERR_get_error() - the top error will be ERR_R_BN_LIB rather than a more specific parameter validation error. 4. The original error is still on the stack but ma…

### `openssl/dh_key.c [DH_compute_key body+public key validation]` — L148 — conf=0.65
**Timing Side-Channel in DH_compute_key_padded with External compute_key Methods**

DH_compute_key_padded is designed to be constant-time, but only when using the built-in ossl_dh_compute_key (which always returns a full-length key). When an external/custom compute_key method is registered via DH_meth_set_compute_key, the returned rv may vary, making pad = BN_num_bytes(dh->params.p) - rv key-dependent. The conditional branch 'if (pad > 0)' then leaks whether the shared secret had leading zeros. While the code comments acknowledge this ('pad is constant (zero) unless compute_key is external'), applications using custom DH methods may unknowingly introduce a timing side-channel…

*PoC sketch:* Register a custom DH method via DH_meth_new() + DH_meth_set_compute_key() that returns variable-length output (e.g., strips leading zeros internally). Then call DH_compute_key_padded in a TLS server. The 'if (pad > 0)' branch will be taken or not based on the shared secret value, creating a timing d…

### `openssl/dh_key.c [generate_key: FIPS validation+BN ops]` — L350 — conf=0.65
**Non-atomic dirty_cnt increment enables race condition in multi-threaded key generation**

The statement `dh->dirty_cnt++` is not atomic. If two threads concurrently call generate_key on the same DH object, the increment can be lost (write-write race). dirty_cnt is used for cache invalidation in OpenSSL's provider architecture. A lost increment could cause stale cached key material to be used, potentially leading to incorrect cryptographic operations. Additionally, concurrent writes to dh->pub_key and dh->priv_key without synchronization could cause use-after-free in the error path cleanup (`if (pub_key != dh->pub_key) BN_free(pub_key)`).

*PoC sketch:* Thread A and Thread B both call DH_generate_key(dh) simultaneously. Both read the same initial dh->dirty_cnt, both increment to the same value (lost update). Concurrent assignment to dh->pub_key/dh->priv_key may leak the BIGNUM that one thread allocated but the other overwrote, or cause double-free …

### `openssl/statem_lib.c [tls_get_message_header: handshake header parse]` — L1555 — conf=0.65
**Silent error return on CCS in stateless mode — no alert sent to peer**

When a ChangeCipherSpec record is received while in TLS_ST_BEFORE state with the TLS1_FLAGS_STATELESS flag set, the function returns 0 (indicating failure/need-more-data) without calling SSLfatal() and without sending any TLS alert to the peer. The return value 0 is ambiguous — callers may interpret it as 'retry later' rather than a fatal error, since s->rwstate is not set to SSL_READING (unlike the i<=0 error path at line 1543). This can cause the state machine to re-enter the read loop without proper error propagation, potentially leading to protocol desynchronization between client and serv…

*PoC sketch:* During a TLS 1.3 stateless handshake (server has TLS1_FLAGS_STATELESS set, hand_state == TLS_ST_BEFORE), send a ChangeCipherSpec record between the first and second ClientHello. The server will return 0 from tls_get_message_header without sending an alert, causing the state machine to potentially lo…

### `openssl/tasn_dec.c [asn1_item_embed_d2i D: EXPLICIT+primitive]` — L543 — conf=0.6
**No cleanup of partially decoded value on error in asn1_template_ex_d2i**

When asn1_template_ex_d2i hits the err label after asn1_template_noexp_d2i has successfully allocated and partially populated *val, the function returns 0 without freeing the partially decoded value. Unlike asn1_item_embed_d2i which has structured error cleanup, this function's err path simply returns 0. If the caller does not properly free *val on error (which is a common pattern in ASN.1 decoding), this leads to a memory leak. More critically, if the caller reuses the partially-populated *val thinking it's in a valid state, it could lead to type confusion or use-of-uninitialized-field issues…

*PoC sketch:* Send a malformed EXPLICIT-tagged structure where the inner content decodes successfully but the trailing length check fails (e.g., extra trailing bytes inside the EXPLICIT wrapper). The inner value is allocated and populated, but the function returns 0. The caller may not free the partial value.

### `openssl/ssl3_cbc.c [CBC padding+Lucky13 header]` — L63 — conf=0.6
**Missing bounds validation on md_out in hash state serialization functions**

The tls1_md5_final_raw, tls1_sha1_final_raw, tls1_sha256_final_raw, and tls1_sha512_final_raw functions write fixed-size output (16, 20, 32, and 64 bytes respectively) to md_out without any size parameter or bounds check. If a caller provides an undersized buffer, a buffer overflow occurs. While these are static internal functions, a future caller could inadvertently pass a buffer smaller than required (e.g., allocating based on a smaller hash's output size when switching algorithms). The risk is elevated because these functions are part of the Lucky13 constant-time MAC mitigation path — a cor…

*PoC sketch:* If a caller allocates md_out as unsigned char md_out[20] (sized for SHA-1) but invokes tls1_sha512_final_raw (which writes 64 bytes), a 44-byte stack/heap overflow occurs. No such misuse exists in the current truncated file, but the API design invites it.

### `openssl/tasn_dec.c [asn1_item_embed_d2i A: SEQUENCE decode]` — L57 — conf=0.5
**Missing NULL validation for 'in' parameter before dereference**

The `asn1_item_embed_d2i` function validates `pval` and `it` for NULL, and checks `len <= 0`, but does not validate that `in` (the pointer to the input buffer pointer) is non-NULL. If `in` is NULL and `len > 0`, subsequent dereference of `*in` or `in` in the primitive/template decode paths would cause a NULL pointer dereference crash. While this requires a caller programming error, the function validates other parameters but omits this one, creating an inconsistent defensive posture.

*PoC sketch:* Call asn1_item_embed_d2i(pval, NULL, 10, it, -1, 0, 0, ctx, 0, NULL, NULL) with a valid it and pval. The len > 0 check passes, but subsequent code dereferencing *in causes a segfault.

### `openssl/tasn_dec.c [asn1_d2i_ex_primitive: DER primitive decoder]` — L748 — conf=0.5
**Missing constructed-flag validation for V_ASN1_OTHER allows primitive encoding of context-specific types**

When `utype == V_ASN1_OTHER`, the code clears the TLC cache but does NOT validate the `cst` (constructed) flag. For SEQUENCE and SET, the code enforces `cst == 1` (must be constructed). However, for OTHER types (context-specific or application-specific tags), both primitive and constructed encodings are accepted without validation. If downstream consumers assume OTHER types are always constructed (as SEQUENCE/SET are enforced to be), this could lead to type confusion where primitive-encoded data is misinterpreted as constructed, or vice versa. While the data is left in encoded form for OTHER t…

*PoC sketch:* Encode a context-specific tagged value [3] (IMPLICIT) as primitive (e.g., context-specific tag 3 with primitive encoding containing arbitrary bytes). The decoder will accept this without error, whereas a strict decoder might reject it if the underlying type is known to be constructed. This could be …

### `openssl/dh_key.c [DH_compute_key body+public key validation]` — L219 — conf=0.5
**Const-Correctness Violation Enables Unsynchronized Montgomery Context Mutation**

In ossl_dh_generate_public_key, the const DH *dh parameter is cast away to allow mutation of dh->method_mont_p via BN_MONT_CTX_set_locked. While BN_MONT_CTX_set_locked uses dh->lock for synchronization during creation, the resulting mont pointer is stored in dh->method_mont_p and subsequently read by other threads without locking (e.g., in dh_bn_mod_exp). If the Montgomery context is being lazily initialized by one thread while another thread reads the partially-initialized pointer, a use-before-initialization race could occur. The const cast subverts compiler protections that would normally c…

*PoC sketch:* Thread A calls ossl_dh_generate_public_key(dh) for the first time, entering BN_MONT_CTX_set_locked to create the Montgomery context. Thread B simultaneously calls a function that reads dh->method_mont_p (e.g., via dh_bn_mod_exp). If Thread B reads the pointer after it's been set but before the Montg…

### `openssl/extensions.c [tls_parse_all_extensions B: context check loop]` — L967 — conf=0.5
**Silent statistics counter leak when ssl_tsan_lock fails in ssl_tsan_decr**

In ssl_tsan_decr(), if ssl_tsan_lock(ctx) fails (returns 0), the tsan_decr(stat) call is silently skipped. This means the statistics counter is never decremented, causing a permanent counter leak. Under sustained concurrent access or resource exhaustion conditions, an attacker could trigger repeated lock failures, causing the counter to monotonically increase without bound. If the counter is used for resource accounting or access control decisions, this could lead to denial of service or incorrect security decisions. The TSAN_QUALIFIER annotation suggests this code path is specifically for thr…

*PoC sketch:* 1. Create a scenario with high concurrency where many threads call ssl_tsan_decr simultaneously on the same SSL_CTX. 2. If the lock implementation has a timeout or failure mode (e.g., pthread_mutex_lock returning EAGAIN), some decrements will be silently dropped. 3. Monitor the statistics counter—it…

### `openssl/extensions.c [tls_parse_all_extensions B: context check loop]` — L975 — conf=0.45
**Client-side hostname not freed in init_server_name on non-server context**

In init_server_name(), s->ext.hostname is only freed when s->server is true. On the client side, the hostname from a previous connection attempt persists in memory across handshake retries or session resumption. If the SSL_CONNECTION object is reused (e.g., via SSL_clear), the stale hostname remains allocated and could be leaked to a different server if the client connects to a new target without explicitly setting a new hostname. This is an information disclosure risk where the SNI from a previous connection could be sent to a different server.

*PoC sketch:* 1. Create an SSL connection to server A with SNI hostname 'secret-internal.example.com'. 2. Call SSL_clear() on the SSL object to reset for a new connection. 3. Connect to server B without setting a new hostname via SSL_set_tlsext_host_name(). 4. The ClientHello to server B may include the stale SNI…

### `openssl/tasn_dec.c [asn1_item_embed_d2i A: SEQUENCE decode]` — L73 — conf=0.4
**Uninitialized pointer variable 'q' may be used in error paths**

The variable `q` is declared as `const unsigned char *p = NULL, *q;` — while `p` is initialized to NULL, `q` is left uninitialized. In the full function body (partially truncated), if `q` is read before being assigned in certain error or early-exit paths within the SEQUENCE decode logic, this constitutes use of uninitialized memory, which could leak stack data or cause unpredictable behavior.

*PoC sketch:* Craft an ASN.1 SEQUENCE input that triggers an error path in the SEQUENCE decoding section (not fully shown) where 'q' is read before assignment. This would require analyzing the full function to identify such a path, but the declaration pattern is a prerequisite for the bug.

### `openssl/statem_lib.c [tls_get_message_header: handshake header parse]` — L1566 — conf=0.4
**Potential init_num underflow via inconsistent ssl_read_bytes return and readbytes**

In the CCS handling path, `s->init_num = readbytes - 1` is assigned after verifying `readbytes == 1`. However, `readbytes` is of type `size_t` (unsigned). If a future code change or a buggy ssl_read_bytes implementation were to return i>0 with readbytes==0, the check `readbytes != 1` would pass (0 != 1 is true, triggering the error path), so this is currently safe. But the defensive check is fragile — it relies on the OR-short-circuit of `s->init_num != 0 || readbytes != 1 || p[0] != SSL3_MT_CCS`. If init_num==0 and the CCS byte matches, but readbytes were somehow 0 due to a race or implementa…

*PoC sketch:* This is a defensive coding concern rather than an immediately exploitable bug. To trigger, one would need ssl_read_bytes to return a positive i value while setting readbytes=0, which contradicts the function's contract. No practical exploit exists with current OpenSSL code, but the pattern should us…


## ⚪ INFO (5)

### `openssl/ssl3_cbc.c [CBC padding+Lucky13 header]` — L55 — conf=0.7
**u32toLE macro side-effect on pointer parameter enables double-increment misuse**

The u32toLE macro modifies its second argument (p) via post-increment as a side effect: (*((p)++) = ...). If ever invoked with an expression like u32toLE(n, ptr++) or u32toLE(n, md_out++), the pointer would be incremented 5 times instead of the intended 4 (once per write plus the explicit increment), causing a 1-byte offset and subsequent data corruption. Current usage in tls1_md5_final_raw is safe (plain md_out), but the macro's contract is fragile and undocumented.

*PoC sketch:* Hypothetical misuse: unsigned char *p = buf; u32toLE(value, p++); /* p advances by 5, not 4 — next write is off-by-one */

### `openssl/dh_key.c [generate_key: FIPS validation+BN ops]` — L354 — conf=0.7
**Overly generic error masking hides real failure reasons**

On any failure (ok != 1), the code unconditionally raises ERR_R_BN_LIB regardless of the actual cause. Failures from parameter validation (ossl_ffc_params_simple_validate), FIPS policy checks, or length validation are all masked as 'BN library error'. This makes it difficult to diagnose misconfigurations or attack attempts, and could cause downstream code to mishandle the error (e.g., retrying when the real issue is invalid parameters).

*PoC sketch:* Set dh->params.q = NULL in FIPS mode and call DH_generate_key. The failure is due to missing q parameter, but the raised error is ERR_R_BN_LIB, misleading the caller about the root cause.

### `openssl/a_int.c [ASN1_INTEGER_set+bn conversions]` — L607 — conf=0.6
**Unchecked return value of c2i_ibuf in ossl_c2i_uint64_int**

In ossl_c2i_uint64_int(), the second call to c2i_ibuf() has its return value explicitly discarded with (void). If this call fails (returns 0), the code proceeds to call asn1_get_uint64() with potentially uninitialized or partially written 'buf' contents. While the first call validates the input, a TOCTOU race condition or memory corruption between the two calls could cause the second to fail while the first succeeded, leading to use of uninitialized stack memory as a uint64_t value.

*PoC sketch:* In a scenario where the memory at *pp is concurrently modified between the two c2i_ibuf calls (e.g., shared memory mapping), the first call succeeds (buflen is valid), but the second call fails, leaving 'buf' uninitialized. The subsequent asn1_get_uint64(ret, buf, buflen) reads uninitialized stack d…

### `openssl/statem_lib.c [tls_get_message_body A: message body read]` — L1659 — conf=0.5
**Missing bounds validation before buffer write in read loop**

The write target `&p[s->init_num]` in the ssl_read_bytes call has no explicit bounds check against the allocated size of init_buf. The code assumes init_num never exceeds message_size and that init_buf was allocated to hold message_size + SSL3_HM_HEADER_LENGTH bytes. If either assumption is violated (e.g., init_num > message_size due to the integer underflow above, or init_buf undersized due to allocation race), data is written past the heap buffer boundary. A defensive check such as `if (s->init_num >= s->s3.tmp.message_size) break;` or validation against init_buf->length is absent.

*PoC sketch:* Same trigger as the integer underflow finding — once n underflows, the loop continues writing at &p[s->init_num] where init_num grows unbounded past the buffer end. On each iteration, ssl_read_bytes writes up to n bytes (now near SIZE_MAX) starting well past the heap allocation.

### `openssl/ssl3_cbc.c [CBC padding+Lucky13 header]` — L100 — conf=0.4
**LARGEST_DIGEST_CTX may become stale if larger hash contexts are introduced**

LARGEST_DIGEST_CTX is hardcoded to SHA512_CTX and is used elsewhere (not in this file) to allocate stack space for hash context unions. If a future hash algorithm with a larger context structure is added to the _final_raw dispatch table without updating this macro, stack buffer overflows could occur when the larger context is used through the union. This is a maintenance hazard rather than a current vulnerability.

*PoC sketch:* If a new hash with a 256-byte context is added and dispatched through ssl3_cbc_digest_record, but LARGEST_DIGEST_CTX remains SHA512_CTX (~210 bytes), the context would overflow the stack-allocated union buffer.

