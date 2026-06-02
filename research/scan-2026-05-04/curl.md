# curl — GLM-5.1 Pipeline A scan (2026-04-17)

- **Model**: glm-5.1
- **Segments**: 14 (0 hit GLM API timeout)
- **Total findings**: 40
  - 🟠 **HIGH**: 4
  - 🟡 **MEDIUM**: 19
  - 🔵 **LOW**: 14
  - ⚪ **INFO**: 3

## Per-segment summary

| Segment | Time (s) | Findings | Status |
|---|---:|---:|---|
| `lib/urlapi.c [parseurl]` | 360.7 | 0 | ✅ ok |
| `lib/urlapi.c [parse_hostname_login+ipv6+hostname_check]` | 360.5 | 0 | ✅ ok |
| `lib/urlapi.c [curl_url_set+curl_url_get]` | 222.9 | 3 | ✅ ok |
| `lib/url.c [parseurlandfillconn+setup_range]` | 291.6 | 4 | ✅ ok |
| `lib/cookie.c [parse_cookie_header+sanitize+add]` | 261.7 | 4 | ✅ ok |
| `lib/ftp.c [match_pasv_6nums+ftp_state_pasv_resp]` | 160.6 | 3 | ✅ ok |
| `lib/ftp.c [ftp_state_use_port+use_pasv]` | 174.2 | 3 | ✅ ok |
| `lib/socks.c [do_SOCKS4]` | 105.6 | 4 | ✅ ok |
| `lib/socks.c [do_SOCKS5]` | 305.9 | 3 | ✅ ok |
| `lib/http_chunks.c [httpchunk_readwrite]` | 360.4 | 0 | ✅ ok |
| `lib/ftplistparser.c [ftp_pl_state_machine]` | 73.8 | 5 | ✅ ok |
| `lib/mime.c [mime_read+boundary]` | 91.1 | 4 | ✅ ok |
| `lib/http_digest.c [output_digest+decode]` | 146.2 | 3 | ✅ ok |
| `lib/curl_sasl.c [decode_mech+sasl_continue]` | 289.6 | 4 | ✅ ok |

## 🟠 HIGH (4)

### `lib/ftp.c [match_pasv_6nums+ftp_state_pasv_resp]` — L1830 — conf=0.95
**SSRF via Malicious PASV Response IP Address**

When processing a PASV (227) response, the FTP client trusts the IP address returned by the server and uses it to establish the data connection. A malicious or compromised FTP server can supply an arbitrary internal IP address (e.g., 127.0.0.1, 10.x.x.x, 169.254.x.x) in the PASV response, causing the client to connect to an internal service instead of the legitimate FTP data port. This enables Server-Side Request Forgery (SSRF) attacks including internal network scanning, data exfiltration, and interaction with internal services. The `ftp_skip_ip` option exists as a mitigation but is NOT enabl…

*PoC sketch:* 1. Set up a malicious FTP server. 2. When a client sends PASV, respond with:    '227 Entering Passive Mode (127,0,0,1,0,80)' 3. The client will connect to 127.0.0.1:80 for the data channel, sending any upload data to the local HTTP server.    Alternatively: '227 Entering Passive Mode (169,254,169,25…

### `lib/cookie.c [parse_cookie_header+sanitize+add]` — L553 — conf=0.85
**Cookie Prefix Security Requirements Not Enforced (__Secure- and __Host-)**

The code detects __Secure- and __Host- cookie prefixes and sets co->prefix_secure / co->prefix_host flags, but does NOT enforce their mandatory security requirements within this parsing function. Per RFC 6265bis: __Secure- requires the cookie be set over a secure origin, yet prefix_secure is set regardless of the 'secure' parameter. __Host- requires no domain attribute, path="/", and a secure origin, yet prefix_host is set unconditionally and the domain attribute is still allowed to be parsed and stored (lines ~610-640). An attacker on a subdomain can set a cookie like '__Host-session=evil; do…

*PoC sketch:* An HTTP server on sub.example.com sends: Set-Cookie: __Host-session=attacker_value; domain=.example.com; path=/  The parser sets co->prefix_host=TRUE (line 559) and then processes the domain attribute, setting co->domain="example.com" with co->tailmatch=TRUE (line ~635). If downstream enforcement on…

### `lib/ftplistparser.c [ftp_pl_state_machine]` — L193 — conf=0.85
**Out-of-bounds read in ftp_pl_get_permission due to missing length validation**

The function ftp_pl_get_permission() accesses str[0] through str[8] without verifying that the input string is at least 9 characters long. Since FTP LIST responses come from untrusted servers, a malicious FTP server can send a malformed listing with a permission field shorter than 9 characters, causing out-of-bounds reads. This could leak heap data or cause a crash.

*PoC sketch:* A malicious FTP server sends a LIST response with a truncated permission field, e.g., 'rw' instead of 'rwxr-xr-x'. The parser passes this short string to ftp_pl_get_permission(), which reads str[2] through str[8] beyond the allocated buffer.

### `lib/ftp.c [ftp_state_use_port+use_pasv]` — L1685 — conf=0.7
**CRLF Injection in FTP RETR/SIZE Command Construction**

The `ftpc->file` variable is used directly in `Curl_pp_sendf` calls for 'RETR %s' and 'SIZE %s' FTP commands without sanitizing for CRLF (\r\n) characters. If `ftpc->file` contains embedded carriage-return/line-feed sequences (e.g., from a URL-decoded path like `%0d%0aDELE%20important_file`), the FTP server will interpret the injected CRLF as a command separator, allowing arbitrary FTP command injection. For example, a filename 'test\r\nDELE important' would cause the server to process 'RETR test' followed by 'DELE important', enabling unauthorized file deletion, directory traversal, or other …

*PoC sketch:* Craft an FTP URL with CRLF in the path component: ftp://user:pass@ftp-server.example.com/%0d%0aDELE%20important_file.txt . When curl processes this URL, ftpc->file becomes '\r\nDELE important_file.txt', and the sent command becomes 'RETR \r\nDELE important_file.txt\r\n', which the FTP server interpr…


## 🟡 MEDIUM (19)

### `lib/urlapi.c [curl_url_set+curl_url_get]` — L1770 — conf=0.92
**Off-by-one error in URL scheme character validation**

The scheme validation loop uses `while(--plen)` starting with `s` pointing to `part[0]`. Since `--plen` pre-decrements, the loop executes `plen-1` times, checking characters at positions 0 through `plen-2`. The first character is redundantly checked (already validated by `ISALPHA(*s)`), while the LAST character at position `plen-1` is NEVER validated. This allows invalid characters (e.g., `\r`, `\n`, `?`, `!`, `/`, `@`) in the trailing position of a URL scheme. When `CURLU_NON_SUPPORT_SCHEME` is set, schemes like `"http\r"` or `"a!"` are accepted, potentially enabling CRLF injection or scheme …

*PoC sketch:* curl_url_set(url_handle, CURLUPART_SCHEME, "a!", CURLU_NON_SUPPORT_SCHEME); /* succeeds — '!' is never validated */ curl_url_set(url_handle, CURLUPART_SCHEME, "x\r", CURLU_NON_SUPPORT_SCHEME); /* succeeds — CR is never validated, enables CRLF injection */ curl_url_set(url_handle, CURLUPART_SCHEME, "…

### `lib/socks.c [do_SOCKS5]` — L362 — conf=0.85
**DNS entry reference leak when no IPv4 address found**

When DNS resolution succeeds (dns != NULL) but returns only non-IPv4 addresses (e.g., only IPv6/AAAA records), the code fails to call Curl_resolv_unlink() to release the DNS cache entry reference. The 'if(hp)' branch correctly calls Curl_resolv_unlink, but the 'else' branch (no IPv4 address found after scanning) only calls failf() and falls through to return CURLPX_RESOLVE_HOST without unlinking. This causes a reference count leak, preventing the DNS cache entry from ever being freed. An attacker who can trigger repeated SOCKS4 connections to IPv6-only hostnames through a victim's curl instanc…

*PoC sketch:* 1. Configure curl to use a SOCKS4 proxy 2. Attempt connection to a hostname resolving only to AAAA records (e.g., 'ipv6.google.com') 3. DNS resolves successfully (dns != NULL), but the while loop finds no AF_INET entry (hp == NULL) 4. Code takes the 'else' branch: failf() is called, but Curl_resolv_…

### `lib/url.c [parseurlandfillconn+setup_range]` — L1855 — conf=0.8
**Credential precedence logic flaw allows URL username to override CURLOPT_USERNAME**

The password processing block sets `data->state.creds_from = CREDS_URL` upon successfully extracting a URL password. The subsequent username processing block checks `(!data->state.aptr.user || (data->state.creds_from != CREDS_OPTION))`. If an application sets CURLOPT_USERNAME (making `aptr.user` non-NULL and initially `creds_from == CREDS_OPTION`) but does NOT set CURLOPT_PASSWORD (`aptr.passwd == NULL`), the password block enters via `!aptr.passwd`, processes the URL password, and sets `creds_from = CREDS_URL`. Now the username block's second condition `creds_from != CREDS_OPTION` is true, ca…

*PoC sketch:* 1. Application calls curl_easy_setopt(curl, CURLOPT_USERNAME, "admin"); but does NOT set CURLOPT_PASSWORD. 2. Application requests URL `http://attacker:pass@trusted-host.com/resource`. 3. Password block: `!aptr.passwd` is true → URL password extracted, `creds_from = CREDS_URL`. 4. Username block: `!…

### `lib/cookie.c [parse_cookie_header+sanitize+add]` — L553 — conf=0.8
**Case-Insensitive Cookie Prefix Detection (RFC Violation)**

The prefix detection uses strncasecompare() (case-insensitive) to match '__Secure-' and '__Host-', but RFC 6265bis Section 4.1.2 requires these prefixes to be case-sensitive. This means cookies named '__secure-foo' or '__HOST-bar' will have their prefix flags set to TRUE, which is incorrect. If downstream enforcement relies on these flags to grant trust (e.g., treating a cookie as securely-scoped), a non-secure-origin cookie with a lowercase prefix could be incorrectly elevated. Conversely, if enforcement rejects such cookies, legitimate non-prefix cookies with similar names would be incorrect…

*PoC sketch:* Set-Cookie: __secure-session=value (lowercase 's')  strncasecompare('__Secure-', '__secure-', 9) returns TRUE, setting co->prefix_secure=TRUE even though the prefix is not the case-sensitive '__Secure-' required by the RFC. If enforcement trusts prefix_secure, this cookie gains unintended security p…

### `lib/url.c [parseurlandfillconn+setup_range]` — L1795 — conf=0.75
**In-place corruption of hostname buffer during IPv6 bracket stripping**

When the hostname starts with '[' (IPv6 address literal), the code strips the brackets by incrementing the hostname pointer and zeroing out the closing ']' in-place: `hostname++; hlen = strlen(hostname); hostname[hlen - 1] = 0;`. This directly mutates the buffer owned by `data->state.up.hostname` (allocated by `curl_url_get`), corrupting it from e.g. `[::1]` to `[::1\0`. The corrupted `data->state.up.hostname` is then used downstream (e.g., cookie domain matching, URL reconstruction for redirects, display in error messages). Any security decision relying on `data->state.up.hostname` having the…

*PoC sketch:* Provide a URL with an IPv6 literal hostname, e.g. `http://[::1]/path`. After parseurlandfillconn executes, `data->state.up.hostname` will contain `[::1` (missing closing bracket, null-terminated at the wrong position) instead of the original `[::1]`. Any subsequent code using `data->state.up.hostnam…

### `lib/ftplistparser.c [ftp_pl_state_machine]` — L258 — conf=0.75
**NULL pointer dereference / invalid pointer computation in ftp_pl_insert_finfo**

Curl_dyn_ptr(&infop->buf) can return NULL if the dynamic buffer is empty or allocation failed. The returned pointer 'str' is then used in pointer arithmetic (str + parser->offsets.filename, etc.) without a NULL check. If str is NULL, these compute addresses like 0+offset, which when dereferenced cause a crash or potentially exploitable behavior. Additionally, data->wildcard, wc->ftpwc, and ftpwc->parser are dereferenced without NULL checks.

*PoC sketch:* Trigger an OOM condition during FTP LIST parsing so that Curl_dyn_ptr returns NULL. The subsequent str + parser->offsets.filename computes a small non-zero address, causing a segfault or memory corruption when the finfo fields are later accessed.

### `lib/http_digest.c [output_digest+decode]` — L90 — conf=0.75
**Integer truncation in IE-style URI path precision specifier**

When authp->iestyle is true and the URI contains a '?', the length of the path before the query is computed as a size_t (urilen), then cast to int for the '%.*s' format specifier in aprintf. If urilen exceeds INT_MAX (2,147,483,647), the cast truncates the value. A negative precision in '%.*s' is treated as if no precision was specified, causing the full uripath (including the query string) to be used for digest computation instead of the truncated IE-style path. This defeats the IE-style digest mode and could cause authentication to succeed or fail unexpectedly against servers expecting the I…

*PoC sketch:* Craft a request with a uripath over 2GB before the '?' character (e.g., '/AAAAAA...?foo=bar' where the path segment exceeds INT_MAX). With iestyle=true, the (int)urilen cast wraps negative, %.*s ignores the precision, and the digest is computed over the full URI including the query string rather tha…

### `lib/curl_sasl.c [decode_mech+sasl_continue]` — L555 — conf=0.72
**Missing SASL_OAUTH2_RESP handler in switch statement**

The SASL_OAUTH2_RESP state is explicitly excluded from the continuation code check (line ~557: `sasl->state != SASL_OAUTH2_RESP`), meaning any server response code is accepted in this state. However, SASL_OAUTH2_RESP has no corresponding case in the switch statement. When the server responds after an OAUTHBEARER token exchange, the switch falls through without processing the response, leaving `newstate = SASL_FINAL` (the default) and `result = CURLE_OK`. This means: (1) OAUTHBEARER error responses (RFC 7628 JSON error payloads) are silently discarded — the client never parses server-provided e…

*PoC sketch:* 1. Connect to a malicious IMAP/SMTP server that advertises OAUTHBEARER. 2. Client sends AUTHENTICATE OAUTHBEARER with a token. 3. Server responds with continuation code (+) containing an error JSON payload (e.g., {"status":"invalid_token"}). 4. Client enters SASL_OAUTH2_RESP state; the contcode chec…

### `lib/ftplistparser.c [ftp_pl_state_machine]` — L287 — conf=0.7
**Integer overflow in Curl_ftp_parselist size calculation**

The computation `size_t bufflen = size*nmemb;` can overflow on 32-bit platforms where size_t is 32 bits. If both size and nmemb are large values (e.g., 0x10000 each), the product wraps around to a small value, causing the parser to process far more data than bufflen indicates. This mismatch can lead to buffer overflows in subsequent parsing logic that relies on bufflen as a boundary.

*PoC sketch:* On a 32-bit system, if the libcurl write callback is invoked with size=0x10000 and nmemb=0x10000, bufflen becomes 0 due to integer wraparound, while the actual data is 1GB. Subsequent parsing using bufflen as a bound would read/write out of bounds.

### `lib/mime.c [mime_read+boundary]` — L131 — conf=0.7
**VmsRealFileSize returns 0 on fopen failure, indistinguishable from empty file**

The VmsRealFileSize() function returns 0 when fopen() fails, which is indistinguishable from a legitimate empty file. A downstream caller that uses this size to allocate buffers or control read loops may behave incorrectly — e.g., skipping MIME part processing entirely, or proceeding with a zero-size assumption that leads to logic errors. On VMS, if a file becomes inaccessible between stat() and this call, the caller will silently treat it as a 0-byte file rather than reporting an error.

*PoC sketch:* 1. Create a MIME part pointing to a file on a VMS system. 2. After stat() succeeds but before fopen() is called, remove read permissions from the file. 3. VmsRealFileSize returns 0, causing the MIME encoder to produce an empty part instead of an error.

### `lib/cookie.c [parse_cookie_header+sanitize+add]` — L610 — conf=0.65
**Domain Attribute Value Not Validated for Invalid Octets**

The cookie name and value are validated with invalid_octets() (line ~570), but the domain attribute value is stored via strstore() without any similar validation. Control characters, null bytes, or other invalid octets in the domain value could corrupt subsequent security checks (e.g., tailmatch comparisons, domain matching) or be injected into cookie jar files. A domain value containing e.g. a null byte could cause string-based comparisons to terminate early, potentially bypassing tailmatch validation.

*PoC sketch:* Set-Cookie: session=abc; domain=example.com\x00.evil.com  The domain value containing a null byte is stored without validation. If cookie_tailmatch or subsequent code uses C string functions, the comparison may only see 'example.com', allowing a tailmatch against the legitimate domain while the stor…

### `lib/cookie.c [parse_cookie_header+sanitize+add]` — L617 — conf=0.6
**Public Suffix / TLD Cookie Setting Without PSL Protection**

When USE_LIBPSL is not defined, the only protection against setting cookies on TLDs or public suffixes is the bad_domain() check, which only requires a dot in the domain (or 'localhost'). This is insufficient — domains like 'co.uk' or 'com.au' contain dots but are public suffixes. When USE_LIBPSL IS defined, there is no check at all in this code path (the #ifndef block is skipped), meaning the PSL check must be done elsewhere. If it's not, cookies can be set on public suffixes, allowing a supercookie that tracks users across all subdomains of a public suffix.

*PoC sketch:* Without PSL: A server on sub.co.uk sends: Set-Cookie: tracker=1; domain=.co.uk  bad_domain() sees a dot and returns false (allows it). The cookie is set on .co.uk, making it a supercookie sent to every *.co.uk domain. With PSL defined, the #ifndef block is skipped entirely, and if PSL validation is …

### `lib/socks.c [do_SOCKS4]` — L230 — conf=0.6
**Integer underflow in socks_state_send outstanding tracking**

In `socks_state_send`, the check `DEBUGASSERT(sx->outstanding >= nwritten)` only validates that `outstanding` is not less than `nwritten` in debug builds. In release builds, if `Curl_conn_cf_send` returns a value larger than `sx->outstanding` (e.g., due to a bug in a custom filter or transport layer), the subtraction `sx->outstanding -= nwritten` will underflow. Since `outstanding` is `ssize_t`, this produces a very large positive value on the next call, causing `Curl_conn_cf_send` to attempt sending an enormous amount of data from `sx->outp`, leading to out-of-bounds reads from the `buffer` a…

*PoC sketch:* If a custom connection filter's send function erroneously returns more bytes than the requested `sx->outstanding` count, the underflow occurs. For example, if outstanding=8 and nwritten=10, outstanding becomes -2 (interpreted as a huge value on next send), and outp advances past the buffer boundary,…

### `lib/ftplistparser.c [ftp_pl_state_machine]` — L258 — conf=0.6
**Unchecked offset values in ftp_pl_insert_finfo can cause out-of-bounds reads**

The parser->offsets fields (filename, user, group, time, perm, symlink_target) are used to compute pointers into the dynamic buffer via str + offset. If a malicious FTP server sends a crafted LIST response that causes these offsets to be set to values larger than the actual buffer size, the resulting pointers will reference memory outside the buffer. The offsets are not validated against the buffer length before use.

*PoC sketch:* A malicious FTP server sends a LIST response that tricks the parser into recording a large offset value (e.g., parser->offsets.filename = 0xFFFFFF). When ftp_pl_insert_finfo computes str + 0xFFFFFF, it creates a pointer far beyond the allocated buffer, causing an out-of-bounds read when finfo->filen…

### `lib/http_digest.c [output_digest+decode]` — L88 — conf=0.6
**Missing NULL check on uripath parameter leads to NULL pointer dereference**

The uripath parameter is used directly in strchr() and strdup() without any NULL validation. If a caller passes a NULL uripath, strchr((char *)uripath, '?') on line 89 or strdup((char *)uripath) on line 96 will dereference a NULL pointer, causing a crash (denial of service). While curl's internal callers likely always provide a valid uripath, the function does not defensively validate this input, making it fragile against future code changes or unexpected call paths.

*PoC sketch:* Call Curl_output_digest(data, FALSE, (const unsigned char *)"GET", NULL) with a NULL uripath. The strchr on line 89 dereferences NULL, causing SIGSEGV.

### `lib/ftp.c [ftp_state_use_port+use_pasv]` — L1748 — conf=0.5
**PASV Response Parsing Lacks IP Address Validation (Potential SSRF)**

The `match_pasv_6nums` function parses the 6 numbers from a PASV response without any validation that the resulting IP address matches the FTP server's control connection address. While the `control_address` helper function exists (presumably for comparison in `ftp_state_pasv_resp`), the parsing function itself imposes no constraints. If the caller does not properly validate the parsed address against the control connection, a malicious FTP server can return a PASV response pointing to an arbitrary internal IP and port, causing the client to connect to that address (SSRF). This is the classic …

*PoC sketch:* A malicious FTP server responds to PASV with: '227 Entering Passive Mode (192,168,1,1,0,80)' where 192.168.1.1:80 is an internal service. If the client does not validate that 192.168.1.1 matches the control connection address, it will connect to the internal service, potentially exfiltrating data or…

### `lib/socks.c [do_SOCKS4]` — L103 — conf=0.5
**Missing buffersize validation in Curl_blockread_all**

The `Curl_blockread_all` function accepts a `ssize_t buffersize` parameter but never validates that it is positive. If a caller passes a negative value, the `Curl_conn_cf_recv` call would receive a negative size, which when cast to `size_t` internally becomes a very large value, potentially causing a buffer overflow. Additionally, the loop condition `buffersize -= nread` with a negative initial `buffersize` would increment rather than decrement, creating an infinite loop or memory corruption.

*PoC sketch:* Call Curl_blockread_all with buffersize = -1. The recv call interprets this as SIZE_MAX, reading far beyond the allocated buffer. The loop's buffersize tracking also breaks, as subtracting nread from a negative buffersize increases its magnitude.

### `lib/mime.c [mime_read+boundary]` — L79 — conf=0.5
**Signed char index into qp_class table may cause out-of-bounds read**

The qp_class[] table is indexed by byte values 0-255. If the calling code (in the not-shown encoder_qp_read function) passes a plain `char` argument on a platform where `char` is signed, negative values (0x80-0xFF) would produce negative array indices, causing an out-of-bounds read before the start of the table. The table entries for 0x80-0xFF are all 0 (not-representable), so the intended behavior is to reject/encode those bytes, but a signed-char index would read from memory before the table instead. This could leak stack/heap data or cause a crash.

*PoC sketch:* Provide MIME data with bytes >= 0x80 to the quoted-printable encoder on a platform where char is signed (e.g., x86 Linux with gcc). If the encoder uses `qp_class[data[i]]` where data[i] is of type `char`, the negative index causes an OOB read.

### `lib/mime.c [mime_read+boundary]` — L105 — conf=0.45
**Signed char index into aschex table may cause out-of-bounds read**

The aschex[] table has 16 entries (plus null terminator) for nibble values 0-15. If used with a signed char value where `byte >> 4` produces a negative result (sign-extended right shift of a negative char), the index would be negative, causing an out-of-bounds read. Typical usage `aschex[byte >> 4]` is safe only if byte is unsigned. If the MIME encoder processes data through a signed char pointer, high-byte values (>=0x80) would trigger this.

*PoC sketch:* Provide binary MIME data containing bytes >= 0x80. If the hex-encoding path uses a signed char variable to index aschex[], the right-shift of a negative value yields a large or negative index, reading outside the 17-byte table.


## 🔵 LOW (14)

### `lib/socks.c [do_SOCKS5]` — L460 — conf=0.8
**DEBUGASSERT used instead of runtime buffer bounds check**

The code uses DEBUGASSERT(packetsize <= sizeof(sx->buffer)) to verify the constructed packet fits within the buffer. DEBUGASSERT is compiled out in release builds, providing zero runtime protection. While the current 255-byte username limit makes this unreachable (max packetsize = 9 + 255 = 264, well under the ~600-byte buffer), this relies entirely on the username length check remaining correct and the buffer size not being reduced. The SOCKS4a path has a proper runtime check (packetsize + hostnamelen < sizeof(sx->buffer)), but the non-4a path has only this debug-only assertion, creating an i…

*PoC sketch:* Not currently exploitable with existing limits. To demonstrate risk: if the plen > 255 check were accidentally removed or the buffer size reduced below 264 bytes, a long proxy username would cause a buffer overflow in release builds with no runtime detection. Contrast with the SOCKS4a path which has…

### `lib/urlapi.c [curl_url_set+curl_url_get]` — L1848 — conf=0.7
**Potential double URL-encoding via flag leakage in CURLUPART_URL relative resolution**

When setting CURLUPART_URL with a relative URL, the code calls `curl_url_get(u, CURLUPART_URL, &oldurl, flags)` passing the full `flags` from `curl_url_set`. If the caller passes `CURLU_URLENCODE`, `curl_url_get` will URL-encode the existing URL components. The encoded result is then concatenated with the relative part and re-parsed by `parseurl_and_replace`, causing double-encoding of percent-encoded sequences (e.g., `%20` becomes `%2520`). This can corrupt URL semantics and potentially bypass security checks that compare decoded URLs.

*PoC sketch:* /* Set a URL with encoded spaces */ curl_url_set(u, CURLUPART_URL, "http://example.com/path%20with%20spaces", 0); /* Append relative path with CURLU_URLENCODE flag — old URL gets re-encoded */ curl_url_set(u, CURLUPART_URL, "newpath", CURLU_URLENCODE); /* Result: %20 becomes %2520 (double-encoded) *…

### `lib/socks.c [do_SOCKS4]` — L126 — conf=0.7
**Improper error return value ~CURLE_OK in Curl_blockread_all**

The function returns `~CURLE_OK` (bitwise NOT of 0, typically -1 or 0xFFFFFFFF) as an error code in two places: when `SOCKET_READABLE` times out with no data, and when `nread` is 0 (EOF) before all requested bytes are read. This value is not a valid `CURLcode` enum member. Callers that switch on the return value or use it in comparisons against known CURLcode values will not match any expected error case, potentially treating the error as success or falling through to unintended code paths.

*PoC sketch:* Trigger a SOCKS GSSAPI connection where the remote closes the connection before sending all expected bytes. The function returns ~CURLE_OK, which the caller may not handle correctly, potentially continuing with uninitialized or partial buffer data.

### `lib/url.c [parseurlandfillconn+setup_range]` — L1738 — conf=0.6
**Missing URL validation when using CURLOPT_CURLU (use_set_uh path)**

When `use_set_uh` is true (i.e., the URL was set via CURLOPT_CURLU rather than CURLOPT_URL), the code duplicates the URL handle with `curl_url_dup` but skips the `curl_url_set(uh, CURLUPART_URL, ...)` call entirely. This means the URL bypasses the validation and normalization that `curl_url_set` performs with flags like `CURLU_GUESS_SCHEME`, `CURLU_NON_SUPPORT_SCHEME`, `CURLU_DISALLOW_USER`, and `CURLU_PATH_AS_IS`. A malformed or maliciously crafted URL handle set via CURLOPT_CURLU could contain data that would normally be rejected by the URL parser, potentially leading to unexpected behavior …

*PoC sketch:* 1. Create a CURLU handle with curl_url() and set a malformed URL component directly via curl_url_set with permissive flags. 2. Set the handle via curl_easy_setopt(curl, CURLOPT_CURLU, uh). 3. Perform the request — the malformed URL components bypass the validation that would occur in the curl_url_se…

### `lib/ftp.c [ftp_state_use_port+use_pasv]` — L1748 — conf=0.6
**PASV Response Parser Accepts Malformed Trailing Content**

The `match_pasv_6nums` function returns TRUE after successfully parsing 6 comma-separated numbers without verifying that the string properly terminates afterward. A PASV response like '227 (10,0,0,1,4,1EXTRA_INJECTION_DATA)' would be accepted, with `p` pointing to the trailing content after parsing. This could allow a malicious server to inject unexpected data that might be misinterpreted by the caller if it continues parsing from the returned position without additional validation.

*PoC sketch:* FTP server sends: '227 Entering Passive Mode (10,0,0,1,4,1)\r\nINJECTED'. The match_pasv_6nums function returns TRUE with array=[10,0,0,1,4,1], but the trailing content could confuse subsequent parsing logic.

### `lib/mime.c [mime_read+boundary]` — L131 — conf=0.6
**TOCTOU race condition in VMS file size determination**

VmsRealFileSize() opens and reads the file to determine its true size, but this is called after a prior stat(). Between the stat() and the fopen()/fread() loop, the file could be replaced (e.g., via symlink swap) with a different file of different content/size. This could cause the reported size to differ from the actual data read, leading to inconsistencies in MIME content-length headers vs actual body data. This is VMS-specific and requires local access and timing.

*PoC sketch:* 1. Create a symlink pointing to a small file. 2. Trigger a MIME upload that stats the symlink (gets small size). 3. Between stat() and VmsRealFileSize()'s fopen(), replace the symlink to point to a large file. 4. The Content-Length header will be wrong relative to actual data sent.

### `lib/curl_sasl.c [decode_mech+sasl_continue]` — L564 — conf=0.55
**No default case in SASL state switch — unhandled states proceed to SASL_FINAL**

The switch statement in Curl_sasl_continue has no default case. If sasl->state contains an unexpected or corrupted value (e.g., via memory corruption, integer overflow in state assignment, or a bug in sasl_state()), execution falls through the switch without setting any error result or response. The default newstate=SASL_FINAL and result=CURLE_OK remain in effect, causing the state machine to transition to SASL_FINAL as if authentication completed successfully. On the next invocation, the SASL_FINAL check only validates the server's response code against finalcode — if a malicious server sends…

*PoC sketch:* 1. Trigger a memory corruption vulnerability (e.g., heap overflow in a separate curl component) that overwrites sasl->state with a value not matching any case label (e.g., 0xFF). 2. The next call to Curl_sasl_continue skips all case handlers, newstate remains SASL_FINAL, result remains CURLE_OK. 3. …

### `lib/url.c [parseurlandfillconn+setup_range]` — L1955 — conf=0.5
**Dangling pointer and potential double-free in setup_range after failed allocation**

In setup_range, after `free(s->range)`, the pointer `s->range` is not set to NULL. If the subsequent `aprintf` or `strdup` call fails (returns NULL), the function returns CURLE_OUT_OF_MEMORY with `s->range` still pointing to freed memory (dangling pointer). Additionally, `s->rangestringalloc` is not reset to false. On a subsequent invocation of setup_range, the check `if(s->rangestringalloc)` would be true, leading to `free(s->range)` on the already-freed dangling pointer — a double-free vulnerability. While the function is truncated and the full error handling is not visible, the pattern of n…

*PoC sketch:* 1. Set a range via CURLOPT_RANGE to trigger initial allocation of s->range with s->rangestringalloc=true. 2. On a subsequent request, trigger setup_range again where the aprintf/strdup allocation fails (e.g., via memory exhaustion). 3. s->range is freed but not NULLed; s->rangestringalloc remains tr…

### `lib/ftplistparser.c [ftp_pl_state_machine]` — L276 — conf=0.5
**Symlink target injection bypass via crafted target string**

The check `strstr(finfo->strings.target, " -> ")` attempts to discard symlinks containing multiple ' -> ' separators. However, this can be bypassed with variations such as '->' (no spaces), '  ->  ' (extra spaces), or Unicode lookalikes. A malicious FTP server could craft a symlink target that bypasses this filter, potentially leading to path traversal or confusion in downstream processing that relies on this validation.

*PoC sketch:* FTP server sends a LIST entry like 'lrwxrwxrwx 1 user group 20 Jan 1 00:00 link -> ../../etc/passwd -> extra' where the second '->' uses different spacing, bypassing the strstr check while still containing ambiguous path information.

### `lib/http_digest.c [output_digest+decode]` — L86 — conf=0.5
**IE-style digest path truncation enables authentication confusion**

When authp->iestyle is true, the digest is computed over only the path component (before '?'), while the full URI including the query string is sent in the HTTP request line. This mismatch between the URI used for digest computation and the URI actually requested can be exploited: an attacker who can observe or predict the digest response for a path (without query) could craft a request with arbitrary query parameters that passes digest authentication. Servers that validate the digest against the full request URI would reject it, but servers using IE-compatible validation would accept it, pote…

*PoC sketch:* 1. Server has IE-compatible digest auth on /resource?secret=yes. 2. Attacker authenticates to /resource (no query). 3. With iestyle=true, the same digest response is computed for both /resource and /resource?secret=yes since the query is stripped. 4. Attacker replays the digest with the full URI to …

### `lib/curl_sasl.c [decode_mech+sasl_continue]` — L575 — conf=0.45
**Unchecked NULL pointers for credentials and bearer tokens in SASL continue**

Multiple SASL mechanism handlers use conn->user, conn->passwd, and oauth_bearer (data->set.str[STRING_BEARER]) without NULL checks. For example, SASL_PLAIN passes conn->user and conn->passwd directly to Curl_auth_create_plain_message; SASL_OAUTH2 passes oauth_bearer to Curl_auth_create_oauth_bearer_message. If any of these are NULL (e.g., user configures OAUTHBEARER without setting a bearer token, or a connection is reused with cleared credentials), the downstream auth functions may dereference NULL, causing a crash (denial of service). While SASL mechanism selection typically validates creden…

*PoC sketch:* 1. Configure curl to use OAUTHBEARER authentication for an IMAP connection. 2. Do not set the STRING_BEARER option (oauth_bearer remains NULL). 3. If the mechanism selection logic has a race or edge case that allows OAUTHBEARER to be chosen without a bearer token, Curl_auth_create_oauth_bearer_messa…

### `lib/socks.c [do_SOCKS4]` — L190 — conf=0.4
**Potential out-of-bounds read in socksstate debug array access**

In debug+verbose builds, the `socksstate` function indexes the `socks_statename[]` array using `oldstate` and `sx->state` without bounds checking. If memory corruption or a logic error produces a state value outside the valid enum range (0-17), the array access `socks_statename[oldstate]` or `socks_statename[sx->state]` reads beyond the array bounds. While this only affects debug builds and requires prior corruption, it could leak heap data in the debug log or crash.

*PoC sketch:* If sx->state is corrupted to a value > 17 (e.g., via heap overflow from another vulnerability), the socks_statename access in the infof call reads beyond the array, potentially crashing or leaking memory addresses in the debug output.

### `lib/socks.c [do_SOCKS5]` — L374 — conf=0.4
**Missing ai_addrlen validation before sockaddr_in pointer cast**

When extracting the IPv4 address from the resolved addrinfo, the code casts hp->ai_addr to struct sockaddr_in* and accesses sin_addr.s_addr without verifying that hp->ai_addrlen >= sizeof(struct sockaddr_in). If a malformed addrinfo structure were provided (ai_family == AF_INET but ai_addrlen smaller than sizeof(sockaddr_in)), accessing the sin_addr field would read beyond the allocated ai_addr buffer. While curl's internal DNS resolver should produce well-formed structures, there is no defensive validation against corrupted or malformed data from external resolver libraries.

*PoC sketch:* Theoretical exploitation would require injecting a malformed addrinfo into curl's resolution path — e.g., via a compromised system resolver library or prior memory corruption altering ai_addrlen. Under normal conditions, curl's resolver always sets ai_addrlen correctly for AF_INET entries. Not pract…

### `lib/urlapi.c [curl_url_set+curl_url_get]` — L2010 — conf=0.35
**Incomplete query append logic (truncated code may hide buffer overflow)**

The code is truncated at `size_t querylen = u->q` within the `appendquery` branch. The append query logic must compute the combined length of the existing query string, an optional '&' separator, and the new query, then allocate and fill a buffer. If the length calculation in the unseen remainder fails to account for the separator or uses incorrect sizes, a heap buffer overflow could occur. Without the full code this cannot be confirmed, but the pattern is historically error-prone in URL manipulation libraries.

*PoC sketch:* N/A — code truncated; would require appending a very long query string to an existing query to trigger if a length miscalculation exists


## ⚪ INFO (3)

### `lib/ftp.c [match_pasv_6nums+ftp_state_pasv_resp]` — L1820 — conf=0.85
**Greedy PASV Number Matching Can Misparse Response**

The scanning loop `while(*str) { if(match_pasv_6nums(str, ip)) break; str++; }` greedily searches the entire 227 response for any six comma-separated numbers. A malicious server can embed a decoy sequence before the real PASV address to trick the client into connecting to an attacker-chosen IP:port. For example, embedding '6,6,6,6,6,6' in the response text before the parenthesized address causes the scanner to match the decoy instead of the legitimate address. This amplifies the SSRF risk since the attacker has full control over which 6-number sequence is matched first.

*PoC sketch:* FTP server responds with: '227 6,6,6,6,6,6 Entering Passive Mode (192,168,1,100,4,1)' The scanner matches '6,6,6,6,6,6' first, causing connection to 6.6.6.6:1542 instead of the intended address.

### `lib/ftp.c [match_pasv_6nums+ftp_state_pasv_resp]` — L1850 — conf=0.7
**PASV Port 0 Allows Invalid Data Connection Target**

The port derived from the PASV response `(ip[4] << 8) + ip[5]` can be 0 (when both ip[4] and ip[5] are 0). Port 0 is generally invalid for TCP connections and can cause undefined behavior in downstream connection logic. While not directly exploitable for remote code execution, it can cause unexpected errors or be combined with the SSRF issue to probe host behavior based on error responses.

*PoC sketch:* Malicious FTP server responds with: '227 Entering Passive Mode (192,168,1,1,0,0)' — port 0 is computed and passed to Curl_resolv/Curl_conn_setup.

### `lib/curl_sasl.c [decode_mech+sasl_continue]` — L131 — conf=0.6
**Integer truncation in Curl_sasl_decode_mech return type**

Curl_sasl_decode_mech returns unsigned short (16-bit), but mechtable[i].bit may be defined as a wider type (e.g., unsigned int). If the mechanism bit flags exceed 16 bits (i.e., more than 16 SASL mechanisms are registered), the return value is silently truncated. This could cause two different mechanisms to map to the same truncated bit value, leading to mechanism confusion — the client could attempt a weaker or different authentication mechanism than intended. Currently, curl supports fewer than 16 SASL mechanisms, making this a latent concern rather than an active vulnerability.

*PoC sketch:* 1. Add 17 or more mechanisms to mechtable with bit values 1<<0 through 1<<16. 2. Call Curl_sasl_decode_mech with a string matching mechanism 17 (bit 1<<16 = 65536). 3. The return value is truncated to 0 (65536 & 0xFFFF), which matches no mechanism or mechanism 0. 4. The client falls back to a differ…

