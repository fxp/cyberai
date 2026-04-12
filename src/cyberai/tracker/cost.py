"""Cost and usage tracking for LLM experiments.

Records every LLM call's token usage, cost, and timing for analysis.
"""

from __future__ import annotations

import json
import time
from dataclasses import asdict, dataclass, field
from pathlib import Path

from cyberai.models.base import ModelUsage


@dataclass
class ExperimentRecord:
    """A single experiment/scan run record."""
    experiment_id: str = ""
    model: str = ""
    task_type: str = ""  # scan, verify, exploit, assess
    target: str = ""
    input_tokens: int = 0
    output_tokens: int = 0
    think_tokens: int = 0
    cost_usd: float = 0.0
    latency_ms: int = 0
    duration_seconds: float = 0.0
    result_summary: str = ""
    timestamp: float = field(default_factory=time.time)


class CostTracker:
    """Tracks cumulative costs and usage across experiments.

    Stores records in a JSONL file for easy analysis.
    """

    def __init__(self, log_path: str | Path = "experiments.jsonl"):
        self.log_path = Path(log_path)
        self._records: list[ExperimentRecord] = []

    def record(self, usage: ModelUsage, *, task_type: str = "", target: str = "",
               experiment_id: str = "", duration: float = 0.0, result: str = "") -> None:
        """Record a single LLM usage event."""
        rec = ExperimentRecord(
            experiment_id=experiment_id,
            model=usage.model,
            task_type=task_type,
            target=target,
            input_tokens=usage.input_tokens,
            output_tokens=usage.output_tokens,
            think_tokens=usage.think_tokens,
            cost_usd=usage.cost_usd,
            latency_ms=usage.latency_ms,
            duration_seconds=duration,
            result_summary=result,
        )
        self._records.append(rec)

        # Append to JSONL
        with open(self.log_path, "a") as f:
            f.write(json.dumps(asdict(rec), ensure_ascii=False) + "\n")

    @property
    def total_cost(self) -> float:
        return sum(r.cost_usd for r in self._records)

    @property
    def total_tokens(self) -> int:
        return sum(r.input_tokens + r.output_tokens + r.think_tokens for r in self._records)

    def summary(self) -> dict:
        """Return a summary of all recorded experiments."""
        if not self._records:
            return {"total_records": 0, "total_cost_usd": 0, "total_tokens": 0}

        by_model: dict[str, dict] = {}
        for r in self._records:
            m = by_model.setdefault(r.model, {"count": 0, "cost": 0.0, "tokens": 0})
            m["count"] += 1
            m["cost"] += r.cost_usd
            m["tokens"] += r.input_tokens + r.output_tokens + r.think_tokens

        return {
            "total_records": len(self._records),
            "total_cost_usd": round(self.total_cost, 6),
            "total_tokens": self.total_tokens,
            "by_model": by_model,
        }
