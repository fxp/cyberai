#!/usr/bin/env python3
"""OpenSSL 3.4.1 T1 scan — GLM-5.1, function-level extracts.

Covers key security-critical files:
  ssl/record/methods/ssl3_cbc.c  — CBC padding, Lucky13 mitigation
  crypto/asn1/tasn_dec.c         — ASN.1 DER decoder (highest CVE density)
  crypto/asn1/a_int.c            — ASN.1 INTEGER parsing
  crypto/dh/dh_key.c             — DH key operations
  ssl/statem/extensions.c        — TLS extensions parsing
  ssl/statem/statem_lib.c        — TLS handshake message header/body

Historical CVEs covered:
- CVE-2014-0160 (Heartbleed): ssl_record reading — NOT in these files
- CVE-2022-0778: Infinite loop in BN_mod_sqrt — crypto/bn/ (not in T1)
- CVE-2022-3602/3786: Stack overflow punycode — not in scope
- CVE-2023-0215: UAF in BIO_new_NDEF — not in scope
- CVE-2023-0286: Invalid ptr X.509 GeneralName — tasn_dec context
- CVE-2023-3817: Excessive allocation DH — dh_key.c
- CVE-2023-5678: DH key generation failure — dh_key.c
- Scanning for NEW issues in ASN.1/TLS/DH code

Run after API quota resets (08:00 BJT daily):
    python scripts/scan_openssl_t1.py
    python scripts/scan_openssl_t1.py --start 10   # resume from index 10
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "openssl"

SCANS = [
    # ── ASN.1 DER Decoder (highest CVE density in OpenSSL) ──
    ("tasn_dec_A.c",  "openssl/tasn_dec.c [ASN1_item_d2i + asn1_item_embed_d2i entry]"),
    ("tasn_dec_B.c",  "openssl/tasn_dec.c [asn1_item_embed_d2i A: SEQUENCE decode]"),
    ("tasn_dec_C.c",  "openssl/tasn_dec.c [asn1_item_embed_d2i B: SET/CHOICE decode]"),
    ("tasn_dec_D.c",  "openssl/tasn_dec.c [asn1_item_embed_d2i C: template+length]"),
    ("tasn_dec_I.c",  "openssl/tasn_dec.c [asn1_item_embed_d2i D: EXPLICIT+primitive]"),
    ("tasn_dec_J.c",  "openssl/tasn_dec.c [asn1_template_ex_d2i+noexp_d2i]"),
    ("tasn_dec_E.c",  "openssl/tasn_dec.c [asn1_d2i_ex_primitive: DER primitive decoder]"),
    ("tasn_dec_F.c",  "openssl/tasn_dec.c [asn1_ex_c2i: content byte decode]"),
    ("tasn_dec_G.c",  "openssl/tasn_dec.c [asn1_find_end+asn1_collect: indefinite length]"),
    ("tasn_dec_H.c",  "openssl/tasn_dec.c [asn1_check_tlen+collect_data: tag+len check]"),
    # ── ASN.1 INTEGER (CVE-2023-0286 context) ──
    ("a_int_A.c",     "openssl/a_int.c [ASN1_INTEGER_cmp+c2i_ASN1_INTEGER entry]"),
    ("a_int_B.c",     "openssl/a_int.c [c2i_ASN1_INTEGER body+sign handling]"),
    ("a_int_C.c",     "openssl/a_int.c [asn1_get_uint64+asn1_get_int64: overflow guards]"),
    ("a_int_D.c",     "openssl/a_int.c [asn1_string_get/set_uint64+i2c_ASN1_INTEGER]"),
    ("a_int_E.c",     "openssl/a_int.c [ASN1_INTEGER_set+bn conversions]"),
    # ── CBC Record Layer (Lucky13 mitigation) ──
    ("ssl3_cbc_A.c",  "openssl/ssl3_cbc.c [CBC padding+Lucky13 header]"),
    ("ssl3_cbc_B.c",  "openssl/ssl3_cbc.c [ssl3_cbc_digest_record A: HMAC setup]"),
    ("ssl3_cbc_C.c",  "openssl/ssl3_cbc.c [ssl3_cbc_digest_record B: MAC blocks]"),
    ("ssl3_cbc_D.c",  "openssl/ssl3_cbc.c [ssl3_cbc_digest_record C: output]"),
    ("ssl3_cbc_E.c",  "openssl/ssl3_cbc.c [ssl3_cbc_digest_record D: padding verify]"),
    # ── DH Key Operations (CVE-2023-5678, CVE-2023-3817) ──
    ("dh_key_A.c",    "openssl/dh_key.c [DH_compute_key+DH_compute_key_padded (CVE-2023-5678)]"),
    ("dh_key_B.c",    "openssl/dh_key.c [DH_compute_key body+public key validation]"),
    ("dh_key_C.c",    "openssl/dh_key.c [dh_bn_mod_exp+DH_generate_key entry]"),
    ("dh_key_D.c",    "openssl/dh_key.c [generate_key: parameter checks]"),
    ("dh_key_E.c",    "openssl/dh_key.c [generate_key: FIPS validation+BN ops]"),
    # ── TLS Extensions Parsing ──
    ("ext_parse_A.c", "openssl/extensions.c [tls_parse_extension+tls_parse_all_extensions A]"),
    ("ext_parse_B.c", "openssl/extensions.c [tls_parse_all_extensions B: context check loop]"),
    ("ext_parse_C.c", "openssl/extensions.c [tls_collect_extensions+tls_check_client_hello]"),
    # ── TLS Handshake Message Parsing ──
    ("statem_msg_A.c","openssl/statem_lib.c [tls_get_message_header: handshake header parse]"),
    ("statem_msg_B.c","openssl/statem_lib.c [tls_get_message_body A: message body read]"),
    ("statem_msg_C.c","openssl/statem_lib.c [ssl3_finish_mac+ssl3_take_mac: MAC ops]"),
    ("statem_msg_D.c","openssl/statem_lib.c [tls_get_message_body B: CCS+verify]"),
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

    out = pathlib.Path("research/openssl/t1_glm5_results.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump({"model": "glm-5.1", "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                   "target": "openssl-3.4.1", "results": results}, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved → {out}")


if __name__ == "__main__":
    asyncio.run(main())
