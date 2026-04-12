"""CLI entry point for CyberAI."""

import asyncio
import sys
from pathlib import Path

import click
from rich.console import Console
from rich.table import Table

console = Console()


@click.group()
def main():
    """CyberAI - AI-powered software security research platform."""
    pass


@main.command()
@click.argument("project_dir", type=click.Path(exists=True))
@click.option("--model", default="glm-5.1", help="LLM model to use")
@click.option("--max-files", default=None, type=int, help="Max files to scan")
@click.option("--max-parallel", default=4, help="Parallel scan workers")
@click.option("--no-verify", is_flag=True, help="Skip second-pass verification")
def scan(project_dir: str, model: str, max_files: int | None, max_parallel: int, no_verify: bool):
    """Run Carlini pipeline scan on a project directory."""
    from cyberai.config import settings
    from cyberai.models.registry import ModelRegistry
    from cyberai.scanner.carlini import CarliniPipeline, PipelineConfig

    console.print(f"[bold blue]CyberAI Scanner[/bold blue]")
    console.print(f"Project: {project_dir}")
    console.print(f"Model: {model}")

    agent = ModelRegistry.create(model, api_key=settings.glm_api_key)
    config = PipelineConfig(
        max_parallel=max_parallel,
        max_files=max_files,
        verify=not no_verify,
    )
    pipeline = CarliniPipeline(agent=agent, config=config)
    result = asyncio.run(pipeline.run(project_dir))

    # Display results
    console.print(f"\n[bold green]Scan Complete[/bold green]")
    console.print(f"Files scanned: {result.total_files_scanned}/{result.total_files_discovered}")
    console.print(f"Findings: {result.total_findings} (verified: {result.total_findings_verified})")
    console.print(f"Cost: ${result.total_cost_usd:.4f}")
    console.print(f"Duration: {result.duration_seconds:.1f}s")

    if result.total_findings > 0:
        table = Table(title="Vulnerability Findings")
        table.add_column("Severity", style="bold")
        table.add_column("File")
        table.add_column("Title")
        table.add_column("Confidence")

        for sr in result.scan_results:
            for f in sr.findings:
                sev_color = {"critical": "red", "high": "yellow", "medium": "cyan"}.get(
                    f.severity.value, "white"
                )
                table.add_row(
                    f"[{sev_color}]{f.severity.value}[/{sev_color}]",
                    f.file_path,
                    f.title,
                    f"{f.confidence:.0%}",
                )

        console.print(table)


@main.command()
@click.argument("file_path", type=click.Path(exists=True))
@click.option("--model", default="glm-5.1", help="LLM model to use")
def scan_file(file_path: str, model: str):
    """Scan a single file for vulnerabilities."""
    from cyberai.config import settings
    from cyberai.models.registry import ModelRegistry

    content = Path(file_path).read_text(errors="ignore")
    agent = ModelRegistry.create(model, api_key=settings.glm_api_key)

    console.print(f"[bold blue]Scanning[/bold blue] {file_path} with {model}...")
    result = asyncio.run(agent.scan_file(content, file_path))

    console.print(f"Findings: {result.vuln_count}")
    console.print(f"Cost: ${result.usage.cost_usd:.4f} | Tokens: {result.usage.total_tokens}")

    for f in result.findings:
        console.print(f"\n[bold {'red' if f.severity.value == 'critical' else 'yellow'}]"
                       f"[{f.severity.value.upper()}] {f.title}[/bold {'red' if f.severity.value == 'critical' else 'yellow'}]")
        console.print(f"  Type: {f.vuln_type} | Lines: {f.line_start}-{f.line_end}")
        console.print(f"  {f.description}")


@main.command()
def test_model():
    """Test the LLM connection."""
    from cyberai.config import settings
    from cyberai.models.registry import ModelRegistry

    console.print("[bold blue]Testing model connection...[/bold blue]")

    agent = ModelRegistry.create("glm-5.1", api_key=settings.glm_api_key)
    resp, usage = asyncio.run(agent.chat([{"role": "user", "content": "Say 'CyberAI ready' in one line."}]))

    console.print(f"[green]Response:[/green] {resp}")
    console.print(f"Tokens: {usage.total_tokens} | Cost: ${usage.cost_usd:.6f}")


if __name__ == "__main__":
    main()
