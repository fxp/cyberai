#!/usr/bin/env python3
"""libssh2 1.11.1 T1 scan — GLM-5.1, function-level extracts.

Covers: src/packet.c (packet dispatch, 1658 lines)
        src/transport.c (SSH transport layer, 1306 lines)
        src/userauth.c (authentication, 2506 lines)
        src/sftp.c (SFTP protocol, 3974 lines)
        src/channel.c (channel management, 3075 lines)
        src/scp.c (SCP protocol, 1185 lines)

Historical CVEs covered (CVE-2019-38xx batch from libssh2 1.8.x):
- CVE-2019-3855: integer overflow in _libssh2_transport_read (packet_length)
- CVE-2019-3856: OOB read in keyboard-interactive request (num_prompts)
- CVE-2019-3857: OOB read in SSH_MSG_CHANNEL_REQUEST (_libssh2_packet_add)
- CVE-2019-3858: OOB read in sftp_packet_add (length field)
- CVE-2019-3859: NULL ptr dereference in _libssh2_packet_require
- CVE-2019-3860: OOB read in SSH_MSG_CHANNEL_REQUEST extra data
- CVE-2019-3861: OOB read in SSH_MSG_DISCONNECT message
- CVE-2019-3862: OOB read in SSH_MSG_CHANNEL_DATA
- CVE-2019-3863: integer overflow in userauth_list response
- Scanning for NEW issues in 1.11.1

Run after API quota resets (08:00 BJT daily):
    python scripts/scan_libssh2_t1.py
    python scripts/scan_libssh2_t1.py --start 10   # resume from index 10
"""
from __future__ import annotations
import asyncio, sys, time, pathlib, argparse, json

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent / "src"))

EXTRACTS_DIR = pathlib.Path(__file__).parent / "extracts" / "libssh2"

SCANS = [
    # ── packet.c: channel open requests ──
    ("packet_listener_A.c",  "libssh2/packet.c [packet_queue_listener A: channel + window L69-170]"),
    ("packet_listener_B.c",  "libssh2/packet.c [packet_queue_listener B: host+port+failure L170-271]"),
    # ── packet.c: _libssh2_packet_add (CVE-2019-3857/3860/3861/3862 area) ──
    ("packet_add_A.c",       "libssh2/packet.c [_libssh2_packet_add A: dispatch+CHANNEL_DATA L634-729]"),
    ("packet_add_B.c",       "libssh2/packet.c [_libssh2_packet_add B: CHANNEL_EXTENDED_DATA L729-827]"),
    ("packet_add_C.c",       "libssh2/packet.c [_libssh2_packet_add C: CHANNEL_EOF+CLOSE L827-925]"),
    ("packet_add_D.c",       "libssh2/packet.c [_libssh2_packet_add D: CHANNEL_REQUEST L925-1021]"),
    ("packet_add_E.c",       "libssh2/packet.c [_libssh2_packet_add E: CHANNEL_REQUEST types L1021-1112]"),
    ("packet_add_F.c",       "libssh2/packet.c [_libssh2_packet_add F: DISCONNECT+IGNORE L1112-1190]"),
    ("packet_add_G.c",       "libssh2/packet.c [_libssh2_packet_add G: SERVICE_ACCEPT+BANNER L1190-1287]"),
    ("packet_add_H.c",       "libssh2/packet.c [_libssh2_packet_add H: GLOBAL_REQUEST+callback L1287-1395]"),
    # ── transport.c: MAC + decrypt (fullpacket) ──
    ("transport_full_A.c",   "libssh2/transport.c [fullpacket A: decrypt+MAC verify L184-261]"),
    ("transport_full_B.c",   "libssh2/transport.c [fullpacket B: decompression+dispatch L261-349]"),
    # ── transport.c: _libssh2_transport_read (CVE-2019-3855 integer overflow) ──
    ("transport_read_A.c",   "libssh2/transport.c [_libssh2_transport_read A: recv+packet_len L367-467]"),
    ("transport_read_B.c",   "libssh2/transport.c [_libssh2_transport_read B: padding+seq L467-569]"),
    ("transport_read_C.c",   "libssh2/transport.c [_libssh2_transport_read C: socket read loop L569-671]"),
    ("transport_read_D.c",   "libssh2/transport.c [_libssh2_transport_read D: payload assemble L671-776]"),
    ("transport_read_E.c",   "libssh2/transport.c [_libssh2_transport_read E: MAC check+return L776-900]"),
    # ── userauth.c: list response (CVE-2019-3863 integer overflow) ──
    ("userauth_list_A.c",    "libssh2/userauth.c [userauth_list A: send+recv parse L67-154]"),
    ("userauth_list_B.c",    "libssh2/userauth.c [userauth_list B: method string parse L154-229]"),
    # ── userauth.c: keyboard-interactive (CVE-2019-3856 num_prompts OOB) ──
    ("userauth_kbd_A.c",     "libssh2/userauth.c [keyboard_interactive A: request send L293-382]"),
    ("userauth_kbd_B.c",     "libssh2/userauth.c [keyboard_interactive B: prompt recv+count L382-455]"),
    ("userauth_kbd_C.c",     "libssh2/userauth.c [keyboard_interactive C: response send L455-540]"),
    # ── userauth.c: pubkey authentication ──
    ("userauth_pk_A.c",      "libssh2/userauth.c [libssh2_sign_sk: sk key signing L905-1001]"),
    ("userauth_pk_B.c",      "libssh2/userauth.c [publickey_auth A: trial+build L1014-1099]"),
    ("userauth_pk_C.c",      "libssh2/userauth.c [publickey_auth B: sign+send L1099-1181]"),
    # ── sftp.c: packet layer (CVE-2019-3858 OOB read in sftp_packet_add) ──
    ("sftp_packet_A.c",      "libssh2/sftp.c [sftp_packet_add A: length check+alloc L173-302]"),
    ("sftp_packet_B.c",      "libssh2/sftp.c [sftp_packet_read B: type+chan dispatch L302-415]"),
    # ── sftp.c: read/write operations ──
    ("sftp_read_A.c",        "libssh2/sftp.c [sftp_read A: request send+state L1402-1495]"),
    ("sftp_read_B.c",        "libssh2/sftp.c [sftp_read B: data recv+buffer fill L1495-1584]"),
    ("sftp_write_A.c",       "libssh2/sftp.c [sftp_write A: data fragment+queue L2083-2188]"),
    ("sftp_write_B.c",       "libssh2/sftp.c [sftp_write B: ack recv+window update L2188-2293]"),
    # ── channel.c: read/write with window accounting ──
    ("channel_read_A.c",     "libssh2/channel.c [_libssh2_channel_read A: adjust window L2073-2155]"),
    ("channel_read_B.c",     "libssh2/channel.c [_libssh2_channel_read B: data dequeue L2155-2226]"),
    ("channel_write_A.c",    "libssh2/channel.c [_libssh2_channel_write A: send split L2338-2409]"),
    ("channel_write_B.c",    "libssh2/channel.c [_libssh2_channel_write B: flow control L2409-2469]"),
    # ── scp.c: path + size parsing (path traversal + OOB read risk) ──
    ("scp_recv_A.c",         "libssh2/scp.c [scp_recv A: banner+path parse L281-389]"),
    ("scp_recv_B.c",         "libssh2/scp.c [scp_recv B: file mode+size parse L389-484]"),
    ("scp_recv_C.c",         "libssh2/scp.c [scp_recv C: timestamp+mtime parse L484-574]"),
    ("scp_recv_D.c",         "libssh2/scp.c [scp_recv D: data recv+ack L574-665]"),
    ("scp_recv_E.c",         "libssh2/scp.c [scp_recv E: close+error handling L665-766]"),
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
    api_key = os.environ.get("GLM_API_KEY", "GLM_KEY_REMOVED")
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

    out = pathlib.Path("research/libssh2/t1_glm5_results.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump({"model": "glm-5.1", "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                   "target": "libssh2-1.11.1", "results": results}, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved → {out}")


if __name__ == "__main__":
    asyncio.run(main())
