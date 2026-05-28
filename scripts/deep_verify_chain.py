#!/usr/bin/env python3
"""
Deep Kill Chain Verifier — Step 3.5

针对 chain_builder 发现的 kill chain，拉取真实源代码逐步复核：
  1. 读取 chains.jsonl
  2. 对每个 chain，从 repo 中读取每个 step 的真实代码上下文
  3. 让 LLM 逐步追踪：每个信号是否真实存在？各步骤之间是否确实可串联？
  4. 输出 deep_verified.jsonl + 打印摘要

Usage:
  python scripts/deep_verify_chain.py \
      --chains results/chains.jsonl \
      --repo /tmp/target \
      --model glm-4-plus \
      --output results/deep_verified.jsonl
"""
import argparse
import json
import os
import re
import sys
import time
from pathlib import Path

try:
    from zhipuai import ZhipuAI
except ImportError:
    print("请安装 zhipuai: pip install zhipuai")
    sys.exit(1)

DEEP_VERIFY_SYSTEM = """You are an elite offensive security researcher doing final exploit validation.

You have been given a kill chain hypothesis AND the actual source code for each step.
Your job: determine if this kill chain is REAL and EXPLOITABLE as described.

## Verification methodology — check each step independently, then the chain

### Per-step checks
1. Does the flagged code pattern actually exist at the stated location?
2. Is the attacker-controlled data truly unsanitized at that point?
3. Is there a guard between the source and sink that blocks the flow?
4. What exactly is the attacker input shape needed to trigger this step?

### Chain-level checks
5. Can steps 1→2→...→N be triggered in the SAME session/request?
6. Does step N provide what step N+1 requires (the `provides`/`requires` link)?
7. Is there a timing or privilege constraint that breaks the chain?
8. Is this reachable from an unauthenticated external attacker?

## Known-safe patterns — reject immediately if present
- `strdup(s)` / `malloc(strlen(s)+1)`: dynamic allocation, no overflow.
- `snprintf(dst, sizeof(dst), ...)`: bounded write, safe.
- Code inside test/, fuzz/, examples/: not production-reachable.
- Null checks + abort after malloc: NOT a UAF.
- `atoi()` on a PID followed by `if (pid > 0)`: NOT command injection.

## Output — EXACTLY one JSON line

{
  "verdict": "CONFIRMED" | "FALSE_POSITIVE" | "PARTIAL" | "NEEDS_MORE_CONTEXT",
  "confidence": <integer 0-100>,
  "confirmed_steps": [<list of step numbers that are real>],
  "broken_at": <step number where chain breaks, or null if CONFIRMED>,
  "break_reason": "<why the chain breaks at that step, or null>",
  "exploitable_as": "<simplified chain if PARTIAL — e.g. step 1+2 alone are exploitable>",
  "exact_input": "<the exact attacker input / packet / file that triggers the chain>",
  "reason": "<precise technical justification, reference actual code lines>",
  "cvss_estimate": "<e.g. AV:N/AC:L/PR:N/UI:N/S:U/C:H/I:H/A:H>"
}

CONFIRMED: every step verified, no blocking guard, chain is end-to-end exploitable.
PARTIAL: some steps verified but chain breaks — the partial sub-chain may still be a real finding.
FALSE_POSITIVE: key signal doesn't exist or is guarded at every step.
NEEDS_MORE_CONTEXT: missing critical code that would determine exploitability."""


def get_glm_client(model: str) -> tuple:
    key = os.environ.get("GLM_API_KEY", "")
    if not key:
        env_file = Path(__file__).resolve().parent.parent / ".env"
        if env_file.exists():
            for line in env_file.read_text().splitlines():
                if line.startswith("GLM_API_KEY="):
                    key = line.split("=", 1)[1].strip()
    if not key:
        print("❌ GLM_API_KEY not found")
        sys.exit(1)
    return ZhipuAI(api_key=key), model


def read_source_context(repo_root: Path, location: str, context_lines: int = 40) -> str:
    """Read source code around the stated location.

    location format: "path/to/file.c:function:lineno"  or  "path/to/file.c:lineno"
    """
    if not location or location == "?":
        return "(location unknown)"

    # parse location
    parts = location.split(":")
    file_part = parts[0]
    lineno = None
    for p in parts[1:]:
        if p.isdigit():
            lineno = int(p)
            break

    candidate = repo_root / file_part
    if not candidate.exists():
        # try stripping leading path components (e.g. "src/net.c" → "net.c")
        found = False
        for depth in range(1, 4):
            parts_stripped = Path(file_part).parts[depth:]
            if not parts_stripped:
                break
            stripped = Path(*parts_stripped)
            candidate = repo_root / stripped
            if candidate.exists():
                found = True
                break
        if not found:
            print(f"    ⚠️  source not found: {file_part} (tried {repo_root})")
            return f"(file not found: {file_part})"

    try:
        lines = candidate.read_text(errors="replace").splitlines()
    except Exception as e:
        return f"(read error: {e})"

    if lineno and 1 <= lineno <= len(lines):
        start = max(0, lineno - context_lines // 2)
        end = min(len(lines), lineno + context_lines // 2)
        snippet = lines[start:end]
        numbered = [f"{start + i + 1:5d}  {l}" for i, l in enumerate(snippet)]
        return f"// {candidate.relative_to(repo_root)} (lines {start+1}-{end})\n" + "\n".join(numbered)
    else:
        # return first N lines of the file
        snippet = lines[:context_lines * 2]
        numbered = [f"{i+1:5d}  {l}" for i, l in enumerate(snippet)]
        return f"// {candidate.relative_to(repo_root)} (first {len(snippet)} lines)\n" + "\n".join(numbered)


def build_chain_prompt(chain: dict, repo_root: Path) -> str:
    steps = chain.get("steps", [])
    chain_type = chain.get("chain_type", "?")
    severity = chain.get("severity", "?")
    confidence = chain.get("confidence", "?")
    narrative = chain.get("attack_narrative", "")
    missing = chain.get("missing_pieces", "")
    poc = chain.get("poc_sketch", "")

    sections = [
        f"## Kill Chain Hypothesis\n"
        f"- ID: {chain.get('chain_id','?')}\n"
        f"- Type: {chain_type}\n"
        f"- Severity: {severity} | Confidence: {confidence}\n"
        f"- Attack narrative: {narrative}\n"
        f"- Missing pieces flagged by detector: {missing}\n"
        f"- PoC sketch: {poc}\n",
    ]

    for step in steps:
        loc = step.get("location", "?")
        sig = step.get("signal_type", "?")
        role = step.get("role", "?")
        provides = step.get("provides", "?")
        requires = step.get("requires", "")
        code = read_source_context(repo_root, loc)
        section = (
            f"## Step {step.get('step')}: [{sig}]\n"
            f"- Location: {loc}\n"
            f"- Role: {role}\n"
            f"- Provides: {provides}\n"
        )
        if requires:
            section += f"- Requires: {requires}\n"
        section += f"\n### Actual source code\n```c\n{code}\n```\n"
        sections.append(section)

    return "\n".join(sections)


def _extract_message_content(resp) -> str:
    """Extract text from chat completion, handling reasoning models.

    GLM-5.1 / glm-z1-* are reasoning models: when the model emits a
    visible answer, it goes to `message.content`; the chain-of-thought
    goes to `message.reasoning_content`. For some prompts (especially
    short or strict-JSON), the model puts the JSON in reasoning_content
    and leaves content empty. Fall back to reasoning_content in that
    case so we don't lose the answer.
    """
    msg = resp.choices[0].message
    content = (getattr(msg, "content", None) or "").strip()
    if content:
        return content
    reasoning = (getattr(msg, "reasoning_content", None) or "").strip()
    return reasoning


_FENCE_RE = re.compile(
    r"^```(?:json|javascript|js)?\s*\n?(.*?)\n?```\s*$",
    re.DOTALL,
)


def _extract_json(text: str) -> dict | None:
    """Extract a JSON object from text. Robust to markdown fences."""
    text = (text or "").strip()
    if not text:
        return None
    # Strip markdown fence if the whole response is fenced
    m = _FENCE_RE.match(text)
    if m:
        text = m.group(1).strip()
    # Direct parse
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        pass
    # Brace-match fallback (first balanced { ... })
    depth = 0
    start = None
    for i, ch in enumerate(text):
        if ch == "{":
            if depth == 0:
                start = i
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0 and start is not None:
                try:
                    return json.loads(text[start:i + 1])
                except json.JSONDecodeError:
                    start = None
    return None


_JSON_ONLY_REMINDER = (
    "Output ONLY a single JSON object with the required schema. "
    "No prose, no markdown fences. Start with `{` and end with `}`."
)


def deep_verify_chain(chain: dict, client, model: str,
                      repo_root: Path) -> dict:
    """Run deep verification on a single kill chain.

    Tries up to 2 attempts: if the first response can't be parsed,
    retries with a stronger json-only reminder appended to the prompt.
    """
    prompt = build_chain_prompt(chain, repo_root)
    last_raw = ""
    for attempt in range(2):
        user_content = prompt if attempt == 0 else (
            prompt + "\n\n---\n" + _JSON_ONLY_REMINDER
        )
        try:
            resp = client.chat.completions.create(
                model=model,
                messages=[
                    {"role": "system", "content": DEEP_VERIFY_SYSTEM},
                    {"role": "user", "content": user_content},
                ],
                max_tokens=4096,
                temperature=0.05,
            )
        except Exception as e:
            return {"verdict": "ERROR", "reason": str(e)}
        last_raw = _extract_message_content(resp)
        result = _extract_json(last_raw)
        if result:
            return result
        # else: retry once with stronger reminder
    return {"verdict": "PARSE_ERROR", "raw": last_raw[:300]}


def main():
    parser = argparse.ArgumentParser(description="Deep kill chain verifier")
    parser.add_argument("--chains", type=Path, required=True,
                        help="chains.jsonl from pipeline_b_chain.py")
    parser.add_argument("--repo", type=Path, required=True,
                        help="Cloned target repo root")
    parser.add_argument("--model", default="glm-4-plus",
                        help="LLM model for deep verification")
    parser.add_argument("--output", type=Path, required=True,
                        help="Output JSONL for deep verification results")
    parser.add_argument("--min-confidence", type=int, default=60,
                        help="Only verify chains with confidence >= this (default 60)")
    args = parser.parse_args()

    client, model = get_glm_client(args.model)

    # Load kill chains only (skip standalone findings and no_chains sentinels)
    chains = []
    with open(args.chains) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                obj = json.loads(line)
                if "chain_id" in obj and obj.get("confidence", 0) >= args.min_confidence:
                    chains.append(obj)
            except Exception:
                pass

    if not chains:
        print("⚠️  No kill chains to deep-verify (check --min-confidence)")
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text("")
        return

    repo_root = args.repo.resolve()
    print(f"🔬 深度复核 {len(chains)} 条 kill chains (repo={repo_root})")
    print(f"{'Chain ID':<15} {'Type':<40} {'Verdict':<20} Confidence")
    print("-" * 100)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    confirmed = []
    partial = []

    with open(args.output, "w") as out:
        for chain in chains:
            cid = chain.get("chain_id", "?")
            ctype = chain.get("chain_type", "?")[:38]
            print(f"  {cid:<13} {ctype:<40}", end=" ", flush=True)

            result = deep_verify_chain(chain, client, model, repo_root)
            verdict = result.get("verdict", "?")
            conf = result.get("confidence", "?")
            print(f"{verdict:<20} {conf}")

            result["_chain"] = chain
            out.write(json.dumps(result, ensure_ascii=False) + "\n")

            if verdict == "CONFIRMED":
                confirmed.append((chain, result))
            elif verdict == "PARTIAL":
                partial.append((chain, result))

            time.sleep(2)

    # Summary
    print(f"\n{'='*70}")
    print(f"✅ 深度复核完成: {len(chains)} 条链")
    print(f"   🔴 CONFIRMED:  {len(confirmed)}")
    print(f"   🟡 PARTIAL:    {len(partial)}")
    print(f"   📄 结果:        {args.output}")

    for chain, result in confirmed:
        print(f"\n  🔴 {chain.get('chain_id')} — {chain.get('chain_type')}")
        print(f"     Severity: {chain.get('severity')} | Conf: {result.get('confidence')}")
        print(f"     Input: {result.get('exact_input','?')[:120]}")
        print(f"     CVSS: {result.get('cvss_estimate','?')}")
        print(f"     Reason: {result.get('reason','')[:150]}")

    for chain, result in partial:
        sub = result.get("exploitable_as", "?")
        broken = result.get("broken_at", "?")
        print(f"\n  🟡 {chain.get('chain_id')} PARTIAL — breaks at step {broken}")
        print(f"     Sub-chain still exploitable: {sub}")
        print(f"     Break reason: {result.get('break_reason','')[:100]}")

    if not confirmed and not partial:
        print("\n  ⚪ 所有 kill chain 均为 FALSE_POSITIVE 或需要更多上下文")


if __name__ == "__main__":
    main()
