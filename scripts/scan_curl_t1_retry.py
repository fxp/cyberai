#!/usr/bin/env python3
"""curl T1 retry scan — for segments that failed with server disconnect.

Uses reduced timeout to avoid the ~300s API server timeout.
Run this after the main scan finishes, for any segments marked as error/timeout.

Usage:
    python scripts/scan_curl_t1_retry.py --segments 0,1   # retry segments 0 and 1
    python scripts/scan_curl_t1_retry.py --failed          # auto-detect failed segments from JSON
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "curl"
RESULTS_FILE = pathlib.Path("research/curl/t1_glm5_results.json")

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


async def scan_one(agent, fname: str, label: str, timeout: int = 240) -> dict:
    content = (EXTRACTS_DIR / fname).read_text()
    print(f"[{time.strftime('%H:%M:%S')}] RETRY {label} ({len(content):,} chars, timeout={timeout}s)...", flush=True)
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
        print(f"  TIMEOUT {time.time()-t0:.0f}s — giving up", flush=True)
        return {"file": label, "chars": len(content), "status": "timeout", "findings": []}
    except Exception as e:
        print(f"  Error: {e}", flush=True)
        return {"file": label, "chars": len(content), "status": f"error:{e}", "findings": []}


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--segments", type=str, default="",
                        help="Comma-separated segment indices to retry (e.g. 0,1,3)")
    parser.add_argument("--failed", action="store_true",
                        help="Auto-detect failed segments from existing results JSON")
    parser.add_argument("--delay", type=int, default=60,
                        help="Seconds between retries (default 60 to reduce server load)")
    parser.add_argument("--timeout", type=int, default=240,
                        help="Per-scan timeout (default 240s < server 300s limit)")
    args = parser.parse_args()

    # Determine which segments to retry
    retry_indices = []
    if args.failed and RESULTS_FILE.exists():
        data = json.loads(RESULTS_FILE.read_text())
        for i, r in enumerate(data.get("results", [])):
            if r.get("status", "ok") != "ok":
                retry_indices.append(i)
        print(f"Auto-detected failed segments: {retry_indices}")
    elif args.segments:
        retry_indices = [int(x.strip()) for x in args.segments.split(",")]
    else:
        print("Specify --segments 0,1 or --failed")
        return

    import os
    from cyberai.models.glm import GLMAdapter
    api_key = os.environ.get("GLM_API_KEY", "GLM_KEY_REMOVED")
    agent = GLMAdapter(model_name="glm-5.1", api_key=api_key)

    new_results = {}
    for i, idx in enumerate(retry_indices):
        if i > 0:
            print(f"  [waiting {args.delay}s...]", flush=True)
            await asyncio.sleep(args.delay)
        fname, label = SCANS[idx]
        r = await scan_one(agent, fname, label, timeout=args.timeout)
        new_results[idx] = r

    # Merge with existing results
    if RESULTS_FILE.exists():
        data = json.loads(RESULTS_FILE.read_text())
        results = data.get("results", [])
        for idx, r in new_results.items():
            if idx < len(results):
                results[idx] = r
            else:
                results.append(r)
        data["results"] = results
        data["retry_timestamp"] = time.strftime("%Y-%m-%dT%H:%M:%S")
    else:
        data = {"model": "glm-5.1", "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                "results": list(new_results.values())}

    RESULTS_FILE.parent.mkdir(parents=True, exist_ok=True)
    with open(RESULTS_FILE, "w") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    print(f"\nRetry results merged → {RESULTS_FILE}")


if __name__ == "__main__":
    asyncio.run(main())
