#!/usr/bin/env python3
"""CyberAI Research Scanner — Real-world vulnerability research.

Usage:
    python scripts/research_scan.py --target /root/research/ImageMagick --batch T1 --max 5
    python scripts/research_scan.py --target /root/research/libtiff --batch T1 --max 5
    python scripts/research_scan.py --target /root/research/mosquitto --max 8

Results are saved to /root/cyberai_research/<project>_<batch>_<timestamp>.json
"""
from __future__ import annotations

import argparse
import asyncio
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from rich.console import Console
from rich.panel import Panel
from rich.table import Table

console = Console()

# T1 priority files per project (override automatic ranking when specified)
T1_OVERRIDES: dict[str, list[str]] = {
    "imagemagick": [
        "coders/png.c",
        "coders/tiff.c",
        "coders/psd.c",
        "coders/svg.c",
        "coders/gif.c",
    ],
    "libtiff": [
        "libtiff/tif_dirread.c",
        "libtiff/tif_read.c",
        "libtiff/tif_pixarlog.c",
        "libtiff/tif_luv.c",
        "libtiff/tif_getimage.c",
    ],
    "mosquitto": [
        "src/read_handle.c",
        "src/packet_mosq.c",
        "src/handle_publish.c",
        "src/handle_connect.c",
        "src/net_mosq.c",
    ],
}

# Known CVEs per file for detection rate tracking (selected high-confidence examples)
KNOWN_CVES: dict[str, list[dict]] = {
    "coders/png.c": [
        {"cve": "CVE-2022-44268", "type": "information_disclosure", "desc": "arbitrary file read via tEXt chunk"},
        {"cve": "CVE-2017-12691", "type": "memory_allocation", "desc": "memory exhaustion via malformed PNG"},
    ],
    "coders/tiff.c": [
        {"cve": "CVE-2022-32546", "type": "integer_overflow", "desc": "integer overflow in TIFFGetField"},
    ],
    "coders/psd.c": [
        {"cve": "CVE-2021-20176", "type": "division_by_zero", "desc": "divide by zero in PSD parser"},
        {"cve": "CVE-2020-27829", "type": "heap_overflow", "desc": "heap overflow in PSD layer parsing"},
    ],
    "coders/gif.c": [
        {"cve": "CVE-2019-13391", "type": "heap_buffer_overflow", "desc": "heap buffer overflow in GIF loader"},
    ],
}


def smart_truncate(content: str, max_chars: int = 12_000) -> tuple[str, bool]:
    """Intelligently truncate large C files to focus on vulnerability-prone code.

    Strategy: skip large comment blocks at the top, extract code starting from
    the first function definition, keep the most relevant portion.
    For very large files, also sample from the middle where ReadXXXImage() tends to be.
    """
    if len(content) <= max_chars:
        return content, False

    lines = content.splitlines(keepends=True)
    total_lines = len(lines)

    # Find first real code line (skip license header / includes block)
    code_start = 0
    in_block_comment = False
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("/*"):
            in_block_comment = True
        if in_block_comment:
            if "*/" in stripped:
                in_block_comment = False
            continue
        # First non-comment, non-empty, non-include line that looks like code
        if stripped and not stripped.startswith("//") and not stripped.startswith("#include"):
            code_start = i
            break

    # Sample: beginning of real code + middle section + end
    chunk = max_chars // 3
    start_content = "".join(lines[code_start:code_start + 200])[:chunk]
    mid = total_lines // 2
    mid_content = "".join(lines[mid:mid + 200])[:chunk]
    end_content = "".join(lines[-150:])[:chunk]

    result = (
        f"// === [FILE START (lines {code_start+1}-{code_start+200})] ===\n"
        + start_content
        + f"\n\n// === [MIDDLE SECTION (lines {mid+1}-{mid+200})] ===\n"
        + mid_content
        + f"\n\n// === [FILE END (last 150 lines)] ===\n"
        + end_content
    )
    return result, True


def _extract_code_context(filepath: Path, line_start: int, line_end: int, radius: int = 50) -> str:
    """Return ±radius lines around a finding as a formatted code snippet."""
    try:
        lines = filepath.read_text(errors="ignore").splitlines()
    except OSError:
        return ""
    total = len(lines)
    lo = max(0, line_start - radius - 1)
    hi = min(total, line_end + radius)
    snippet = "\n".join(f"{i+1:5d}: {lines[i]}" for i in range(lo, hi))
    return f"Source context ({filepath.name} lines {lo+1}-{hi}):\n```c\n{snippet}\n```"


async def verify_finding(agent, finding, filepath: Path, delay: float = 15.0) -> dict:
    """Second-pass verification for a single finding with source code context."""
    import asyncio
    from cyberai.models.base import VulnFinding

    context = _extract_code_context(filepath, finding["line_start"], finding["line_end"])

    # Reconstruct a VulnFinding-like object for assess_severity
    from cyberai.models.base import VulnFinding, Severity
    vf = VulnFinding(
        file_path=finding.get("file", ""),
        title=finding["title"],
        vuln_type=finding["type"],
        line_start=finding["line_start"],
        line_end=finding["line_end"],
        severity=Severity(finding["severity"]),
        description=finding["description"],
        proof_of_concept=finding.get("poc", ""),
        cvss_score=finding.get("cvss", 0.0),
        confidence=finding["confidence"],
    )

    try:
        updated = await asyncio.wait_for(
            agent.assess_severity(vf, context=context),
            timeout=180,
        )
        return {
            "verified": True,
            "is_valid": updated.confidence > 0.0,
            "severity": updated.severity.value,
            "confidence": updated.confidence,
        }
    except Exception as e:
        return {"verified": False, "error": str(e)}


async def scan_file_with_context(agent, filepath: Path, project_root: Path, verify: bool = False, verify_delay: float = 15.0) -> dict:
    """Scan a single file and return structured research result."""
    rel_path = str(filepath.relative_to(project_root))
    raw_content = filepath.read_text(errors="ignore")
    content, truncated = smart_truncate(raw_content)

    console.print(f"  [dim]Scanning {rel_path} ({len(content):,} chars{'  truncated' if truncated else ''})...[/dim]")
    start = time.time()

    try:
        result = await asyncio.wait_for(
            agent.scan_file(content, rel_path),
            timeout=120,
        )
        elapsed = time.time() - start

        findings_list = [
            {
                "id": f.id,
                "severity": f.severity.value,
                "title": f.title,
                "type": f.vuln_type,
                "description": f.description,
                "line_start": f.line_start,
                "line_end": f.line_end,
                "confidence": f.confidence,
                "cvss": f.cvss_score,
                "poc": f.proof_of_concept,
                "file": rel_path,
                "verified": False,
            }
            for f in result.findings
        ]

        # --- Optional Stage 4: second-pass verification with source context ---
        if verify and findings_list:
            console.print(f"  [dim]Verifying {len(findings_list)} findings...[/dim]")
            for i, finding in enumerate(findings_list):
                if i > 0:
                    await asyncio.sleep(verify_delay)
                vr = await verify_finding(agent, finding, filepath)
                if vr.get("verified"):
                    finding["verified"] = True
                    finding["is_valid"] = vr["is_valid"]
                    finding["severity_verified"] = vr["severity"]
                    finding["confidence_verified"] = vr["confidence"]
                    if not vr["is_valid"]:
                        console.print(
                            f"    [dim strikethrough]{finding['title']}[/] "
                            f"[yellow]→ false positive[/yellow]"
                        )
                    else:
                        old_sev = finding["severity"]
                        new_sev = vr["severity"]
                        if old_sev != new_sev:
                            sev_color = {"critical": "red", "high": "yellow", "medium": "cyan"}.get(new_sev, "white")
                            console.print(
                                f"    [{sev_color}]{finding['title']}[/] "
                                f"[dim]{old_sev} → {new_sev}[/dim]"
                            )

        # Recount severity after verification (exclude confirmed false positives)
        severity_counts = {}
        valid_count = 0
        for f in findings_list:
            if f.get("is_valid") is False:
                continue  # confirmed false positive
            sev = f.get("severity_verified", f["severity"])
            severity_counts[sev] = severity_counts.get(sev, 0) + 1
            valid_count += 1

        if not verify:
            # pre-verification: use raw counts
            severity_counts = {}
            for f in findings_list:
                sev = f["severity"]
                severity_counts[sev] = severity_counts.get(sev, 0) + 1
            valid_count = len(findings_list)

        return {
            "file": rel_path,
            "size_chars": len(content),
            "truncated": truncated,
            "scan_time_s": round(elapsed, 1),
            "vuln_count": valid_count,
            "error": result.error,
            "cost_usd": result.usage.cost_usd,
            "tokens": result.usage.total_tokens,
            "severity_counts": severity_counts,
            "findings": findings_list,
        }

    except asyncio.TimeoutError:
        return {
            "file": rel_path, "size_chars": len(content), "truncated": truncated,
            "scan_time_s": 120, "vuln_count": 0, "error": "TIMEOUT",
            "cost_usd": 0, "tokens": 0, "severity_counts": {}, "findings": [],
        }
    except Exception as e:
        return {
            "file": rel_path, "size_chars": len(content), "truncated": truncated,
            "scan_time_s": time.time() - start, "vuln_count": 0, "error": str(e),
            "cost_usd": 0, "tokens": 0, "severity_counts": {}, "findings": [],
        }


def get_target_files(project_root: Path, batch: str | None, max_files: int) -> list[Path]:
    """Get list of files to scan, using T1 overrides or automatic ranking."""
    project_name = project_root.name.lower()

    # Use T1 override list if batch=T1 and project is known
    if batch and batch.upper() == "T1":
        for key, override_list in T1_OVERRIDES.items():
            if key in project_name:
                files = []
                for rel in override_list[:max_files]:
                    p = project_root / rel
                    if p.exists():
                        files.append(p)
                    else:
                        console.print(f"  [yellow]Warning: {rel} not found in {project_root}[/yellow]")
                if files:
                    console.print(f"  [cyan]Using T1 override list: {len(files)} files[/cyan]")
                    return files

    # Fall back to automatic ranking
    from cyberai.scanner.file_ranker import get_scan_queue
    queue = get_scan_queue(project_root, max_files=max_files)
    console.print(f"  [cyan]Auto-ranked: {len(queue)} files selected[/cyan]")
    return [Path(f.path) for f in queue]


async def main():
    parser = argparse.ArgumentParser(description="CyberAI Research Scanner")
    parser.add_argument("--target", required=True, help="Path to project root")
    parser.add_argument("--batch", default=None, help="Batch name: T1, T2, T3 (T1 uses priority override)")
    parser.add_argument("--max", type=int, default=5, help="Max files to scan")
    parser.add_argument("--delay", type=int, default=15, help="Seconds to wait between scans (rate limit)")
    parser.add_argument("--verify", action="store_true", help="Run second-pass verification with source code context")
    parser.add_argument("--verify-delay", type=int, default=15, help="Seconds between verification API calls")
    parser.add_argument("--output-dir", default="/root/cyberai_research", help="Output directory for results")
    args = parser.parse_args()

    project_root = Path(args.target).resolve()
    if not project_root.exists():
        console.print(f"[red]Target not found: {project_root}[/red]")
        sys.exit(1)

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    project_name = project_root.name
    batch_label = args.batch or "auto"
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    output_file = output_dir / f"{project_name}_{batch_label}_{timestamp}.json"

    console.print(Panel.fit(
        f"[bold white]CyberAI Research Scanner[/bold white]\n"
        f"Target:  {project_root}\n"
        f"Batch:   {batch_label}\n"
        f"Max:     {args.max} files\n"
        f"Delay:   {args.delay}s between scans\n"
        f"Output:  {output_file}",
        title="🔬 Research Mode",
        border_style="cyan",
    ))

    from cyberai.config import settings
    from cyberai.models.glm import GLMAdapter

    agent = GLMAdapter(api_key=settings.glm_api_key)
    target_files = get_target_files(project_root, args.batch, args.max)

    if not target_files:
        console.print("[red]No files to scan.[/red]")
        sys.exit(1)

    console.print(f"\n[bold]Files queued for scanning:[/bold]")
    for i, f in enumerate(target_files, 1):
        rel = f.relative_to(project_root)
        size = f.stat().st_size
        console.print(f"  {i}. {rel}  [dim]({size:,} bytes)[/dim]")

    console.print()

    # Scan sequentially with delays to avoid rate limits
    scan_results = []
    total_cost = 0.0
    total_findings = 0
    start_time = time.time()

    for i, filepath in enumerate(target_files):
        if i > 0:
            console.print(f"  [dim]Waiting {args.delay}s...[/dim]")
            await asyncio.sleep(args.delay)

        console.print(f"\n[bold cyan][{i+1}/{len(target_files)}][/bold cyan]", end=" ")
        result = await scan_file_with_context(agent, filepath, project_root, verify=args.verify, verify_delay=args.verify_delay)
        scan_results.append(result)

        total_cost += result["cost_usd"]
        total_findings += result["vuln_count"]

        # Print findings inline
        if result["error"]:
            console.print(f"    [red]Error: {result['error']}[/red]")
        else:
            sev_str = "  ".join(
                f"[{'red' if s=='critical' else 'yellow' if s=='high' else 'cyan'}]{n}×{s}[/]"
                for s, n in sorted(result["severity_counts"].items(),
                                   key=lambda x: ["critical","high","medium","low","info"].index(x[0])
                                   if x[0] in ["critical","high","medium","low","info"] else 99)
            ) or "[dim]no findings[/dim]"
            console.print(f"    {result['vuln_count']} findings: {sev_str}  "
                          f"[dim]${result['cost_usd']:.4f}  {result['scan_time_s']:.0f}s[/dim]")

            for f in result["findings"]:
                sev_color = {"critical": "red", "high": "yellow", "medium": "cyan"}.get(f["severity"], "white")
                console.print(f"      [{sev_color}][{f['severity'].upper()}][/{sev_color}] "
                               f"{f['title']}  [dim]L{f['line_start']}-{f['line_end']} "
                               f"conf={f['confidence']:.0%}[/dim]")

    elapsed = time.time() - start_time

    # Summary table
    console.print()
    table = Table(title=f"Scan Summary — {project_name} ({batch_label})")
    table.add_column("File", max_width=40)
    table.add_column("Findings", justify="right")
    table.add_column("Critical", justify="right", style="red")
    table.add_column("High", justify="right", style="yellow")
    table.add_column("Cost", justify="right")
    table.add_column("Status")

    for r in scan_results:
        status = "[red]ERROR[/red]" if r["error"] else "[green]OK[/green]"
        table.add_row(
            r["file"].split("/")[-1],
            str(r["vuln_count"]),
            str(r["severity_counts"].get("critical", 0)),
            str(r["severity_counts"].get("high", 0)),
            f"${r['cost_usd']:.4f}",
            status,
        )
    console.print(table)

    console.print(Panel.fit(
        f"Files scanned:    {len(scan_results)}\n"
        f"Total findings:   {total_findings}\n"
        f"  Critical:       {sum(r['severity_counts'].get('critical', 0) for r in scan_results)}\n"
        f"  High:           {sum(r['severity_counts'].get('high', 0) for r in scan_results)}\n"
        f"Total cost:       ${total_cost:.4f}\n"
        f"Total time:       {elapsed:.0f}s\n"
        f"Results saved:    {output_file}",
        title="📊 Results",
        border_style="green",
    ))

    # Save JSON report
    report = {
        "project": project_name,
        "target": str(project_root),
        "batch": batch_label,
        "model": "glm-5.1",
        "timestamp": timestamp,
        "duration_s": round(elapsed, 1),
        "total_findings": total_findings,
        "total_cost_usd": round(total_cost, 6),
        "files_scanned": len(scan_results),
        "known_cves_reference": {
            f: cves for f, cves in KNOWN_CVES.items()
            if any(f in r["file"] for r in scan_results)
        },
        "results": scan_results,
    }

    with open(output_file, "w", encoding="utf-8") as fp:
        json.dump(report, fp, indent=2, ensure_ascii=False)

    console.print(f"\n[bold green]Report saved → {output_file}[/bold green]")


if __name__ == "__main__":
    asyncio.run(main())
