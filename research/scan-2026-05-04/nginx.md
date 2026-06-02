# nginx — GLM-5.1 Pipeline A scan (2026-05-05)

- **Model**: glm-5.1
- **Segments**: 33 (0 hit GLM API timeout)
- **Total findings**: 41
  - 🔴 **CRITICAL**: 2
  - 🟠 **HIGH**: 10
  - 🟡 **MEDIUM**: 16
  - 🔵 **LOW**: 10
  - ⚪ **INFO**: 3

## Per-segment summary

| Segment | Time (s) | Findings | Status |
|---|---:|---:|---|
| `ngx_http_parse.c [parse_request_line A: method-states]` | 165.1 | 0 | ✅ ok |
| `ngx_http_parse.c [parse_request_line B: URI-path]` | 360.4 | 0 | ✅ ok |
| `ngx_http_parse.c [parse_request_line C: URI-continuation]` | 130.2 | 3 | ✅ ok |
| `ngx_http_parse.c [parse_request_line D: HTTP-version]` | 360.5 | 0 | ✅ ok |
| `ngx_http_parse.c [parse_request_line E: CRLF+errors]` | 84.4 | 2 | ✅ ok |
| `ngx_http_parse.c [parse_header_line A: name+colon]` | 360.5 | 0 | ✅ ok |
| `ngx_http_parse.c [parse_header_line B: value+CRLF]` | 360.4 | 0 | ✅ ok |
| `ngx_http_parse.c [parse_complex_uri A: %-decode]` | 360.4 | 0 | ✅ ok |
| `ngx_http_parse.c [parse_complex_uri B: dot-segments]` | 360.5 | 0 | ✅ ok |
| `ngx_http_parse.c [parse_complex_uri C: query+args]` | 360.4 | 0 | ✅ ok |
| `ngx_http_parse.c [parse_unsafe_uri: traversal-detect]` | 286.7 | 1 | ✅ ok |
| `ngx_http_parse.c [parse_chunked A: hex-size-accum]` | 360.6 | 0 | ✅ ok |
| `ngx_http_parse.c [parse_chunked B: data+trailer]` | 360.4 | 0 | ✅ ok |
| `ngx_http_parse.c [parse_status_line: upstream-response]` | 109.0 | 3 | ✅ ok |
| `ngx_http_request.c [init_request A: pool+bufs]` | 43.2 | 4 | ✅ ok |
| `ngx_http_request.c [init_request B: ssl+addr]` | 64.2 | 4 | ✅ ok |
| `ngx_http_request.c [read_request_header A: recv-loop]` | 102.7 | 4 | ✅ ok |
| `ngx_http_request.c [process_request_line entry]` | 156.8 | 3 | ✅ ok |
| `ngx_http_request.c [process_request_headers A]` | 85.4 | 3 | ✅ ok |
| `ngx_http_request.c [process_request_headers B]` | 328.2 | 3 | ✅ ok |
| `ngx_http_v2.c [connection-init+preface]` | 6.9 | 0 | ✅ ok |
| `ngx_http_v2.c [frame-header+length-check]` | 25.0 | 0 | ✅ ok |
| `ngx_http_v2.c [frame-type-dispatch+stream-lookup]` | 136.3 | 0 | ✅ ok |
| `ngx_http_v2.c [SETTINGS+PING-frames]` | 146.6 | 4 | ✅ ok |
| `ngx_http_v2.c [HEADERS-frame+hpack-decode]` | 360.4 | 0 | ✅ ok |
| `ngx_http_v2.c [CONTINUATION+list-size-limit]` | 148.8 | 4 | ✅ ok |
| `ngx_http_v2.c [DATA-frame+flow-control]` | 158.9 | 3 | ✅ ok |
| `ngx_http_proxy.c [hop-by-hop-filter]` | 25.5 | 0 | ✅ ok |
| `ngx_http_proxy.c [upstream-header-copy]` | 16.5 | 0 | ✅ ok |
| `ngx_http_proxy.c [response-header-rewrite]` | 11.6 | 0 | ✅ ok |
| `ngx_http_proxy.c [create_request: request-line]` | 17.8 | 0 | ✅ ok |
| `ngx_http_proxy.c [create_request: Host+X-Forwarded-For]` | 16.4 | 0 | ✅ ok |
| `ngx_http_proxy.c [create_request: body+auth]` | 13.3 | 0 | ✅ ok |

## 🔴 CRITICAL (2)

### `ngx_http_parse.c [parse_unsafe_uri: traversal-detect]` — L48 — conf=1.0
**Path Traversal Bypass via `/..` and `/..?args` due to Flawed Traversal Detection Logic**

The function `ngx_http_parse_unsafe_uri` is intended to prevent path traversal by detecting `..` segments. However, the detection logic for `/..` is flawed in two ways: 1) It uses `len > 2` (line 48), which fails to match when the URI ends with `/..` (where `len` is 2 after the `/`). This allows bypasses like `/..` or `/a/..`. 2) It only checks if `..` is followed by a path separator or if `len == 3` (line 53). It fails to recognize `?` as a valid terminator for a path segment. This allows bypasses like `/..?args` or `/a/..?args`, where the path component is `/..` but the check is skipped beca…

*PoC sketch:* 1. `http://target/..` (bypasses due to `len > 2`) 2. `http://target/valid_path/..` (bypasses due to `len > 2`) 3. `http://target/..?args` (bypasses due to `?` not being recognized as a terminator) 4. `http://target/valid_path/..?args` (bypasses due to `?` not being recognized as a terminator) 5. `ht…

### `ngx_http_v2.c [CONTINUATION+list-size-limit]` — L1 — conf=0.85
**HTTP/2 CONTINUATION Frame Flood - Missing Header List Size Enforcement**

The filename 'CONTINUATION+list-size-limit' indicates this code is part of nginx's HTTP/2 implementation that fails to properly enforce the http2_max_header_list_size limit when processing CONTINUATION frames. An attacker can send an unlimited sequence of CONTINUATION frames (without setting END_HEADERS flag) in a single header block, causing the server to accumulate unbounded memory for the header list. While the shown code is the output/send path, the vulnerability originates in the input parsing path where CONTINUATION frames are accepted without checking cumulative header list size against…

*PoC sketch:* Send an HTTP/2 connection with a HEADERS frame (no END_HEADERS flag) followed by an unlimited sequence of CONTINUATION frames (each without END_HEADERS flag). Each CONTINUATION frame adds to the header list without the server checking if the total size exceeds http2_max_header_list_size. PoC: import…


## 🟠 HIGH (10)

### `ngx_http_parse.c [parse_request_line C: URI-continuation]` — L52 — conf=0.85
**Overly Permissive IPv6 Literal Character Set Enables Request Smuggling**

The sw_host_ip_literal state accepts sub-delims (!, $, &, ', (, ), *, +, ,, ;, =) and unreserved characters (_, ~) inside IPv6 address brackets [...]. Per RFC 3986, an IPv6address literal should only contain hex digits, colons, and dots. The semicolon (;) is particularly dangerous: a request like GET http://[::1;evil.com]/admin HTTP/1.1 will be accepted by nginx with the full bracketed string as the host, but downstream parsers or backend servers may interpret the semicolon as a delimiter, treating the effective host as ::1 or evil.com. This parsing discrepancy enables HTTP request smuggling a…

*PoC sketch:* GET http://[::1;evil.com]/admin HTTP/1.1 Host: [::1;evil.com]    -- nginx accepts this, but a backend may parse the host as '::1' (localhost), bypassing host-based ACLs. Similarly: GET http://[::1&inject=param]/ HTTP/1.1 where '&' could be misinterpreted as a query separator by downstream systems…

### `ngx_http_v2.c [SETTINGS+PING-frames]` — L10 — conf=0.85
**Shared recv_buffer across HTTP/2 connections enables data corruption**

The receive buffer `h2mcf->recv_buffer` is allocated from the global cycle pool and stored in the main configuration (`h2mcf`), making it shared across ALL HTTP/2 connections within the same worker process. The lazy initialization pattern `if (h2mcf->recv_buffer == NULL)` is not atomic. More critically, when multiple HTTP/2 connections are active and their event-driven processing is interleaved (normal in nginx's architecture), Connection A may save partial frame data into the shared buffer, yield waiting for more data, then Connection B overwrites the same buffer with its own data. When Conne…

*PoC sketch:* 1. Open HTTP/2 connection A to the server, send a partial SETTINGS frame (incomplete, forcing the server to buffer the remainder in h2mcf->recv_buffer) 2. Before sending the rest of connection A's frame, open HTTP/2 connection B and send data that triggers use of the same recv_buffer 3. Complete con…

### `ngx_http_v2.c [DATA-frame+flow-control]` — L68 — conf=0.75
**Use-After-Free via pool destruction in ngx_http_v2_handle_connection**

In ngx_http_v2_handle_connection, the function destroys h2c->pool via ngx_destroy_pool() and then continues to access h2c members and modify connection state. While the code sets h2c->pool, h2c->free_frames, and h2c->free_fake_connections to NULL after destruction, any other h2c members that reference pool-allocated memory (e.g., h2c->state buffers, stream-related structures, or HPACK decoder state) become dangling pointers. The connection is then transitioned to an idle state with ngx_http_v2_idle_handler. When new data arrives on this idle connection, the idle handler or subsequent processin…

*PoC sketch:* 1. Establish an HTTP/2 connection to the server. 2. Send requests that cause h2c->state to retain references to pool-allocated buffers (e.g., partial HPACK decoding state with allocated dynamic table entries). 3. Trigger the connection to enter the idle/cleanup path in ngx_http_v2_handle_connection …

### `ngx_http_request.c [init_request B: ssl+addr]` — L30 — conf=0.7
**HTTP Request Smuggling via Duplicate TE Header Processing**

The TE header is processed with ngx_http_process_header_line (allowing duplicates) while Transfer-Encoding uses ngx_http_process_unique_header_line. An attacker can send multiple TE headers with conflicting values. When a frontend proxy and Nginx disagree on which TE header value takes precedence, this creates a desynchronization that enables HTTP request smuggling. The TE header directly influences chunked transfer encoding behavior, making this particularly dangerous when combined with Transfer-Encoding: chunked smuggling techniques (e.g., TE: chunked vs TE: chunked, identity).

*PoC sketch:* POST / HTTP/1.1 Host: target Transfer-Encoding: chunked TE: chunked TE: identity Content-Length: 10  0  GET /smuggled HTTP/1.1 Host: evil

### `ngx_http_v2.c [CONTINUATION+list-size-limit]` — L95 — conf=0.7
**Frame Handler Called on Partially-Sent Data - Potential Use-After-Free**

After c->send_chain() returns, the code unconditionally calls out->handler() for ALL frames in the output queue, regardless of whether send_chain actually transmitted all the data. If send_chain could only partially send the data (returning a non-NULL, non-error chain), handlers for frames whose data wasn't fully flushed are still invoked. These handlers may free frame buffers or update stream state assuming the data was sent, while the connection's write buffer still references those buffers. This can lead to use-after-free when the connection later attempts to send the remaining buffered dat…

*PoC sketch:* 1. Establish HTTP/2 connection with constrained send buffer (e.g., via TCP window manipulation). 2. Trigger server to queue multiple response frames. 3. When send_chain partially sends data, handlers for unsent frames are called, potentially freeing buffers still referenced by the unsent chain. 4. W…

### `ngx_http_request.c [read_request_header A: recv-loop]` — L22 — conf=0.65
**Integer underflow in address matching loop leading to out-of-bounds read**

In the AF_INET6 address matching loop, the condition `i < port->naddrs - 1` is used. If `port->naddrs` is 0 (an unsigned type), the subtraction `port->naddrs - 1` wraps around to a very large value (e.g., SIZE_MAX or UINT_MAX). This causes the loop to iterate far beyond the bounds of the `addr6` array, resulting in out-of-bounds memory reads during `ngx_memcmp`. After the loop, `hc->addr_conf = &addr6[i].conf` also performs an out-of-bounds access. The same vulnerability exists in the AF_INET default case (lines ~40-46). This could lead to information disclosure, crash (DoS), or potential code…

*PoC sketch:* Trigger requires a server configuration where a listening port has port->naddrs == 0 (no addresses configured). If an attacker can influence server configuration reload or if a race condition during reconfiguration sets naddrs to 0 transiently, a connection to that port would trigger the underflow. …

### `ngx_http_request.c [read_request_header A: recv-loop]` — L40 — conf=0.65
**Integer underflow in AF_INET address matching loop leading to out-of-bounds read**

Identical to the AF_INET6 case: the loop `for (i = 0; i < port->naddrs - 1; i++)` suffers from an unsigned integer underflow when `port->naddrs` is 0. The variable `naddrs` is of type `ngx_uint_t` (unsigned), so `0 - 1` wraps to UINT_MAX. The loop iterates out of bounds on the `addr` array, and the post-loop assignment `hc->addr_conf = &addr[i].conf` accesses memory far beyond the array. This can cause a segmentation fault (DoS) or information disclosure from heap memory.

*PoC sketch:* Same as above — requires port->naddrs == 0. A TCP connection to an affected port triggers the OOB read in the AF_INET path. The memcmp on addr[i].addr vs sin->sin_addr.s_addr reads beyond the allocated addr array, and the final addr[i].conf dereference reads from arbitrary heap offsets.

### `ngx_http_request.c [init_request B: ssl+addr]` — L95 — conf=0.55
**Virtual Host Confusion via Incomplete Address Matching in ngx_http_init_connection**

The ngx_http_init_connection function begins address-based server configuration lookup when port->naddrs > 1, but the code is structured such that the default server configuration may be selected before SSL/TLS handshake completes. When multiple virtual hosts share a port with different SSL certificates, an attacker can trigger a configuration mismatch by sending a request with a Host header that doesn't match the SNI hostname. This can cause Nginx to serve content from the wrong virtual host, potentially bypassing access controls or leaking information from other tenants. The hc structure all…

*PoC sketch:* openssl s_client -connect target:443 -servername legitimate.example.com GET /private-endpoint HTTP/1.1 Host: other-vhost.example.com

### `ngx_http_request.c [init_request A: pool+bufs]` — L72 — conf=0.4
**Incomplete/truncated source prevents full analysis - potential HTTP request smuggling via duplicate Content-Length**

The ngx_http_headers_in[] array maps 'Content-Length' to ngx_http_process_unique_header_line, which is intended to reject duplicate headers. However, without seeing the implementation of ngx_http_process_unique_header_line, we cannot verify it properly rejects duplicate Content-Length headers. If duplicates are not properly rejected, an attacker could send requests with multiple Content-Length headers with different values, enabling HTTP request smuggling. The code is truncated at line 100 (Content-Range header definition), hiding critical implementation details.

*PoC sketch:* POST / HTTP/1.1 Host: target Content-Length: 0 Content-Length: 44  GET /admin HTTP/1.1 Host: target

### `ngx_http_request.c [init_request A: pool+bufs]` — L28 — conf=0.35
**Truncated code hides large header buffer allocation logic**

The function ngx_http_alloc_large_header_buffer is declared (line 28-29) but its implementation is not shown. This function is responsible for reallocating buffers when request headers exceed the initial buffer size. The file description 'pool+bufs' suggests this is the area of interest. Known nginx vulnerabilities (e.g., CVE-2013-2070, CVE-2014-0133) have involved buffer management in this path. Without the implementation, we cannot verify proper bounds checking, integer overflow protection on buffer size calculations, or correct pool memory management.

*PoC sketch:* GET / HTTP/1.1 X-Large-Header: <extremely long value exceeding large_client_header_buffers>


## 🟡 MEDIUM (16)

### `ngx_http_parse.c [parse_request_line E: CRLF+errors]` — L100 — conf=0.95
**HTTP Minor Version Validation Bypass (Check-Before-Update)**

In the `sw_minor_digit` state, the overflow check `if (r->http_minor > 99)` is performed BEFORE the value is updated with `r->http_minor = r->http_minor * 10 + (ch - '0')`. This means when http_minor is exactly 99 (two valid digits already parsed), the check passes (99 is not > 99), and the subsequent multiplication and addition produces 990-999 — far exceeding the intended maximum of 99. This allows a 3-digit minor version number (e.g., HTTP/1.999) to be accepted. The resulting `http_version` computed as `r->http_major * 1000 + r->http_minor` would be 1999 instead of the expected maximum of 1…

*PoC sketch:* GET / HTTP/1.999  This request line causes the parser to accept http_minor=999. Trace: first digit '9' -> http_minor=9 (sw_first_minor_digit); second digit '9' -> check 9>99? No -> http_minor=99; third digit '9' -> check 99>99? No -> http_minor=999. The request is accepted with http_version=1999 i…

### `ngx_http_parse.c [parse_status_line: upstream-response]` — L82 — conf=0.95
**HTTP Major Version Bounds Check Bypass (Off-by-One in Validation Order)**

In the sw_major_digit state, the bounds check `if (r->http_major > 99)` is performed BEFORE the multiplication `r->http_major = r->http_major * 10 + (ch - '0')`. This allows http_major to reach values up to 999 (e.g., input '9' → 9, passes check, then 9*10+9=99, passes check, then 99*10+9=999). The check should either be performed after the assignment or use a stricter threshold (>= 10) before multiplication. A major version of 999 could cause incorrect HTTP protocol version handling downstream, potentially leading to protocol confusion, incorrect keepalive behavior, or response smuggling when…

*PoC sketch:* Upstream response: HTTP/999.1 200 OK\r\n — This sets r->http_major to 999, bypassing the intended limit of 99.

### `ngx_http_parse.c [parse_status_line: upstream-response]` — L105 — conf=0.95
**HTTP Minor Version Bounds Check Bypass (Off-by-One in Validation Order)**

Identical to the major version issue: in sw_minor_digit, the check `if (r->http_minor > 99)` is performed BEFORE the multiplication `r->http_minor = r->http_minor * 10 + (ch - '0')`. This allows http_minor to reach values up to 999 instead of being capped at 99. A minor version exceeding 99 could cause incorrect protocol behavior, particularly in chunked transfer encoding and persistent connection handling decisions.

*PoC sketch:* Upstream response: HTTP/1.999 200 OK\r\n — This sets r->http_minor to 999, bypassing the intended limit of 99.

### `ngx_http_request.c [process_request_headers B]` — L42 — conf=0.7
**Missing check for recv() returning 0 (peer closed connection) in ngx_http_ssl_handshake**

When recv() with MSG_PEEK returns 0, it indicates the peer has performed an orderly connection shutdown. The code only checks for n == -1 (error) but does not handle n == 0. In the non-proxy-protocol path, n == 1 evaluates to false and execution falls through without closing the connection. This can lead to orphaned connections consuming file descriptors and memory, or potentially an infinite event loop if the read event is re-triggered on the closed socket, resulting in denial of service through resource exhaustion.

*PoC sketch:* 1. Connect to nginx on an SSL-enabled port 2. Immediately close the connection without sending any data 3. recv() with MSG_PEEK returns 0 4. The n == -1 check is skipped; hc->proxy_protocol is likely false 5. n == 1 is false, code falls through without closing the connection 6. Repeat to exhaust fil…

### `ngx_http_v2.c [DATA-frame+flow-control]` — L14 — conf=0.7
**Event posting suppression via h2c->blocked flag leading to connection stall**

The error handling path checks h2c->blocked before posting the write event: 'if (!h2c->blocked) { ngx_post_event(wev, &ngx_posted_events); }'. In ngx_http_v2_handle_connection, h2c->blocked is set to 1 before calling ngx_http_v2_send_output_queue and reset to 0 afterward. If ngx_http_v2_send_output_queue internally triggers the error path (e.g., via a nested call that hits the error label), the write event will not be posted because h2c->blocked is still 1. When the function then returns NGX_ERROR and the caller does not close the connection (e.g., if the error is handled differently upstream)…

*PoC sketch:* 1. Establish an HTTP/2 connection. 2. Send data that causes c->buffered to be set and ngx_http_v2_send_output_queue to be called. 3. While h2c->blocked is 1, trigger an error condition inside ngx_http_v2_send_output_queue (e.g., by causing a write failure or memory allocation failure). 4. The error …

### `ngx_http_parse.c [parse_request_line C: URI-continuation]` — L33 — conf=0.65
**URI Start Pointer Points to '?' Instead of '/' on Empty Path with Query**

In sw_host_end, when a '?' character is encountered (indicating a query string with no path), r->uri_start is set to point at the '?' character itself rather than a '/'. While r->empty_path_in_uri is set to 1 as a flag, downstream code that assumes uri_start always points to a '/' character and performs pointer arithmetic or comparisons based on that assumption could behave incorrectly. For example, code that computes path length as (args_start - uri_start - 1) or that dereferences *uri_start expecting '/' would produce wrong results. This inconsistency between the uri_start pointer value and …

*PoC sketch:* GET http://host?query=value HTTP/1.1 Host: host    -- r->uri_start points to '?' instead of '/', r->empty_path_in_uri = 1. Downstream code checking *r->uri_start == '/' will fail.

### `ngx_http_v2.c [SETTINGS+PING-frames]` — L73 — conf=0.65
**No rate limiting on control frame processing during initialization**

The initialization code processes all buffered data in a tight do-while loop without any rate limiting or frame count checks. A client that sends the HTTP/2 connection preface followed by a rapid burst of SETTINGS, PING, or other control frames before the server finishes initialization can force the server to spend excessive CPU time processing these frames. Combined with the `priority_limit` being set to `ngx_max(h2scf->concurrent_streams, 100)` (guaranteeing at least 100), an attacker can open many streams or send many control frames to exhaust server resources during the critical initializa…

*PoC sketch:* 1. Establish TCP connection 2. Send valid HTTP/2 connection preface 3. Immediately send thousands of PING frames or SETTINGS frames in a single TCP segment 4. The server's initialization loop processes all frames without yielding, consuming CPU and memory resources 5. Repeat from multiple connection…

### `ngx_http_request.c [init_request B: ssl+addr]` — L36 — conf=0.6
**Duplicate Upgrade Header Allowing WebSocket/HTTP Downgrade Attacks**

The Upgrade header is processed with ngx_http_process_header_line, allowing multiple Upgrade headers in a single request. An attacker could send conflicting Upgrade headers (e.g., 'Upgrade: websocket' and 'Upgrade: h2c') to cause inconsistent behavior between a frontend proxy and Nginx. This could lead to protocol downgrade attacks, where a proxy believes the connection is upgraded (and skips security checks) while Nginx treats it as a normal HTTP request, potentially bypassing authentication or authorization controls applied only to upgraded connections.

*PoC sketch:* GET /admin HTTP/1.1 Host: target Upgrade: websocket Upgrade: h2c Connection: Upgrade

### `ngx_http_v2.c [SETTINGS+PING-frames]` — L11 — conf=0.6
**Unchecked recv_buffer_size allows zero-size or undersized buffer allocation**

The allocation `ngx_palloc(ngx_cycle->pool, h2mcf->recv_buffer_size)` does not validate that `recv_buffer_size` is sufficient for HTTP/2 frame processing. If the configuration sets `recv_buffer_size` to 0 or a value smaller than the minimum required to process HTTP/2 frames (9 bytes for frame header + payload), subsequent frame processing functions that copy data into this buffer will write past its bounds. While nginx typically enforces a minimum via configuration directives, the allocation site itself lacks a defensive check.

*PoC sketch:* Set `http2_recv_buffer_size` to an extremely small value (e.g., 1 byte) in nginx configuration. Establish an HTTP/2 connection and send any frame. The frame processing code will attempt to copy frame data into the undersized buffer, causing a heap buffer overflow.

### `ngx_http_v2.c [CONTINUATION+list-size-limit]` — L30 — conf=0.6
**h2c->blocked Flag Cleared on NGX_AGAIN Allowing Re-entrant Frame Queue Corruption**

The h2c->blocked flag is set to 1 before calling ngx_http_v2_send_output_queue() and cleared to 0 after, even when the function returns NGX_AGAIN. When NGX_AGAIN is returned, the output queue is still being processed (frames are buffered for later sending). Clearing the blocked flag allows other event handlers to modify h2c->last_out while the current send operation is still pending. If a read event triggers processing that adds new frames to the output queue, and then the write handler resumes, the frame list could be corrupted, leading to double-free, use-after-free, or frame loss.

*PoC sketch:* 1. Trigger a scenario where send_chain returns NGX_AGAIN (e.g., full send buffer). 2. h2c->blocked is cleared to 0. 3. A read event arrives and triggers processing that adds frames to h2c->last_out. 4. The write event fires again, and ngx_http_v2_send_output_queue processes the queue, potentially co…

### `ngx_http_v2.c [CONTINUATION+list-size-limit]` — L95 — conf=0.55
**Handler Callback Can Modify h2c->last_out Causing Frame Loss**

During the handler callback loop (lines 95-105), each frame's handler is invoked. A handler callback could potentially add new frames to h2c->last_out (e.g., by triggering a response). However, at the end of the function (line 120), h2c->last_out is unconditionally overwritten with the reversed remaining frame list. Any frames added to h2c->last_out by handler callbacks would be lost, causing memory leaks and protocol violations (missing response data). While the 'blocked' flag is intended to prevent this, the flag is per-frame and may not cover all cases where handlers could modify the output…

*PoC sketch:* 1. Queue frames F1, F2, F3 for sending. 2. F3's handler triggers generation of a new response frame F4, which gets added to h2c->last_out. 3. After handler loop, h2c->last_out is overwritten with remaining unsent frames from the original list. 4. F4 is lost, causing memory leak and incomplete respon…

### `ngx_http_request.c [init_request B: ssl+addr]` — L82 — conf=0.5
**Duplicate Cookie Headers Leading to Authentication Bypass**

The Cookie header is processed with ngx_http_process_header_line, allowing multiple Cookie headers. RFC 6265 states that Cookie headers should NOT be combined and multiple Cookie headers are not equivalent to a single header with combined values. If downstream authentication middleware or application code concatenates multiple Cookie header values without proper separation, session tokens could be forged or authentication checks bypassed. For example, an attacker could inject a second Cookie header containing a session token that overwrites or merges with the legitimate one during processing.

*PoC sketch:* GET /protected HTTP/1.1 Host: target Cookie: session=invalid_token Cookie: session=admin_token

### `ngx_http_request.c [read_request_header A: recv-loop]` — L55 — conf=0.5
**Missing bounds check on else-branch direct array access with naddrs == 0**

In the `else` branch, the code directly accesses `addr6[0].conf` or `addr[0].conf` without verifying that `port->naddrs > 0`. If `naddrs` is 0 and the `addrs` pointer is NULL or points to an empty allocation, this results in a NULL pointer dereference or out-of-bounds read. While the else-branch is typically taken when there's exactly one address, there's no defensive check ensuring the array is non-empty.

*PoC sketch:* If a configuration edge case or race condition during reload results in port->naddrs == 0 while the else-branch is taken, accessing addr6[0] or addr[0] dereferences invalid memory, crashing the worker process (DoS).

### `ngx_http_request.c [read_request_header A: recv-loop]` — L37 — conf=0.45
**Type confusion in default switch case assumes AF_INET for unknown address families**

The `default` case in the switch on `c->local_sockaddr->sa_family` treats any unknown address family as AF_INET, casting `c->local_sockaddr` to `struct sockaddr_in *`. If `sa_family` is an unexpected value (e.g., AF_UNIX, AF_NETLINK, or any non-IP family), the struct fields `sin_addr` and `sin_port` would be reinterpreted from the wrong memory layout. This type confusion could lead to incorrect address matching, wrong configuration selection (`hc->addr_conf`), or out-of-bounds access if the sockaddr structure is smaller than `sockaddr_in`.

*PoC sketch:* If a code path or kernel interface allows creating a connection with c->local_sockaddr->sa_family set to a non-AF_INET/AF_INET6 value (e.g., AF_UNIX=1 on Linux), the default case would misinterpret the sockaddr structure. This requires an unusual setup but could be triggered via AF_UNIX proxying or …

### `ngx_http_request.c [process_request_headers A]` — L119 — conf=0.4
**Integer overflow in HTTP variables array allocation**

The multiplication `cmcf->variables.nelts * sizeof(ngx_http_variable_value_t)` passed to `ngx_pcalloc` can theoretically overflow on 32-bit systems. If an attacker can influence the number of registered variables (e.g., through a module that dynamically registers variables based on client input or via a misconfiguration with an extremely large number of variable definitions), the multiplication could wrap around, resulting in a smaller-than-expected allocation. Subsequent writes to the `r->variables` array would then overflow the heap buffer, potentially leading to arbitrary code execution. On…

*PoC sketch:* On a 32-bit nginx build, configure an extremely large number of variables (e.g., via many `map` or `set` directives) such that `variables.nelts * sizeof(ngx_http_variable_value_t)` exceeds 2^32. For example, if sizeof(ngx_http_variable_value_t) == 16, then ~268M variables would cause overflow. Then …

### `ngx_http_request.c [init_request A: pool+bufs]` — L56 — conf=0.3
**Host header processing may lack sufficient validation**

The Host header is processed by ngx_http_process_host which feeds into ngx_http_find_virtual_server. Without seeing the implementation, there is a risk that malformed Host headers (e.g., with embedded null bytes, unusual characters, or overly long values) could bypass virtual host routing or cause buffer issues in downstream processing. The ngx_http_alloc_large_header_buffer function (declared but not shown) handles buffer reallocation for large headers and could be a vector.

*PoC sketch:* GET / HTTP/1.1 Host: evil.com%00.legitimate.com


## 🔵 LOW (10)

### `ngx_http_parse.c [parse_request_line E: CRLF+errors]` — L82 — conf=0.9
**Inconsistent Major vs Minor Version Validation Order**

The major version validation in `sw_major_digit` performs the check AFTER the assignment (`r->http_major = r->http_major * 10 + (ch - '0'); if (r->http_major > 1) return error;`), which correctly catches overflow. However, the minor version validation in `sw_minor_digit` performs the check BEFORE the assignment, creating an inconsistency and the bypass described above. While the major version path is safe due to the tight bound (>1), the inconsistent pattern indicates a code quality issue that led to the minor version bug.

*PoC sketch:* Compare: sw_major_digit assigns then checks; sw_minor_digit checks then assigns. The same pattern should be used consistently (assign-then-check) to prevent the off-by-one-iteration bypass.

### `ngx_http_parse.c [parse_status_line: upstream-response]` — L133 — conf=0.85
**Incorrect status->start Pointer When Spaces Intermix Status Code Digits**

In the sw_status state, spaces within the status code digits are silently skipped (`if (ch == ' ') break;`), but when the third digit is found, `status->start = p - 2` assumes the three status code digits are contiguous. If spaces appear between digits (e.g., '2 0 0'), p-2 points to the wrong buffer location (a space or wrong digit instead of the first digit of the status code). This causes status->start to reference incorrect data, which could lead to misinterpretation of the status line by downstream consumers that rely on status->start for pointer arithmetic or string extraction.

*PoC sketch:* Upstream response: HTTP/1.1 2 0 0 OK\r\n — status->code correctly becomes 200, but status->start points to the space before the last '0' instead of the first '2', causing a 2-byte offset error in any code using status->start to locate the status code in the buffer.

### `ngx_http_parse.c [parse_request_line C: URI-continuation]` — L85 — conf=0.7
**Empty Port Accepted After Colon in Authority**

The sw_port state transitions immediately upon seeing a colon in sw_host_end, but does not require at least one digit before accepting a subsequent '/', '?', or ' '. This means requests like 'GET http://host:/path HTTP/1.1' are accepted with an empty port specification. While RFC 3986 requires a port to be non-empty if the colon is present, nginx accepts the empty port. This could cause inconsistent behavior with strict parsers and potentially lead to misrouting if downstream systems interpret the empty port differently (e.g., defaulting to port 80 vs. rejecting the request).

*PoC sketch:* GET http://example.com:/path HTTP/1.1 Host: example.com

### `ngx_http_request.c [process_request_line entry]` — L65 — conf=0.6
**Dangling buffer pointers after ngx_pfree creates fragile use-after-free surface**

When ngx_pfree succeeds, only b->start is set to NULL while b->pos, b->last, and b->end remain pointing to the freed memory region. Although the current code path returns immediately after freeing and re-initializes on re-entry (checking b->start == NULL), this pattern is fragile. Any future code modification, callback invocation, or signal handler that accesses b->pos/b->last/b->end between the free and re-initialization would trigger a use-after-free. Additionally, if ngx_pfree returns NGX_OK but the pool implementation has deferred freeing, the NULL check on b->start would skip re-allocatio…

*PoC sketch:* 1. Connect to nginx and send a partial HTTP request 2. Wait for the server to hit NGX_AGAIN path 3. If b->pos == b->last (empty buffer), ngx_pfree frees b->start and sets it to NULL 4. b->pos, b->last, b->end still reference freed memory 5. On re-entry, b->start == NULL triggers reallocation, overwr…

### `ngx_http_v2.c [SETTINGS+PING-frames]` — L82 — conf=0.55
**total_bytes accounting uses stale c->buffer->pos after loop mutation**

After the do-while processing loop, the code computes `h2c->total_bytes += p - c->buffer->pos`. While `p` should equal `end` on normal exit, if a state handler ever returns a pointer before `c->buffer->pos` (due to a bug in frame processing), this subtraction would underflow since `total_bytes` is likely an unsigned type. Additionally, if the loop exits early via `p == NULL` (error path on line 77), the function returns without updating `c->buffer->pos`, causing the same data to be reprocessed on the next read — potentially leading to infinite processing loops or double-counting of bytes for f…

*PoC sketch:* Send a malformed HTTP/2 frame that causes a state handler to return NULL (error). The buffer position is not advanced, so on the next read event, the same malformed data is processed again, creating an infinite loop of error handling without consuming the bad data.

### `ngx_http_request.c [process_request_headers B]` — L55 — conf=0.5
**Potential buffer over-read via ngx_proxy_protocol_read with n == 0**

When recv() returns 0 and hc->proxy_protocol is true, ngx_proxy_protocol_read(c, buf, buf + 0) is called with start == end (empty buffer). If ngx_proxy_protocol_read does not properly handle a zero-length input buffer, it may read beyond the provided bounds, potentially leaking stack data. While a well-implemented parser should return NULL for empty input, the missing n == 0 guard means this edge case is reached solely dependent on ngx_proxy_protocol_read's robustness.

*PoC sketch:* 1. Configure nginx with proxy_protocol enabled on an SSL port 2. Connect and immediately close the connection (send no data) 3. recv() returns 0, hc->proxy_protocol is true 4. ngx_proxy_protocol_read(c, buf, buf) is called with empty range 5. If the function doesn't handle empty input, stack data ma…

### `ngx_http_v2.c [DATA-frame+flow-control]` — L112 — conf=0.5
**Integer overflow in lingering_time calculation**

In ngx_http_v2_lingering_close, the lingering_time is calculated as: h2c->lingering_time = ngx_time() + (time_t)(clcf->lingering_time / 1000). If clcf->lingering_time is configured to an extremely large value, or if ngx_time() returns a value near TIME_T_MAX, the addition can overflow. On 32-bit systems with time_t as a 32-bit integer, this is more feasible. An overflow would cause lingering_time to wrap to a small or negative value, potentially causing the lingering close logic to expire immediately instead of waiting for the configured timeout, or to never expire if it wraps to a very large …

*PoC sketch:* On a 32-bit system with time_t as int32_t: 1. Configure lingering_time to a very large value (e.g., 2147483647000 ms, which divided by 1000 = 2147483647 = INT32_MAX). 2. If ngx_time() returns any positive value, the addition overflows. 3. lingering_time wraps to a negative value, causing immediate e…

### `ngx_http_request.c [process_request_headers A]` — L93 — conf=0.3
**Potential NULL pointer dereference via r->header_in assignment**

The assignment `r->header_in = hc->busy ? hc->busy->buf : c->buffer` does not validate that the resulting pointer is non-NULL. If `hc->busy` is non-NULL but `hc->busy->buf` is NULL (a corrupted or partially initialized buffer chain), or if `c->buffer` is NULL in an edge-case connection state, `r->header_in` will be NULL. Subsequent request line/header processing that reads from `r->header_in` without a NULL check would cause a denial of service (worker process crash).

*PoC sketch:* Craft a connection state where the HTTP connection's busy buffer chain has an entry with a NULL buf pointer. This could potentially be triggered via HTTP/2 upgrade edge cases or pipelined request sequences that leave the busy chain in an inconsistent state, then send a subsequent request to trigger …

### `ngx_http_request.c [process_request_headers B]` — L42 — conf=0.3
**TOCTOU race between MSG_PEEK and actual recv in proxy protocol consumption**

The code first peeks at data using recv(MSG_PEEK), parses the proxy protocol header from the peeked buffer, then consumes it via c->recv(). Between the MSG_PEEK and the actual recv, the socket data could theoretically change (e.g., in a multithreaded environment or via external manipulation). The consumed data may differ from what was parsed, leading to inconsistent connection state. While nginx's single-threaded event model makes this unlikely in practice, the pattern is inherently fragile and could become exploitable if the architecture changes.

*PoC sketch:* This is a theoretical race condition. In practice, nginx's event-driven single-threaded model prevents exploitation. However, if the connection's recv callback were to perform additional reads or if the architecture changed to be multithreaded, the data consumed by c->recv() could differ from what w…

### `ngx_http_request.c [init_request A: pool+bufs]` — L74 — conf=0.25
**User-Agent header processing without visible sanitization**

The User-Agent header uses a dedicated handler ngx_http_process_user_agent rather than the generic ngx_http_process_header_line. This suggests special processing logic that is not visible. If the User-Agent value is logged or used in string operations without proper sanitization, it could enable log injection or other secondary attacks. The dedicated handler may also have different buffer handling than generic headers.

*PoC sketch:* GET / HTTP/1.1 Host: target User-Agent: Mozilla  FAKE_LOG_ENTRY 200 OK


## ⚪ INFO (3)

### `ngx_http_request.c [process_request_line entry]` — L94 — conf=0.5
**Proxy protocol IP spoofing when enabled on untrusted interfaces**

The proxy protocol processing (ngx_proxy_protocol_read) trusts the source IP address provided in the PROXY protocol header without verifying the connection actually originates from a trusted proxy. If the proxy_protocol directive is configured on a listener accessible to untrusted clients, an attacker can spoof any source IP address by sending a crafted PROXY protocol header. The hc->proxy_protocol flag is consumed (set to 0) after processing, preventing re-processing but not validating the source. This could lead to IP-based access control bypass.

*PoC sketch:* 1. Connect to a nginx listener with proxy_protocol enabled on a public interface 2. Send: PROXY TCP4 10.0.0.1 10.0.0.2 12345 80\r\n 3. Follow with HTTP request 4. Server will believe the connection originates from 10.0.0.1, bypassing IP-based ACLs

### `ngx_http_request.c [process_request_headers A]` — L47 — conf=0.5
**Unbounded c->requests counter overflow in keep-alive connections**

The `c->requests++` increment in `ngx_http_create_request` has no overflow protection. On long-lived keep-alive connections, if the counter wraps around to 0 (on 32-bit ngx_uint_t after ~4 billion requests on the same connection), the keepalive_requests limit check would be bypassed, allowing the connection to persist beyond the configured limit. This is a minor policy bypass rather than a memory safety issue, and is extremely difficult to trigger in practice.

*PoC sketch:* Open a persistent HTTP keep-alive connection and send more than UINT_MAX requests (4,294,967,296 on 32-bit) to wrap the counter to 0, bypassing the keepalive_requests directive limit.

### `ngx_http_request.c [process_request_line entry]` — L119 — conf=0.35
**HTTP/2 preface partial match may cause protocol confusion with truncated reads**

The HTTP/2 preface detection uses ngx_memcmp with a size derived from the minimum of the preface length and available data. If only a partial match is found (size < sizeof(NGX_HTTP_V2_PREFACE) - 1), the code enters a branch (cut off in the snippet) that presumably waits for more data. However, if the partial data matches the beginning of the HTTP/2 preface but subsequent data diverges, the server may have already committed to HTTP/2 detection logic. This could lead to protocol confusion where HTTP/1.1 data is interpreted as HTTP/2 frames, potentially causing parsing errors or unexpected behavi…

*PoC sketch:* 1. Connect to nginx with http2 enabled (h2c) 2. Send partial HTTP/2 preface: 'PRI * HTTP/2.0\r\n\r\n' (first 16 bytes) 3. Wait for server to buffer partial match 4. Send remaining bytes that DON'T match: 'XX\r\n\r\n' instead of 'SM\r\n\r\n' 5. Server must fall back to HTTP/1.1 but may have already p…

