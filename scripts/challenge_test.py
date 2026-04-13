#!/usr/bin/env python3
"""CyberAI Challenge Test - CVE Detection + Real-world Code Scan"""
import asyncio
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from rich.console import Console
from rich.table import Table
from rich.panel import Panel

console = Console()


async def scan_cve_collection():
    """Scan known CVE pattern files."""
    from cyberai.config import settings
    from cyberai.models.glm import GLMAdapter
    from cyberai.scanner.carlini import CarliniPipeline, PipelineConfig

    console.print(Panel("[bold]Challenge 1: CVE Pattern Detection[/bold]\n"
                        "4 files with known CVE patterns:\n"
                        "- Heartbleed (CVE-2014-0160)\n"
                        "- Sudo heap overflow (CVE-2021-3156)\n"
                        "- OpenSSH PKCS#11 (CVE-2023-38408)\n"
                        "- TOCTOU race condition",
                        border_style="red"))

    agent = GLMAdapter(api_key=settings.glm_api_key)
    config = PipelineConfig(max_parallel=2, max_files=10, verify=True)
    pipeline = CarliniPipeline(agent=agent, config=config)

    result = await pipeline.run("/tmp/cyberai_challenge/cve_collection")

    console.print(f"\n[bold green]CVE Collection Results:[/bold green]")
    console.print(f"  Files scanned: {result.total_files_scanned}/{result.total_files_discovered}")
    console.print(f"  Raw findings: {result.total_findings}")
    console.print(f"  Verified: {result.total_findings_verified}")
    console.print(f"  Cost: ${result.total_cost_usd:.4f}")
    console.print(f"  Duration: {result.duration_seconds:.1f}s")

    if result.scan_results:
        table = Table(title="CVE Detection Results")
        table.add_column("Severity", style="bold")
        table.add_column("File")
        table.add_column("Vulnerability")
        table.add_column("Conf")
        for sr in result.scan_results:
            for f in sr.findings:
                sev_color = {"critical": "red", "high": "yellow", "medium": "cyan"}.get(f.severity.value, "white")
                table.add_row(
                    f"[{sev_color}]{f.severity.value}[/{sev_color}]",
                    f.file_path.split("/")[-1] if "/" in f.file_path else f.file_path,
                    f.title[:60],
                    f"{f.confidence:.0%}",
                )
        console.print(table)

    # Check detection rate
    files_with_findings = set()
    for sr in result.scan_results:
        if sr.findings:
            fname = sr.file_path.split("/")[-1] if "/" in sr.file_path else sr.file_path
            files_with_findings.add(fname)

    expected = {"heartbleed_pattern.c", "sudoedit_vuln.c", "ssh_pkcs11_pattern.c", "toctou_race.c"}
    detected = expected & files_with_findings
    console.print(f"\n  [bold]Detection Rate: {len(detected)}/{len(expected)} CVE patterns detected[/bold]")
    for f in expected:
        status = "[green]DETECTED[/green]" if f in detected else "[red]MISSED[/red]"
        console.print(f"    {f}: {status}")

    return result


async def scan_jq_sample():
    """Scan jq-style sample code."""
    from cyberai.config import settings
    from cyberai.models.glm import GLMAdapter
    from cyberai.scanner.carlini import CarliniPipeline, PipelineConfig

    console.print(Panel("[bold]Challenge 2: Real-world C Code (jq-style parser)[/bold]\n"
                        "3 files: JSON parser, bytecode executor, utilities\n"
                        "Mix of vulnerable and safe code",
                        border_style="yellow"))

    agent = GLMAdapter(api_key=settings.glm_api_key)
    config = PipelineConfig(max_parallel=2, max_files=5, verify=False)
    pipeline = CarliniPipeline(agent=agent, config=config)

    result = await pipeline.run("/tmp/cyberai_challenge/jq_sample")

    console.print(f"\n[bold green]jq Sample Results:[/bold green]")
    console.print(f"  Files discovered: {result.total_files_discovered}")
    console.print(f"  Files scanned: {result.total_files_scanned}")
    console.print(f"  Findings: {result.total_findings}")
    console.print(f"  Cost: ${result.total_cost_usd:.4f}")
    console.print(f"  Duration: {result.duration_seconds:.1f}s")

    if result.scan_results:
        table = Table(title="jq-style Code Findings")
        table.add_column("Severity", style="bold")
        table.add_column("File")
        table.add_column("Vulnerability")
        table.add_column("Lines")
        table.add_column("Conf")
        for sr in result.scan_results:
            for f in sr.findings:
                sev_color = {"critical": "red", "high": "yellow", "medium": "cyan"}.get(f.severity.value, "white")
                table.add_row(
                    f"[{sev_color}]{f.severity.value}[/{sev_color}]",
                    f.file_path.split("/")[-1] if "/" in f.file_path else f.file_path,
                    f.title[:55],
                    f"{f.line_start}-{f.line_end}",
                    f"{f.confidence:.0%}",
                )
        console.print(table)

    # Analyze: util.c should have fewer/no critical findings
    for sr in result.scan_results:
        fname = sr.file_path.split("/")[-1] if "/" in sr.file_path else sr.file_path
        high_count = sum(1 for f in sr.findings if f.severity.value in ("critical", "high"))
        console.print(f"  {fname}: {len(sr.findings)} findings ({high_count} high/critical)")

    return result


async def main():
    console.print(Panel.fit(
        "[bold white]CyberAI Challenge Test Suite[/bold white]\n"
        "Challenge 1: 4 Known CVE Pattern Detection\n"
        "Challenge 2: Real-world C Code Analysis",
        title="🔒 CyberAI Challenge",
        border_style="cyan",
    ))

    start = time.time()
    results = {}

    for name, fn in [("CVE Collection", scan_cve_collection), ("jq Sample", scan_jq_sample)]:
        try:
            results[name] = await fn()
        except Exception as e:
            console.print(f"[red]{name} FAILED: {e}[/red]")
            import traceback
            traceback.print_exc()
            results[name] = None

    total_time = time.time() - start

    # Grand Summary
    console.print("\n")
    console.print(Panel("[bold white]═══ Grand Summary ═══[/bold white]", border_style="green"))

    total_cost = 0
    total_findings = 0
    total_files = 0

    summary_table = Table(title="Challenge Results")
    summary_table.add_column("Challenge", style="bold")
    summary_table.add_column("Files Scanned")
    summary_table.add_column("Findings")
    summary_table.add_column("Cost")
    summary_table.add_column("Time")

    for name, result in results.items():
        if result:
            total_cost += result.total_cost_usd
            total_findings += result.total_findings
            total_files += result.total_files_scanned
            summary_table.add_row(
                name,
                str(result.total_files_scanned),
                str(result.total_findings),
                f"${result.total_cost_usd:.4f}",
                f"{result.duration_seconds:.0f}s",
            )
        else:
            summary_table.add_row(name, "-", "[red]FAILED[/red]", "-", "-")

    console.print(summary_table)
    console.print(f"\n[bold green]Total files scanned: {total_files}[/bold green]")
    console.print(f"[bold green]Total findings: {total_findings}[/bold green]")
    console.print(f"[bold green]Total cost: ${total_cost:.4f}[/bold green]")
    console.print(f"[bold green]Total time: {total_time:.0f}s[/bold green]")
    console.print(f"\n[bold cyan]Pipeline Status: {'✅ ALL PASSED' if all(results.values()) else '⚠️ SOME FAILED'}[/bold cyan]")


if __name__ == "__main__":
    asyncio.run(main())
