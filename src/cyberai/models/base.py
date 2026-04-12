"""Abstract base classes for the model abstraction layer.

Every LLM adapter implements SecurityAgent, so the scanner/red-team/blue-team
code never couples to a specific provider.
"""

from __future__ import annotations

import time
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from enum import Enum
from typing import Any
from uuid import uuid4


# ---------------------------------------------------------------------------
# Data models
# ---------------------------------------------------------------------------

class Severity(str, Enum):
    CRITICAL = "critical"
    HIGH = "high"
    MEDIUM = "medium"
    LOW = "low"
    INFO = "info"


class OSSFuzzTier(int, Enum):
    """OSS-Fuzz crash exploitability tiers (from System Card)."""
    TIER_1 = 1  # basic crash
    TIER_2 = 2  # controlled crash
    TIER_3 = 3  # info leak
    TIER_4 = 4  # partial control-flow hijack
    TIER_5 = 5  # full control-flow hijack


@dataclass
class ModelUsage:
    """Token usage + cost tracking for a single LLM call."""
    model: str = ""
    input_tokens: int = 0
    output_tokens: int = 0
    think_tokens: int = 0  # for models with think tool
    cost_usd: float = 0.0
    latency_ms: int = 0

    @property
    def total_tokens(self) -> int:
        return self.input_tokens + self.output_tokens + self.think_tokens


@dataclass
class VulnFinding:
    """A single vulnerability finding from a scan."""
    id: str = field(default_factory=lambda: uuid4().hex[:12])
    file_path: str = ""
    line_start: int = 0
    line_end: int = 0
    vuln_type: str = ""  # buffer_overflow, integer_overflow, uaf, ...
    severity: Severity = Severity.INFO
    oss_fuzz_tier: OSSFuzzTier | None = None
    title: str = ""
    description: str = ""
    proof_of_concept: str = ""
    cvss_score: float = 0.0
    confidence: float = 0.0  # 0.0 - 1.0
    raw_response: str = ""


@dataclass
class ScanResult:
    """Result of a vulnerability scan on a single file or project."""
    task_id: str = field(default_factory=lambda: uuid4().hex[:16])
    target: str = ""  # file path or project name
    model: str = ""
    findings: list[VulnFinding] = field(default_factory=list)
    usage: ModelUsage = field(default_factory=ModelUsage)
    started_at: float = field(default_factory=time.time)
    finished_at: float = 0.0
    error: str | None = None

    @property
    def duration_seconds(self) -> float:
        if self.finished_at > 0:
            return self.finished_at - self.started_at
        return time.time() - self.started_at

    @property
    def vuln_count(self) -> int:
        return len(self.findings)


@dataclass
class ExploitResult:
    """Result of an exploit development attempt."""
    task_id: str = field(default_factory=lambda: uuid4().hex[:16])
    vuln_id: str = ""
    model: str = ""
    success: bool = False
    score: float = 0.0  # 0.0, 0.25, 0.5, 0.75, 1.0
    exploit_code: str = ""
    chain_steps: int = 0
    techniques: list[str] = field(default_factory=list)
    usage: ModelUsage = field(default_factory=ModelUsage)
    error: str | None = None


# ---------------------------------------------------------------------------
# Abstract agent interface
# ---------------------------------------------------------------------------

class SecurityAgent(ABC):
    """Unified interface for LLM-powered security tasks.

    Each model adapter (GLM, Claude, GPT, local) implements this interface.
    """

    def __init__(self, model_name: str, api_key: str = "", **kwargs: Any):
        self.model_name = model_name
        self.api_key = api_key
        self._kwargs = kwargs

    @abstractmethod
    async def scan_file(
        self,
        file_content: str,
        file_path: str,
        *,
        prompt_strategy: str = "ctf",
        token_budget: int = 100_000,
    ) -> ScanResult:
        """Scan a single source file for vulnerabilities.

        Args:
            file_content: The source code to analyze.
            file_path: Path of the file (for context).
            prompt_strategy: Prompt framework — "ctf" (Carlini) or "audit".
            token_budget: Maximum tokens for this call.

        Returns:
            ScanResult with zero or more VulnFinding entries.
        """
        ...

    @abstractmethod
    async def assess_severity(
        self,
        finding: VulnFinding,
        context: str = "",
    ) -> VulnFinding:
        """Re-assess the severity of a finding (second-pass verification).

        Returns a copy of the finding with updated severity/confidence.
        """
        ...

    @abstractmethod
    async def chat(self, messages: list[dict[str, str]]) -> tuple[str, ModelUsage]:
        """Raw chat completion — for ad-hoc security queries.

        Returns (response_text, usage).
        """
        ...

    # convenience --------------------------------------------------------

    @property
    def provider(self) -> str:
        """Return the provider name (e.g. 'glm', 'claude', 'openai')."""
        return self.__class__.__name__.lower().replace("adapter", "")

    def __repr__(self) -> str:
        return f"<{self.__class__.__name__} model={self.model_name}>"
