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
   - **Critically**: check if the code has existing mitigations (input sanitization, bounds checks,
     escaping functions, security policy guards) that would prevent exploitation.
   - If a dangerous pattern (e.g. format string, injection sink) has sanitization applied to its
     inputs BEFORE the sink, mark it as invalid (false positive).
2. Is the severity rating accurate given any mitigations present?
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


_GLM_API_URL = "https://open.bigmodel.cn/api/paas/v4/chat/completions"


class GLMAdapter(SecurityAgent):
    """GLM 5.1 adapter using httpx.AsyncClient for native async with proper timeouts."""

    def __init__(self, model_name: str = "glm-5.1", api_key: str = "", **kwargs):
        super().__init__(model_name=model_name, api_key=api_key, **kwargs)

    def _calc_cost(self, input_tokens: int, output_tokens: int) -> float:
        prices = GLM_PRICING.get(self.model_name, GLM_PRICING["glm-5.1"])
        return (input_tokens * prices["input"] + output_tokens * prices["output"]) / 1000

    def _auth_header(self) -> str:
        """Generate JWT auth header required by BigModel API."""
        import base64
        import hmac
        import hashlib
        import struct

        # BigModel API key format: <id>.<secret>
        parts = self.api_key.split(".", 1)
        if len(parts) != 2:
            return f"Bearer {self.api_key}"

        api_id, api_secret = parts[0], parts[1]

        # Build JWT: header.payload.signature
        header = base64.urlsafe_b64encode(
            json.dumps({"alg": "HS256", "sign_type": "SIGN"}).encode()
        ).rstrip(b"=").decode()

        now_ms = int(time.time() * 1000)
        payload = base64.urlsafe_b64encode(
            json.dumps({"api_key": api_id, "exp": now_ms + 3600_000, "timestamp": now_ms}).encode()
        ).rstrip(b"=").decode()

        sig_input = f"{header}.{payload}".encode()
        sig = base64.urlsafe_b64encode(
            hmac.new(api_secret.encode(), sig_input, hashlib.sha256).digest()
        ).rstrip(b"=").decode()

        return f"Bearer {header}.{payload}.{sig}"

    async def chat(
        self,
        messages: list[dict[str, str]],
        *,
        timeout: float = 300.0,
        max_retries: int = 3,
        retry_base_delay: float = 15.0,
    ) -> tuple[str, ModelUsage]:
        """Send chat completion via native httpx async (true cancellable timeout).

        Using httpx.AsyncClient directly ensures asyncio.wait_for() can truly
        cancel the request — no lingering background threads left behind.
        """
        import asyncio
        import httpx

        last_err: Exception | None = None

        for attempt in range(max_retries + 1):
            t0 = time.monotonic()
            try:
                async with httpx.AsyncClient(timeout=httpx.Timeout(timeout)) as client:
                    resp = await client.post(
                        _GLM_API_URL,
                        headers={
                            "Authorization": self._auth_header(),
                            "Content-Type": "application/json",
                        },
                        json={
                            "model": self.model_name,
                            "messages": messages,
                            "temperature": 0.1,
                        },
                    )

                latency = int((time.monotonic() - t0) * 1000)

                if resp.status_code == 429:
                    raise Exception(f"429 rate limit: {resp.text[:120]}")
                if resp.status_code >= 500:
                    raise Exception(f"{resp.status_code} server error: {resp.text[:120]}")
                resp.raise_for_status()

                data = resp.json()
                content = data["choices"][0]["message"]["content"] or ""
                usage_data = data.get("usage", {})
                input_tokens = usage_data.get("prompt_tokens", 0)
                output_tokens = usage_data.get("completion_tokens", 0)

                usage = ModelUsage(
                    model=self.model_name,
                    input_tokens=input_tokens,
                    output_tokens=output_tokens,
                    cost_usd=self._calc_cost(input_tokens, output_tokens),
                    latency_ms=latency,
                )
                return content, usage

            except Exception as e:
                import httpx as _httpx
                # httpx timeout exceptions have empty str() — re-raise as clear TimeoutError
                if isinstance(e, (_httpx.TimeoutException, _httpx.ConnectTimeout,
                                  _httpx.ReadTimeout, _httpx.WriteTimeout)):
                    raise TimeoutError(f"GLM API timeout after {timeout:.0f}s") from e

                last_err = e
                err_str = str(e)
                is_retryable = "429" in err_str or "500" in err_str or "502" in err_str or "503" in err_str
                if is_retryable and attempt < max_retries:
                    delay = retry_base_delay * (2 ** attempt)
                    logger.warning(
                        "GLM API error (attempt %d/%d): %s — retrying in %.0fs",
                        attempt + 1, max_retries + 1, err_str[:80], delay,
                    )
                    await asyncio.sleep(delay)
                else:
                    raise

        raise last_err  # type: ignore[misc]

    async def scan_file(
        self,
        file_content: str,
        file_path: str,
        *,
        prompt_strategy: str = "ctf",
        token_budget: int = 100_000,
        timeout: float = 600.0,
    ) -> ScanResult:
        """Scan a single file using CTF prompt strategy (Carlini method).

        timeout is passed through to the underlying chat() call. Per-file
        scans on large extracts can take 5+ minutes, so the default 300s
        chat timeout is too tight here — bump to 600s.
        """
        result = ScanResult(target=file_path, model=self.model_name)

        prompt = CTF_SCAN_PROMPT.format(file_path=file_path, file_content=file_content)
        messages = [{"role": "user", "content": prompt}]

        try:
            response_text, usage = await self.chat(messages, timeout=timeout)
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
