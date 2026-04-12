"""GLM 5.1 adapter via BigModel (open.bigmodel.cn) API.

Uses the zhipuai SDK for GLM calls. Implements SecurityAgent interface.
"""

from __future__ import annotations

import json
import time
import logging
from dataclasses import dataclass

from cyberai.models.base import (
    SecurityAgent,
    ScanResult,
    VulnFinding,
    ModelUsage,
    Severity,
)

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Pricing (GLM-5.1 via BigModel, per 1K tokens, in USD approx)
# ---------------------------------------------------------------------------
GLM_PRICING = {
    "glm-5.1": {"input": 0.0005, "output": 0.0005},
    "glm-4-plus": {"input": 0.0070, "output": 0.0070},
    "glm-4-flash": {"input": 0.0001, "output": 0.0001},
}

# ---------------------------------------------------------------------------
# Prompt templates
# ---------------------------------------------------------------------------

CTF_SCAN_PROMPT = """You are an elite security researcher competing in a CTF (Capture The Flag) competition.

Your task: find security vulnerabilities in the following source code file.

**File**: `{file_path}`

```
{file_content}
```

Instructions:
1. Analyze the code thoroughly for security vulnerabilities.
2. Focus on: buffer overflows, integer overflows, use-after-free, type confusion, race conditions, injection flaws, authentication/authorization bypasses, cryptographic weaknesses.
3. For each vulnerability found, provide:
   - A short title
   - The vulnerability type
   - Affected line numbers
   - Severity (critical / high / medium / low / info)
   - A brief description of the bug and its impact
   - A proof-of-concept exploit or triggering input (if possible)
   - CVSS v3.1 base score estimate
   - Your confidence level (0.0 to 1.0)

Respond in JSON format:
```json
{{
  "findings": [
    {{
      "title": "...",
      "vuln_type": "buffer_overflow",
      "line_start": 42,
      "line_end": 45,
      "severity": "critical",
      "description": "...",
      "proof_of_concept": "...",
      "cvss_score": 9.8,
      "confidence": 0.9
    }}
  ]
}}
```

If no vulnerabilities are found, return `{{"findings": []}}`.
"""

SEVERITY_ASSESS_PROMPT = """You are a senior security analyst performing a second-pass review of a vulnerability finding.

**Original Finding:**
- Title: {title}
- Type: {vuln_type}
- Severity: {severity}
- Description: {description}
- PoC: {poc}

{context}

Re-assess this finding:
1. Is this a real vulnerability or a false positive?
2. Is the severity rating accurate?
3. What is your confidence level (0.0 to 1.0)?

Respond in JSON:
```json
{{
  "is_valid": true,
  "severity": "critical",
  "confidence": 0.85,
  "reasoning": "..."
}}
```
"""


class GLMAdapter(SecurityAgent):
    """GLM 5.1 adapter using zhipuai SDK."""

    def __init__(self, model_name: str = "glm-5.1", api_key: str = "", **kwargs):
        super().__init__(model_name=model_name, api_key=api_key, **kwargs)
        self._client = None

    @property
    def client(self):
        if self._client is None:
            from zhipuai import ZhipuAI

            self._client = ZhipuAI(api_key=self.api_key)
        return self._client

    def _calc_cost(self, input_tokens: int, output_tokens: int) -> float:
        prices = GLM_PRICING.get(self.model_name, GLM_PRICING["glm-5.1"])
        return (input_tokens * prices["input"] + output_tokens * prices["output"]) / 1000

    async def chat(self, messages: list[dict[str, str]]) -> tuple[str, ModelUsage]:
        """Send chat completion to GLM."""
        t0 = time.monotonic()
        response = self.client.chat.completions.create(
            model=self.model_name,
            messages=messages,
            temperature=0.1,
        )
        latency = int((time.monotonic() - t0) * 1000)

        content = response.choices[0].message.content or ""
        usage_data = response.usage

        usage = ModelUsage(
            model=self.model_name,
            input_tokens=usage_data.prompt_tokens,
            output_tokens=usage_data.completion_tokens,
            cost_usd=self._calc_cost(usage_data.prompt_tokens, usage_data.completion_tokens),
            latency_ms=latency,
        )
        return content, usage

    async def scan_file(
        self,
        file_content: str,
        file_path: str,
        *,
        prompt_strategy: str = "ctf",
        token_budget: int = 100_000,
    ) -> ScanResult:
        """Scan a single file using CTF prompt strategy (Carlini method)."""
        result = ScanResult(target=file_path, model=self.model_name)

        prompt = CTF_SCAN_PROMPT.format(file_path=file_path, file_content=file_content)
        messages = [{"role": "user", "content": prompt}]

        try:
            response_text, usage = await self.chat(messages)
            result.usage = usage

            # Parse JSON from response
            findings_data = self._extract_json(response_text)
            if findings_data and "findings" in findings_data:
                for f in findings_data["findings"]:
                    finding = VulnFinding(
                        file_path=file_path,
                        title=f.get("title", ""),
                        vuln_type=f.get("vuln_type", "unknown"),
                        line_start=f.get("line_start", 0),
                        line_end=f.get("line_end", 0),
                        severity=self._parse_severity(f.get("severity", "info")),
                        description=f.get("description", ""),
                        proof_of_concept=f.get("proof_of_concept", ""),
                        cvss_score=f.get("cvss_score", 0.0),
                        confidence=f.get("confidence", 0.0),
                        raw_response=response_text,
                    )
                    result.findings.append(finding)

        except Exception as e:
            logger.error("Scan failed for %s: %s", file_path, e)
            result.error = str(e)

        result.finished_at = time.time()
        return result

    async def assess_severity(
        self,
        finding: VulnFinding,
        context: str = "",
    ) -> VulnFinding:
        """Second-pass severity verification."""
        prompt = SEVERITY_ASSESS_PROMPT.format(
            title=finding.title,
            vuln_type=finding.vuln_type,
            severity=finding.severity.value,
            description=finding.description,
            poc=finding.proof_of_concept,
            context=f"Additional context:\n{context}" if context else "",
        )

        try:
            response_text, usage = await self.chat([{"role": "user", "content": prompt}])
            data = self._extract_json(response_text)

            if data:
                finding.severity = self._parse_severity(data.get("severity", finding.severity.value))
                finding.confidence = data.get("confidence", finding.confidence)
                if not data.get("is_valid", True):
                    finding.confidence = 0.0  # mark as false positive

        except Exception as e:
            logger.warning("Severity assessment failed: %s", e)

        return finding

    @staticmethod
    def _extract_json(text: str) -> dict | None:
        """Extract JSON from LLM response (handles markdown code blocks)."""
        # Try to find JSON in code blocks
        import re

        json_match = re.search(r"```(?:json)?\s*\n?(.*?)\n?```", text, re.DOTALL)
        if json_match:
            text = json_match.group(1)

        # Try direct parse
        try:
            return json.loads(text.strip())
        except json.JSONDecodeError:
            # Try to find any JSON object
            brace_match = re.search(r"\{.*\}", text, re.DOTALL)
            if brace_match:
                try:
                    return json.loads(brace_match.group())
                except json.JSONDecodeError:
                    pass
        return None

    @staticmethod
    def _parse_severity(value: str) -> Severity:
        try:
            return Severity(value.lower())
        except ValueError:
            return Severity.INFO


# ---------------------------------------------------------------------------
# Quick test
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    import asyncio
    from cyberai.config import settings

    async def _test():
        agent = GLMAdapter(api_key=settings.glm_api_key)
        print(f"Testing {agent}")

        # Simple test
        resp, usage = await agent.chat([{"role": "user", "content": "Say 'CyberAI ready' in one line."}])
        print(f"Response: {resp}")
        print(f"Usage: {usage}")

        # Scan test with a trivially vulnerable C snippet
        vuln_code = '''
#include <string.h>
#include <stdio.h>

void process_input(char *input) {
    char buffer[64];
    strcpy(buffer, input);  // no bounds check
    printf("Processed: %s\\n", buffer);
}

int main(int argc, char **argv) {
    if (argc > 1) process_input(argv[1]);
    return 0;
}
'''
        result = await agent.scan_file(vuln_code, "test_vuln.c")
        print(f"\nScan result: {result.vuln_count} findings")
        for f in result.findings:
            print(f"  [{f.severity.value}] {f.title} (confidence: {f.confidence})")
        print(f"Cost: ${result.usage.cost_usd:.4f}, Tokens: {result.usage.total_tokens}")

    asyncio.run(_test())
