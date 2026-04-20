#!/usr/bin/env python3
"""nginx 1.27.4 T1 scan — GLM-5.1, function-level extracts.

Covers: ngx_http_parse.c (request line, headers, URI, chunked)
        ngx_http_request.c (request init, header processing)
        ngx_http_v2.c (HTTP/2 frame parsing)
        ngx_http_proxy_module.c (upstream header forwarding)

Historical CVEs covered:
- CVE-2013-4547: Crafted URI with space → bypass restrictions (parse_complex_uri)
- CVE-2017-7529: Range header integer overflow → info disclosure (ngx_http_range)
- CVE-2021-23017: Off-by-one in resolver, not in parser files scanned here
- CVE-2022-41741/41742: ngx_http_mp4_module OOB — not in scope here
- Scanning for NEW issues in parsing/proxy/HTTP2 code

Run after API quota resets (08:00 BJT daily):
    python scripts/scan_nginx_t1.py
    python scripts/scan_nginx_t1.py --start 10   # resume from index 10
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "nginx"

SCANS = [
    # ── HTTP Request Line Parser (highest CVE history) ──
    ("parse_request_line_A.c", "ngx_http_parse.c [parse_request_line A: method-states]"),
    ("parse_request_line_B.c", "ngx_http_parse.c [parse_request_line B: URI-path]"),
    ("parse_request_line_C.c", "ngx_http_parse.c [parse_request_line C: URI-continuation]"),
    ("parse_request_line_D.c", "ngx_http_parse.c [parse_request_line D: HTTP-version]"),
    ("parse_request_line_E.c", "ngx_http_parse.c [parse_request_line E: CRLF+errors]"),
    # ── Header Line Parser ──
    ("parse_header_line_A.c",  "ngx_http_parse.c [parse_header_line A: name+colon]"),
    ("parse_header_line_B.c",  "ngx_http_parse.c [parse_header_line B: value+CRLF]"),
    # ── URI Parsing (% encoding, dot-segments — CVE-2013-4547 class) ──
    ("parse_complex_uri_A.c",  "ngx_http_parse.c [parse_complex_uri A: %-decode]"),
    ("parse_complex_uri_B.c",  "ngx_http_parse.c [parse_complex_uri B: dot-segments]"),
    ("parse_complex_uri_C.c",  "ngx_http_parse.c [parse_complex_uri C: query+args]"),
    ("parse_unsafe_uri.c",     "ngx_http_parse.c [parse_unsafe_uri: traversal-detect]"),
    # ── Chunked Transfer Encoding ──
    ("parse_chunked_A.c",      "ngx_http_parse.c [parse_chunked A: hex-size-accum]"),
    ("parse_chunked_B.c",      "ngx_http_parse.c [parse_chunked B: data+trailer]"),
    # ── Upstream Status Line ──
    ("parse_status_line.c",    "ngx_http_parse.c [parse_status_line: upstream-response]"),
    # ── Request Processing (ngx_http_request.c) ──
    ("request_init_A.c",       "ngx_http_request.c [init_request A: pool+bufs]"),
    ("request_init_B.c",       "ngx_http_request.c [init_request B: ssl+addr]"),
    ("request_read_A.c",       "ngx_http_request.c [read_request_header A: recv-loop]"),
    ("request_read_B.c",       "ngx_http_request.c [process_request_line entry]"),
    ("request_headers_A.c",    "ngx_http_request.c [process_request_headers A]"),
    ("request_headers_B.c",    "ngx_http_request.c [process_request_headers B]"),
    # ── HTTP/2 Frame Parsing (ngx_http_v2.c) ──
    ("v2_preface.c",           "ngx_http_v2.c [connection-init+preface]"),
    ("v2_frame_A.c",           "ngx_http_v2.c [frame-header+length-check]"),
    ("v2_frame_A2.c",          "ngx_http_v2.c [frame-type-dispatch+stream-lookup]"),
    ("v2_frame_B.c",           "ngx_http_v2.c [SETTINGS+PING-frames]"),
    ("v2_headers_A.c",         "ngx_http_v2.c [HEADERS-frame+hpack-decode]"),
    ("v2_headers_B.c",         "ngx_http_v2.c [CONTINUATION+list-size-limit]"),
    ("v2_data.c",              "ngx_http_v2.c [DATA-frame+flow-control]"),
    # ── Proxy Module (ngx_http_proxy_module.c) ──
    ("proxy_headers_A.c",      "ngx_http_proxy.c [hop-by-hop-filter]"),
    ("proxy_headers_B.c",      "ngx_http_proxy.c [upstream-header-copy]"),
    ("proxy_headers_C.c",      "ngx_http_proxy.c [response-header-rewrite]"),
    ("proxy_create_req_A.c",   "ngx_http_proxy.c [create_request: request-line]"),
    ("proxy_create_req_B.c",   "ngx_http_proxy.c [create_request: Host+X-Forwarded-For]"),
    ("proxy_create_req_C.c",   "ngx_http_proxy.c [create_request: body+auth]"),
]


async def scan_one(agent, fname: str, label: str, timeout: int = 150) -> dict:
    content = (EXTRACTS_DIR / fname).read_text()
    print(f"[{time.strftime('%H:%M:%S')}] {label} ({len(content):,} chars)...", flush=True)
    t0 = time.time()
    try:
        result = await asyncio.wait_for(
            agent.scan_file(content, label, timeout=float(timeout)), timeout=timeout + 30)
        elapsed = time.time() - t0
        print(f"  Done {elapsed:.0f}s — {result.vuln_count} findings", flush=True)
        for f in result.findings:
            print(f"  [{f.severity.value.upper()}] {f.title} (L{f.line_start}, conf={f.confidence:.0%})", flush=True)
        if result.error:
            print(f"  Error: {result.error}", flush=True)
        return {
            "file": label, "chars": len(content),
            "elapsed_s": round(elapsed, 1), "status": "ok",
            "findings": [
                {"severity": f.severity.value, "title": f.title,
                 "line_start": f.line_start, "confidence": f.confidence,
                 "description": f.description, "poc": f.proof_of_concept}
                for f in result.findings
            ],
        }
    except asyncio.TimeoutError:
        print(f"  TIMEOUT {time.time()-t0:.0f}s", flush=True)
        return {"file": label, "chars": len(content), "status": "timeout", "findings": []}
    except Exception as e:
        print(f"  Error: {e}", flush=True)
        return {"file": label, "chars": len(content), "status": f"error:{e}", "findings": []}


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--start", type=int, default=0)
    parser.add_argument("--delay", type=int, default=30)
    parser.add_argument("--timeout", type=int, default=150)
    args = parser.parse_args()

    import os
    from cyberai.models.glm import GLMAdapter
    api_key = os.environ.get("GLM_API_KEY", "")
    agent = GLMAdapter(model_name="glm-5.1", api_key=api_key)

    results = []
    scans = SCANS[args.start:]
    print(f"Scanning {len(scans)} segments (start={args.start}), {args.delay}s delay\n")

    for i, (fname, label) in enumerate(scans):
        if i > 0:
            print(f"  [waiting {args.delay}s...]", flush=True)
            await asyncio.sleep(args.delay)
        r = await scan_one(agent, fname, label, timeout=args.timeout)
        results.append(r)

    out = pathlib.Path("research/nginx/t1_glm5_results.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump({"model": "glm-5.1", "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                   "target": "nginx-1.27.4", "results": results}, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved → {out}")


if __name__ == "__main__":
    asyncio.run(main())
