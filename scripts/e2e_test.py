#!/usr/bin/env python3
"""CyberAI End-to-End Minimal Test

Tests the full pipeline locally:
1. GLM API connection
2. Single file scan (Carlini method)
3. File ranker on a sample project
4. Cost tracking
5. FastAPI server health check
"""

import asyncio
import sys
import tempfile
import os
from pathlib import Path

# Add src to path
sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from rich.console import Console
from rich.panel import Panel
from rich.table import Table

console = Console()

# ---- Test vulnerable code samples ----

VULN_C_CODE = '''
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Vulnerability 1: Stack buffer overflow
void process_request(char *user_input) {
    char buffer[64];
    strcpy(buffer, user_input);  // no bounds check!
    printf("Request: %s\\n", buffer);
}

// Vulnerability 2: Format string
void log_message(char *msg) {
    printf(msg);  // format string vulnerability
}

// Vulnerability 3: Integer overflow
void allocate_items(unsigned short count) {
    unsigned short total = count * sizeof(int);  // overflow if count > 16383
    int *items = (int *)malloc(total);
    if (items) {
        memset(items, 0, count * sizeof(int));  // actual size may exceed allocated
        free(items);
    }
}

int main(int argc, char **argv) {
    if (argc > 1) {
        process_request(argv[1]);
        log_message(argv[1]);
    }
    allocate_items(20000);
    return 0;
}
'''

SAFE_C_CODE = '''
#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int main() {
    printf("Result: %d\\n", add(1, 2));
    return 0;
}
'''


async def test_1_glm_connection():
    """Test 1: GLM API connectivity."""
    console.print("\n[bold cyan]Test 1: GLM API Connection[/bold cyan]")

    from cyberai.config import settings
    from cyberai.models.glm import GLMAdapter

    agent = GLMAdapter(api_key=settings.glm_api_key)
    resp, usage = await agent.chat([{"role": "user", "content": "Reply with exactly: CYBERAI_OK"}])

    ok = "CYBERAI_OK" in resp.upper().replace(" ", "_")
    console.print(f"  Response: {resp.strip()}")
    console.print(f"  Tokens: {usage.total_tokens} | Cost: ${usage.cost_usd:.6f}")
    console.print(f"  [{'green' if ok else 'red'}]{'PASS' if ok else 'FAIL'}[/]")
    return ok


async def test_2_vuln_scan():
    """Test 2: Scan vulnerable code — should find bugs."""
    console.print("\n[bold cyan]Test 2: Vulnerable Code Scan[/bold cyan]")

    from cyberai.config import settings
    from cyberai.models.glm import GLMAdapter

    agent = GLMAdapter(api_key=settings.glm_api_key)
    result = await agent.scan_file(VULN_C_CODE, "test_vuln.c")

    ok = result.vuln_count > 0 and result.error is None
    console.print(f"  Findings: {result.vuln_count}")
    for f in result.findings:
        console.print(f"    [{f.severity.value}] {f.title} (conf: {f.confidence})")
    console.print(f"  Cost: ${result.usage.cost_usd:.4f} | Tokens: {result.usage.total_tokens}")
    console.print(f"  [{'green' if ok else 'red'}]{'PASS' if ok else 'FAIL'}[/]")
    return ok


async def test_3_safe_scan():
    """Test 3: Scan safe code — should find nothing (or only low/info)."""
    console.print("\n[bold cyan]Test 3: Safe Code Scan[/bold cyan]")

    from cyberai.config import settings
    from cyberai.models.glm import GLMAdapter

    agent = GLMAdapter(api_key=settings.glm_api_key)
    result = await agent.scan_file(SAFE_C_CODE, "safe_code.c")

    # Accept 0 findings, or only info/low severity
    high_findings = [f for f in result.findings if f.severity.value in ("critical", "high")]
    ok = len(high_findings) == 0
    console.print(f"  Findings: {result.vuln_count} (high/critical: {len(high_findings)})")
    console.print(f"  Cost: ${result.usage.cost_usd:.4f}")
    console.print(f"  [{'green' if ok else 'yellow'}]{'PASS' if ok else 'WARN - false positives'}[/]")
    return ok


async def test_4_file_ranker():
    """Test 4: File ranker on a temp project."""
    console.print("\n[bold cyan]Test 4: File Ranker[/bold cyan]")

    from cyberai.scanner.file_ranker import get_scan_queue

    # Create a temp project with mixed files
    with tempfile.TemporaryDirectory() as tmpdir:
        # Security-relevant file
        (Path(tmpdir) / "net_handler.c").write_text(
            '#include <sys/socket.h>\nvoid handle_recv(int sock) { char buf[256]; recv(sock, buf, 1024, 0); }\n'
        )
        # Less relevant file
        (Path(tmpdir) / "utils.py").write_text('def add(a, b): return a + b\n')
        # Test file (should be skipped)
        (Path(tmpdir) / "test_utils.py").write_text('def test_add(): assert add(1,2)==3\n')
        # Non-code file (should be skipped)
        (Path(tmpdir) / "README.md").write_text('# Project\n')

        queue = get_scan_queue(tmpdir)

    ok = len(queue) == 2  # net_handler.c + utils.py (test file skipped, .md skipped)
    console.print(f"  Files in queue: {len(queue)}")
    for f in queue:
        console.print(f"    score={f.score:.0f} {f.relative_path} keywords={f.keywords_found[:3]}")

    # net_handler.c should be ranked first (has socket/recv keywords)
    if queue:
        first_is_c = queue[0].relative_path == "net_handler.c"
        console.print(f"  Top ranked: {queue[0].relative_path} ({'correct' if first_is_c else 'unexpected'})")
        ok = ok and first_is_c

    console.print(f"  [{'green' if ok else 'red'}]{'PASS' if ok else 'FAIL'}[/]")
    return ok


async def test_5_cost_tracker():
    """Test 5: Cost tracker records and summarizes."""
    console.print("\n[bold cyan]Test 5: Cost Tracker[/bold cyan]")

    from cyberai.tracker.cost import CostTracker
    from cyberai.models.base import ModelUsage

    with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f:
        tracker = CostTracker(log_path=f.name)

    tracker.record(
        ModelUsage(model="glm-5.1", input_tokens=1000, output_tokens=500, cost_usd=0.001),
        task_type="scan", target="test.c", experiment_id="exp001",
    )
    tracker.record(
        ModelUsage(model="glm-5.1", input_tokens=2000, output_tokens=800, cost_usd=0.002),
        task_type="verify", target="test.c", experiment_id="exp001",
    )

    summary = tracker.summary()
    ok = (
        summary["total_records"] == 2
        and summary["total_cost_usd"] == 0.003
        and summary["total_tokens"] == 4300
    )

    console.print(f"  Records: {summary['total_records']}")
    console.print(f"  Total cost: ${summary['total_cost_usd']}")
    console.print(f"  Total tokens: {summary['total_tokens']}")
    console.print(f"  [{'green' if ok else 'red'}]{'PASS' if ok else 'FAIL'}[/]")

    os.unlink(tracker.log_path)
    return ok


async def test_6_pipeline_mini():
    """Test 6: Mini Carlini pipeline on a temp project."""
    console.print("\n[bold cyan]Test 6: Mini Carlini Pipeline[/bold cyan]")

    from cyberai.config import settings
    from cyberai.models.glm import GLMAdapter
    from cyberai.scanner.carlini import CarliniPipeline, PipelineConfig

    with tempfile.TemporaryDirectory() as tmpdir:
        # One vulnerable file (simple, avoids escaping issues)
        (Path(tmpdir) / "vuln.c").write_text(
            '#include <string.h>\n'
            'void vuln(char *input) {\n'
            '    char buf[32];\n'
            '    strcpy(buf, input);\n'
            '}\n'
            'int main(int argc, char **argv) {\n'
            '    if (argc>1) vuln(argv[1]);\n'
            '}\n'
        )
        # One safe file
        (Path(tmpdir) / "safe.c").write_text(SAFE_C_CODE)

        agent = GLMAdapter(api_key=settings.glm_api_key)
        config = PipelineConfig(
            max_parallel=2,
            max_files=2,
            verify=False,  # skip verification to save API calls
        )
        pipeline = CarliniPipeline(agent=agent, config=config)
        result = await pipeline.run(tmpdir)

    ok = result.total_files_scanned > 0 and result.total_findings > 0
    console.print(f"  Files discovered: {result.total_files_discovered}")
    console.print(f"  Files scanned: {result.total_files_scanned}")
    console.print(f"  Findings: {result.total_findings}")
    console.print(f"  Cost: ${result.total_cost_usd:.4f}")
    console.print(f"  Duration: {result.duration_seconds:.1f}s")
    console.print(f"  [{'green' if ok else 'red'}]{'PASS' if ok else 'FAIL'}[/]")
    return ok


async def main():
    console.print(Panel.fit(
        "[bold white]CyberAI End-to-End Test Suite[/bold white]\n"
        "Testing: GLM connection → Scan → Ranker → Tracker → Pipeline",
        title="🔒 CyberAI",
        border_style="cyan",
    ))

    tests = [
        ("GLM API Connection", test_1_glm_connection),
        ("Vulnerable Code Scan", test_2_vuln_scan),
        ("Safe Code Scan", test_3_safe_scan),
        ("File Ranker", test_4_file_ranker),
        ("Cost Tracker", test_5_cost_tracker),
        ("Mini Carlini Pipeline", test_6_pipeline_mini),
    ]

    results = []
    for name, test_fn in tests:
        try:
            passed = await test_fn()
            results.append((name, passed))
        except Exception as e:
            console.print(f"  [red]ERROR: {e}[/red]")
            results.append((name, False))

    # Summary
    console.print("\n")
    table = Table(title="Test Summary")
    table.add_column("Test", style="bold")
    table.add_column("Result")

    passed_count = 0
    for name, passed in results:
        status = "[green]PASS[/green]" if passed else "[red]FAIL[/red]"
        table.add_row(name, status)
        if passed:
            passed_count += 1

    console.print(table)
    console.print(f"\n[bold]{passed_count}/{len(results)} tests passed[/bold]")

    return passed_count == len(results)


if __name__ == "__main__":
    success = asyncio.run(main())
    sys.exit(0 if success else 1)
