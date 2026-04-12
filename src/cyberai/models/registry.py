"""Model registry — factory for creating SecurityAgent instances."""

from __future__ import annotations

from typing import Type

from cyberai.models.base import SecurityAgent


class ModelRegistry:
    """Central registry of available model adapters."""

    _adapters: dict[str, Type[SecurityAgent]] = {}

    @classmethod
    def register(cls, name: str, adapter_cls: Type[SecurityAgent]) -> None:
        cls._adapters[name] = adapter_cls

    @classmethod
    def create(cls, model_name: str, api_key: str = "", **kwargs) -> SecurityAgent:
        """Create a SecurityAgent for the given model name.

        Model name format: "provider" or "provider-version"
        Examples: "glm-5.1", "claude-opus-4.6", "gpt-4o"
        """
        # Auto-register built-in adapters on first use
        cls._ensure_builtins()

        # Find adapter by prefix match
        for prefix, adapter_cls in cls._adapters.items():
            if model_name.startswith(prefix):
                return adapter_cls(model_name=model_name, api_key=api_key, **kwargs)

        available = ", ".join(sorted(cls._adapters.keys()))
        raise ValueError(
            f"Unknown model '{model_name}'. Available prefixes: {available}"
        )

    @classmethod
    def list_models(cls) -> list[str]:
        cls._ensure_builtins()
        return sorted(cls._adapters.keys())

    @classmethod
    def _ensure_builtins(cls) -> None:
        if cls._adapters:
            return

        from cyberai.models.glm import GLMAdapter

        cls.register("glm", GLMAdapter)
        # Future:
        # from cyberai.models.claude import ClaudeAdapter
        # cls.register("claude", ClaudeAdapter)
        # from cyberai.models.openai import OpenAIAdapter
        # cls.register("gpt", OpenAIAdapter)
