"""Carlini-style per-file vulnerability scanner (Pipeline A).

Implements the 4-stage pipeline:
  Stage 1: File pre-filtering + ranking (file_ranker.py)
  Stage 2: Parallel LLM scanning (this module)
  Stage 3: Result aggregation + dedup (dedup.py)
  Stage 4: Second-pass verification (verifier.py)
"""

from __future__ import annotations

import asyncio
import logging
import time
from dataclasses import dataclass, field
from pathlib import Path

from cyberai.models.base import SecurityAgent, ScanResult
from cyberai.scanner.file_ranker import RankedFile, get_scan_queue

logger = logging.getLogger(__name__)


@dataclass
class PipelineConfig:
    """Configuration for the Carlini scanning pipeline."""
    max_parallel: int = 4         # concurrent scans
    token_budget: int = 100_000   # per-file token budget
    time_budget: int = 600        # per-file time limit (seconds)
    max_files: int | None = None  # limit number of files to scan
    min_score: float = 0.0        # minimum file ranking score
    prompt_strategy: str = "ctf"  # "ctf" (Carlini) or "audit"
    verify: bool = True           # run second-pass verification
    verify_model: str | None = None  # model for verification (None = same)


@dataclass
class PipelineResult:
    """Aggregated result of a full pipeline run."""
    project: str = ""
    config: PipelineConfig = field(default_factory=PipelineConfig)
    scan_results: list[ScanResult] = field(default_factory=list)
    total_files_discovered: int = 0
    total_files_scanned: int = 0
    total_findings: int = 0
    total_findings_verified: int = 0
    total_cost_usd: float = 0.0
    total_tokens: int = 0
    started_at: float = field(default_factory=time.time)
    finished_at: float = 0.0

    @property
    def duration_seconds(self) -> float:
        if self.finished_at > 0:
            return self.finished_at - self.started_at
        return time.time() - self.started_at


class CarliniPipeline:
    """Orchestrates the Carlini per-file scanning pipeline."""

    def __init__(self, agent: SecurityAgent, config: PipelineConfig | None = None):
        self.agent = agent
        self.config = config or PipelineConfig()

    async def run(self, project_dir: str | Path) -> PipelineResult:
        """Execute the full 4-stage pipeline on a project directory.

        Args:
            project_dir: Root directory of the project to scan.

        Returns:
            PipelineResult with all findings.
        """
        project_dir = Path(project_dir).resolve()
        result = PipelineResult(project=str(project_dir), config=self.config)

        # --- Stage 1: File discovery + ranking ---
        logger.info("Stage 1: Discovering and ranking files in %s", project_dir)
        scan_queue = get_scan_queue(
            project_dir,
            max_files=self.config.max_files,
            min_score=self.config.min_score,
        )
        result.total_files_discovered = len(scan_queue)
        logger.info(
            "Found %d files to scan (top score: %.1f)",
            len(scan_queue),
            scan_queue[0].score if scan_queue else 0,
        )

        if not scan_queue:
            result.finished_at = time.time()
            return result

        # --- Stage 2: Parallel scanning ---
        logger.info("Stage 2: Scanning %d files (parallel=%d)", len(scan_queue), self.config.max_parallel)
        semaphore = asyncio.Semaphore(self.config.max_parallel)

        async def _scan_one(ranked_file: RankedFile) -> ScanResult:
            async with semaphore:
                return await self._scan_file(ranked_file)

        tasks = [_scan_one(rf) for rf in scan_queue]
        scan_results = await asyncio.gather(*tasks, return_exceptions=True)

        # Collect successful results
        for sr in scan_results:
            if isinstance(sr, ScanResult):
                result.scan_results.append(sr)
                result.total_files_scanned += 1
                result.total_findings += sr.vuln_count
                result.total_cost_usd += sr.usage.cost_usd
                result.total_tokens += sr.usage.total_tokens
            elif isinstance(sr, Exception):
                logger.error("Scan task failed: %s", sr)

        logger.info(
            "Stage 2 complete: scanned %d files, found %d raw findings",
            result.total_files_scanned,
            result.total_findings,
        )

        # --- Stage 3: Dedup (inline simple version) ---
        # Full dedup logic is in dedup.py; here we just log
        logger.info("Stage 3: Deduplication (TODO: full implementation)")

        # --- Stage 4: Verification ---
        if self.config.verify and result.total_findings > 0:
            logger.info("Stage 4: Second-pass verification of %d findings", result.total_findings)
            verified_count = await self._verify_findings(result)
            result.total_findings_verified = verified_count
            logger.info("Verified: %d / %d findings", verified_count, result.total_findings)

        result.finished_at = time.time()
        logger.info(
            "Pipeline complete in %.1fs. Files: %d, Findings: %d, Cost: $%.4f",
            result.duration_seconds,
            result.total_files_scanned,
            result.total_findings,
            result.total_cost_usd,
        )
        return result

    async def _scan_file(self, ranked_file: RankedFile) -> ScanResult:
        """Scan a single file with timeout."""
        try:
            content = Path(ranked_file.path).read_text(errors="ignore")
        except OSError as e:
            return ScanResult(target=ranked_file.path, error=str(e))

        # Truncate very long files to fit token budget
        max_chars = self.config.token_budget * 3  # rough chars-to-tokens ratio
        if len(content) > max_chars:
            content = content[:max_chars] + "\n\n... [truncated]"

        try:
            return await asyncio.wait_for(
                self.agent.scan_file(
                    file_content=content,
                    file_path=ranked_file.relative_path,
                    prompt_strategy=self.config.prompt_strategy,
                    token_budget=self.config.token_budget,
                ),
                timeout=self.config.time_budget,
            )
        except asyncio.TimeoutError:
            return ScanResult(
                target=ranked_file.path,
                model=self.agent.model_name,
                error=f"Timeout after {self.config.time_budget}s",
            )

    async def _verify_findings(self, result: PipelineResult) -> int:
        """Run second-pass severity assessment on all findings."""
        verified = 0
        for sr in result.scan_results:
            for finding in sr.findings:
                try:
                    updated = await self.agent.assess_severity(finding)
                    if updated.confidence > 0.0:  # not marked as false positive
                        verified += 1
                except Exception as e:
                    logger.warning("Verification failed for %s: %s", finding.id, e)
        return verified
