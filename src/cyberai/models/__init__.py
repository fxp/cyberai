"""Model abstraction layer — unified interface for multiple LLMs."""

from cyberai.models.base import SecurityAgent, ScanResult, ExploitResult, ModelUsage
from cyberai.models.registry import ModelRegistry

__all__ = ["SecurityAgent", "ScanResult", "ExploitResult", "ModelUsage", "ModelRegistry"]
