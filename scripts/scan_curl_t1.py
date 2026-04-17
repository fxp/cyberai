#!/usr/bin/env python3
"""curl T1 scan — GLM-5.1, function-level extracts.

Covers: urlapi.c, url.c, cookie.c, ftp.c, socks.c, http_chunks.c,
        mime.c, http_digest.c, curl_sasl.c, ftplistparser.c

Run after API quota resets (08:00 BJT daily):
    python scripts/scan_curl_t1.py
    python scripts/scan_curl_t1.py --start 3   # resume from segment index 3
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "curl"

SCANS = [
    ("urlapi_A.c",      "lib/urlapi.c [parseurl]"),
    ("urlapi_B.c",      "lib/urlapi.c [parse_hostname_login+ipv6+hostname_check]"),
    ("urlapi_C.c",      "lib/urlapi.c [curl_url_set+curl_url_get]"),
    ("url_A.c",         "lib/url.c [parseurlandfillconn+setup_range]"),
    ("cookie_A.c",      "lib/cookie.c [parse_cookie_header+sanitize+add]"),
    ("ftp_A.c",         "lib/ftp.c [match_pasv_6nums+ftp_state_pasv_resp]"),
    ("ftp_B.c",         "lib/ftp.c [ftp_state_use_port+use_pasv]"),
    ("socks_A.c",       "lib/socks.c [do_SOCKS4]"),
    ("socks_B.c",       "lib/socks.c [do_SOCKS5]"),
    ("http_chunks_A.c", "lib/http_chunks.c [httpchunk_readwrite]"),
    ("ftplist_A.c",     "lib/ftplistparser.c [ftp_pl_state_machine]"),
    ("mime_A.c",        "lib/mime.c [mime_read+boundary]"),
    ("http_digest_A.c", "lib/http_digest.c [output_digest+decode]"),
    ("sasl_A.c",        "lib/curl_sasl.c [decode_mech+sasl_continue]"),
]


async def scan_one(agent, fname: str, label: str, timeout: int = 600) -> dict:
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
    parser.add_argument("--start", type=int, default=0, help="Resume from segment index (0-based)")
    parser.add_argument("--delay", type=int, default=30, help="Seconds between scans")
    parser.add_argument("--timeout", type=int, default=600, help="Per-scan timeout seconds")
    args = parser.parse_args()

    import os
    from cyberai.models.glm import GLMAdapter
    api_key = os.environ.get("GLM_API_KEY", "GLM_KEY_REMOVED")
    agent = GLMAdapter(model_name="glm-5.1", api_key=api_key)

    results = []
    scans = SCANS[args.start:]
    print(f"Scanning {len(scans)} segments (starting at {args.start}), {args.delay}s delay\n")

    for i, (fname, label) in enumerate(scans):
        if i > 0:
            print(f"  [waiting {args.delay}s...]", flush=True)
            await asyncio.sleep(args.delay)
        r = await scan_one(agent, fname, label, timeout=args.timeout)
        results.append(r)
        if r["status"] == "timeout" and i == 0:
            print("\nFirst scan timed out — API quota likely exhausted. Try again later.")
            break

    out = pathlib.Path("research/curl/t1_glm5_results.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump({"model": "glm-5.1", "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                   "results": results}, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved → {out}")


if __name__ == "__main__":
    asyncio.run(main())
