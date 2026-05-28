#!/usr/bin/env python3
"""
Pipeline B — Step 2.5: Kill Chain Builder

读取所有 detect chunk 的信号（note_signal 输出），在全局视角下：
  1. 聚合所有 TAINT_SOURCE / TAINT_SINK / INFO_LEAK / PARTIAL_CORRUPTION 等信号
  2. 用 LLM 寻找可以串联的 kill chain
  3. 输出 chains.jsonl 供 verify_candidates.py 验证

Usage:
  python scripts/pipeline_b_chain.py \
      --input-dir results/ \
      --repo /tmp/target \
      --model glm-4-plus \
      --output results/chains.jsonl
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

CHAIN_SYSTEM = """You are an expert exploit developer analyzing a map of security signals across a codebase.

You have been given a set of weak security signals collected from multiple source files.
Each signal represents a partial vulnerability — alone it may not be exploitable.

Your task: find combinations of 2-4 signals that together form a complete kill chain.

## Kill chain patterns to look for

1. **Info leak + Memory corruption**
   - INFO_LEAK (exposes heap address / stack canary / ASLR base) +
     PARTIAL_CORRUPTION or TAINT_SINK (controlled write) =
     → ASLR bypass + arbitrary write = potential RCE

2. **Taint propagation**
   - TAINT_SOURCE (attacker input enters) +
     TAINT_SINK (dangerous op: exec/system/memcpy/write) =
     → Direct injection if no sanitization between them

3. **Race condition exploit**
   - RACE_WINDOW (TOCTOU gap) +
     PRIVILEGE_TRANSITION (privilege check happens in the window) =
     → Privilege escalation

4. **Use-After-Free**
   - DOUBLE_FREE_RISK (pointer freed) +
     TAINT_SOURCE (attacker controls allocation size/content) =
     → UAF with controlled content = type confusion or RCE

5. **Type confusion**
   - TYPE_CONFUSION (union/cast with attacker-controlled tag) +
     TAINT_SINK (pointer dereference of confused type) =
     → Controlled read/write

## Output format

For each chain you find, output EXACTLY one JSON line:

{
  "chain_id": "chain_001",
  "chain_type": "<brief type, e.g. 'info_leak+heap_spray=ASLR_bypass+RCE'>",
  "severity": "CRITICAL|HIGH|MEDIUM",
  "confidence": <50-90>,
  "steps": [
    {
      "step": 1,
      "signal_type": "INFO_LEAK",
      "location": "src/net.c:recv_packet:142",
      "role": "leaks heap base address via uninitialized padding",
      "provides": "heap_base_addr"
    },
    {
      "step": 2,
      "signal_type": "TAINT_SINK",
      "location": "src/alloc.c:pool_alloc:87",
      "role": "attacker-controlled size passed to malloc without overflow check",
      "requires": "heap_base_addr",
      "provides": "controlled_allocation"
    }
  ],
  "attack_narrative": "1. Send crafted X to trigger INFO_LEAK at A, learning heap layout. 2. Use Y to corrupt allocator metadata. 3. Next allocation lands at controlled address Z.",
  "missing_pieces": "Need to verify: can step 1 and step 2 be triggered in the same session without crashing?",
  "poc_sketch": "Pseudo-code or input sequence to trigger the chain"
}

Output only chains with confidence >= 50. If no viable chain exists, output exactly:
{"no_chains": true, "reason": "<why signals don't combine>"}
"""


_FENCE_BLOCK_RE = re.compile(
    r"```(?:json|javascript|js)?\s*\n?(.*?)\n?```",
    re.DOTALL,
)


def _extract_message_content(resp) -> str:
    """Extract text from a chat completion, handling reasoning models.

    GLM-5.1 / glm-z1-* are reasoning models — when prompted for a
    short JSON answer the model sometimes leaves `message.content`
    empty and puts the answer into `message.reasoning_content`.
    Fall back so we don't drop the response.
    """
    msg = resp.choices[0].message
    content = (getattr(msg, "content", None) or "").strip()
    if content:
        return content
    reasoning = (getattr(msg, "reasoning_content", None) or "").strip()
    return reasoning


def _extract_json_objects(text: str) -> list[dict]:
    """Extract all top-level JSON objects from text using brace depth tracking.

    Handles markdown fenced blocks (```json … ```), one-JSON-per-line,
    and pretty-printed multi-line JSON. Each fenced block is scanned
    independently before falling back to scanning the whole stripped
    text — this keeps a malformed fenced block from poisoning later
    objects.
    """
    text = (text or "").strip()
    if not text:
        return []

    # First: extract content of every fenced block, scan each, then
    # also remove fences and scan the remainder so unfenced JSON is
    # still picked up.
    fenced_segments = _FENCE_BLOCK_RE.findall(text)
    unfenced = _FENCE_BLOCK_RE.sub("\n", text)
    segments = list(fenced_segments) + [unfenced]

    seen: set[str] = set()
    objects: list[dict] = []
    for seg in segments:
        depth = 0
        start = None
        for i, ch in enumerate(seg):
            if ch == "{":
                if depth == 0:
                    start = i
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0 and start is not None:
                    fragment = seg[start:i + 1]
                    try:
                        obj = json.loads(fragment)
                    except Exception:
                        pass
                    else:
                        # de-dup identical objects across segments
                        key = json.dumps(obj, sort_keys=True)[:512]
                        if key not in seen:
                            seen.add(key)
                            objects.append(obj)
                    start = None
    return objects


def _call_chain_llm(signals: list[dict], client, model: str,
                    round_label: str) -> list[dict]:
    """Send one batch of signals to the LLM and extract kill chain JSON objects.

    Returns a list of parsed dicts (may include no_chains sentinel or error entries).
    """
    if not signals:
        return []

    signals_text = build_signals_summary(signals)
    prompt = f"""Analyze these security signals from the codebase and find kill chains.

{signals_text}

Look for combinations that form complete exploit chains.
Output one JSON chain per line. If none viable, output the no_chains sentinel."""

    print(f"  🔗 {round_label} ({len(signals)} signals, model={model})...")
    try:
        resp = client.chat.completions.create(
            model=model,
            messages=[
                {"role": "system", "content": CHAIN_SYSTEM},
                {"role": "user", "content": prompt},
            ],
            max_tokens=8192,
            temperature=0.1,
        )
        raw = _extract_message_content(resp)
    except Exception as e:
        print(f"  ❌ LLM call failed: {e}")
        return [{"error": str(e)}]

    extracted = _extract_json_objects(raw)
    if extracted:
        return extracted
    # One retry with stronger json-only nudge — reasoning models
    # sometimes obey better on the second turn.
    print(f"  ⚠️  无法解析 LLM 输出 ({len(raw)} chars), retrying with json-only nudge")
    try:
        resp = client.chat.completions.create(
            model=model,
            messages=[
                {"role": "system", "content": CHAIN_SYSTEM},
                {"role": "user", "content": prompt + (
                    "\n\n---\nOutput ONLY one or more JSON objects. "
                    "No prose, no markdown fences. Each object on its own line."
                )},
            ],
            max_tokens=8192,
            temperature=0.05,
        )
        raw2 = _extract_message_content(resp)
    except Exception as e:
        print(f"  ❌ retry LLM call failed: {e}")
        return [{"raw_response": raw}]
    extracted = _extract_json_objects(raw2)
    if extracted:
        return extracted
    return [{"raw_response": raw or raw2}]


def build_signals_summary(signals: list[dict]) -> str:
    """Format signals for the LLM prompt."""
    by_type = {}
    for s in signals:
        t = s.get("signal_type", "UNKNOWN")
        by_type.setdefault(t, []).append(s)

    lines = [f"Total signals: {len(signals)}\n"]
    for sig_type, items in sorted(by_type.items()):
        lines.append(f"\n## {sig_type} ({len(items)} signals)")
        for i, s in enumerate(items, 1):
            lines.append(f"\n  [{i}] Location: {s.get('location', s.get('source_file', '?'))}")
            lines.append(f"      Description: {s.get('description', '?')[:120]}")
            lines.append(f"      Data: {s.get('data_involved', '?')[:80]}")
            lines.append(f"      Reach: {s.get('attacker_reach', '?')[:80]}")
            lines.append(f"      Chain potential: {s.get('chain_potential', '?')[:100]}")
            lines.append(f"      Confidence: {s.get('confidence', '?')}")

    return "\n".join(lines)


def run_chain_builder(input_dir: Path, output: Path, model: str,
                      repo_root: Path | None = None):
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

    client = ZhipuAI(api_key=key)

    # 收集所有信号（_record_type == "signal"）
    all_signals = []
    all_findings = []
    for jf in sorted(input_dir.glob("**/*.jsonl")):
        if jf.name.startswith("chains") or jf.name.startswith("verified"):
            continue
        with open(jf) as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                try:
                    obj = json.loads(line)
                    rt = obj.get("_record_type", "finding")
                    if rt == "signal":
                        if obj.get("confidence", 0) >= 60:
                            all_signals.append(obj)
                    else:
                        all_findings.append(obj)
                except Exception:
                    pass

    print(f"📡 收集到 {len(all_signals)} 个信号, {len(all_findings)} 个直接 findings")

    output.parent.mkdir(parents=True, exist_ok=True)

    # 直接 findings 直通到 chains.jsonl（已是完整漏洞）
    chains = []
    with open(output, "w") as out:
        for f in all_findings:
            f["chain_type"] = "standalone_finding"
            f["_source"] = "report_finding"
            out.write(json.dumps(f, ensure_ascii=False) + "\n")
            chains.append(f)

        if not all_signals:
            print("⚠️  No signals to chain — skipping chain builder LLM call")
            if not all_findings:
                out.write(json.dumps({"no_chains": True,
                                      "reason": "No signals or findings collected"}) + "\n")
            return

        # ── 两轮信号处理 ────────────────────────────────────────────
        # Round 1: 主链路信号（污点传播、内存损坏、信息泄露）— 最多 50 个
        # Round 2: 特殊信号（竞态、类型混淆、权限转换）— 最多 30 个
        # 分开送 LLM，避免高噪信号稀释低频但高价值的 race/type confusion 信号
        HIGH_PRIORITY = {"TAINT_SOURCE", "TAINT_SINK", "INFO_LEAK",
                         "PARTIAL_CORRUPTION", "DOUBLE_FREE_RISK"}
        SPECIAL_TYPES = {"RACE_WINDOW", "TYPE_CONFUSION", "PRIVILEGE_TRANSITION"}

        high_sigs = sorted(
            [s for s in all_signals if s.get("signal_type") in HIGH_PRIORITY],
            key=lambda s: -s.get("confidence", 0)
        )[:50]

        special_sigs = sorted(
            [s for s in all_signals if s.get("signal_type") in SPECIAL_TYPES],
            key=lambda s: -s.get("confidence", 0)
        )[:30]

        print(f"🔗 Kill chain builder: "
              f"{len(high_sigs)} 主链路信号 + {len(special_sigs)} 特殊信号")

        r1_objects = _call_chain_llm(high_sigs, client, model,
                                     "Round 1 — taint/corruption/leak")
        r2_objects = (_call_chain_llm(special_sigs, client, model,
                                      "Round 2 — race/type-confusion/privilege")
                      if special_sigs else [])

        # 合并两轮结果，顺序重新编号 chain_id 避免冲突
        chain_count = 0
        for obj in r1_objects + r2_objects:
            obj["_source"] = "chain_builder"
            if "chain_id" in obj:
                chain_count += 1
                obj["chain_id"] = f"chain_{chain_count:03d}"
            out.write(json.dumps(obj, ensure_ascii=False) + "\n")
            chains.append(obj)

    # 打印摘要
    confirmed_chains = [c for c in chains if "chain_id" in c]
    print(f"\n{'='*60}")
    print(f"✅ Chain 分析完成")
    print(f"   直接 findings: {len(all_findings)}")
    print(f"   弱信号数量:    {len(all_signals)}")
    print(f"   发现 chains:   {len(confirmed_chains)}")
    print(f"   输出文件:      {output}")

    for c in confirmed_chains:
        sev = c.get("severity", "?")
        conf = c.get("confidence", "?")
        ctype = c.get("chain_type", "?")
        print(f"\n  🔗 [{sev}] {ctype} (conf={conf})")
        for step in c.get("steps", []):
            print(f"     Step {step.get('step')}: {step.get('signal_type')} @ {step.get('location','?')}")
            print(f"       → {step.get('role','?')[:80]}")
        if c.get("attack_narrative"):
            print(f"     narrative: {c['attack_narrative'][:150]}")


def main():
    parser = argparse.ArgumentParser(description="Pipeline B kill chain builder")
    parser.add_argument("--input-dir", type=Path, required=True,
                        help="Directory containing chunk_*.jsonl files")
    parser.add_argument("--output", type=Path, required=True,
                        help="Output JSONL path for chains")
    parser.add_argument("--model", default="glm-4-plus",
                        help="LLM model for chain analysis")
    parser.add_argument("--repo", type=Path, default=None,
                        help="Target repo root (for context lookups)")
    args = parser.parse_args()

    run_chain_builder(
        input_dir=args.input_dir,
        output=args.output,
        model=args.model,
        repo_root=args.repo,
    )


if __name__ == "__main__":
    main()
