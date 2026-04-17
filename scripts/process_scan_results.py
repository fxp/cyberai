#!/usr/bin/env python3
"""process_scan_results.py — 扫描结果后处理 + 报告更新

在扫描批次完成后运行:
  1. 读取 research/{target}/t1_glm5_results.json
  2. 统计新发现数量
  3. 将 GLM findings 追加写入对应的 vulnerability_report.md
  4. 打印需要手动 triage 的候选列表 (confidence ≥ 65%)
  5. 生成 git commit 建议

Usage:
  python scripts/process_scan_results.py --target libpng
  python scripts/process_scan_results.py --target expat
  python scripts/process_scan_results.py --target curl
  python scripts/process_scan_results.py --all
"""
from __future__ import annotations
import json, pathlib, argparse, sys, time
from datetime import datetime

RESEARCH = pathlib.Path("research")
TARGETS = {
    "libpng": RESEARCH / "libpng",
    "expat":  RESEARCH / "expat",
    "curl":   RESEARCH / "curl",
}

# Confidence threshold for manual triage flagging
TRIAGE_THRESHOLD = 0.65
HIGH_CONFIDENCE   = 0.80


def load_results(target: str) -> dict | None:
    result_file = TARGETS[target] / "t1_glm5_results.json"
    if not result_file.exists():
        print(f"  ⚠️  No results file found: {result_file}")
        return None
    return json.loads(result_file.read_text())


def summarize(target: str, data: dict) -> None:
    results = data.get("results", [])
    timestamp = data.get("timestamp", "unknown")

    total_findings = 0
    triage_candidates = []
    all_findings = []

    ok = sum(1 for r in results if r.get("status") == "ok")
    failed = sum(1 for r in results if r.get("status") != "ok")

    for seg in results:
        if seg.get("status") != "ok":
            continue
        for f in seg.get("findings", []):
            total_findings += 1
            conf = f.get("confidence", 0)
            finding_info = {
                "segment": seg["file"],
                "severity": f.get("severity", "?"),
                "title": f.get("title", "?"),
                "line": f.get("line_start", 0),
                "confidence": conf,
                "description": f.get("description", "")[:120],
            }
            all_findings.append(finding_info)
            if conf >= TRIAGE_THRESHOLD:
                triage_candidates.append(finding_info)

    print(f"\n{'='*60}")
    print(f"TARGET: {target.upper()}")
    print(f"{'='*60}")
    print(f"Scan timestamp : {timestamp}")
    print(f"Segments OK    : {ok}/{len(results)} ({failed} failed/timeout)")
    print(f"Total findings : {total_findings}")
    print(f"Triage (≥{TRIAGE_THRESHOLD:.0%}): {len(triage_candidates)}")

    if not triage_candidates:
        print("→ No high-confidence findings requiring manual triage.")
        return

    # Sort by severity + confidence
    sev_order = {"critical": 0, "high": 1, "medium": 2, "low": 3, "info": 4}
    triage_candidates.sort(key=lambda x: (
        sev_order.get(x["severity"].lower(), 5), -x["confidence"]
    ))

    print("\n── Manual Triage Required ──")
    for i, f in enumerate(triage_candidates, 1):
        flag = "🔴" if f["confidence"] >= HIGH_CONFIDENCE else "🟡"
        print(f"\n  [{i}] {flag} [{f['severity'].upper()}] {f['title']}")
        print(f"      Segment: {f['segment']}")
        print(f"      L{f['line']}, conf={f['confidence']:.0%}")
        if f["description"]:
            print(f"      {f['description'][:100]}...")

    return triage_candidates


def update_vulnerability_report(target: str, data: dict) -> None:
    """Append a GLM findings triage table to the vulnerability report."""
    report_path = TARGETS[target] / "vulnerability_report.md"
    if not report_path.exists():
        print(f"  ⚠️  Report not found: {report_path}")
        return

    results = data.get("results", [])
    timestamp = data.get("timestamp", "unknown")

    # Build triage table
    rows = []
    total_findings = 0
    for seg in results:
        label = seg.get("file", "?")
        status = "✅" if seg.get("status") == "ok" else "❌ timeout"
        findings = seg.get("findings", [])
        total_findings += len(findings)
        key_finding = ""
        if findings:
            f = sorted(findings, key=lambda x: -x.get("confidence", 0))[0]
            key_finding = f"[{f['severity'].upper()}] {f['title'][:50]} (conf={f['confidence']:.0%})"
        elapsed = f"{seg.get('elapsed_s', 0):.0f}s" if seg.get("status") == "ok" else "-"
        rows.append(f"| {label} | {status} {elapsed} | {len(findings)} | {key_finding} |")

    table = f"""
---

## GLM-5.1 扫描结果 (T1) — {timestamp}

**统计**: {sum(1 for r in results if r.get('status')=='ok')}/{len(results)} 段成功 | {total_findings} 个原始 findings

| 段 | 状态 | Findings | 最高置信度发现 |
|----|------|---------|--------------|
""" + "\n".join(rows)

    current = report_path.read_text()
    # Avoid appending duplicate tables
    if f"GLM-5.1 扫描结果 (T1) — {timestamp}" in current:
        print(f"  ℹ️  Table for {timestamp} already in report, skipping.")
        return

    report_path.write_text(current.rstrip() + "\n" + table + "\n")
    print(f"  ✅ Updated: {report_path}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", choices=list(TARGETS.keys()))
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--update-report", action="store_true", default=True)
    args = parser.parse_args()

    targets_to_process = list(TARGETS.keys()) if args.all else ([args.target] if args.target else [])

    if not targets_to_process:
        print("Specify --target <name> or --all")
        sys.exit(1)

    for target in targets_to_process:
        data = load_results(target)
        if not data:
            continue
        summarize(target, data)
        if args.update_report:
            update_vulnerability_report(target, data)

    print("\n── Next steps ──")
    print("1. Review triage candidates above (🔴 = high priority)")
    print("2. Read source context: targets/<target>/lib/ ±50 lines around each finding")
    print("3. Confirm or dismiss; write PoC for confirmed HIGH findings")
    print("4. Update vulnerability_report.md with Confirmed/FP labels")
    print("5. git add research/ && git commit -m 'triage: ...'")


if __name__ == "__main__":
    main()
