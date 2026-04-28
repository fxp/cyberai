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


def _extract_json_objects(text: str) -> list[dict]:
    """Extract all top-level JSON objects from text using brace depth tracking.

    Handles both one-JSON-per-line and pretty-printed multi-line JSON.
    Strips markdown code fences if present.
    """
    # Strip markdown fences
    text = text.replace("```json", "").replace("```", "")
    objects = []
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
                fragment = text[start:i + 1]
                try:
                    obj = json.loads(fragment)
                    objects.append(obj)
                except Exception:
                    pass
                start = None
    return objects


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

        # 按信号类型分组，若信号太多则截取最有价值的
        signals_for_prompt = all_signals
        if len(all_signals) > 60:
            # 优先保留 TAINT_SOURCE, TAINT_SINK, INFO_LEAK
            priority = ["TAINT_SOURCE", "TAINT_SINK", "INFO_LEAK",
                        "PARTIAL_CORRUPTION", "DOUBLE_FREE_RISK"]
            scored = sorted(all_signals,
                            key=lambda s: (priority.index(s.get("signal_type", ""))
                                           if s.get("signal_type") in priority else 99,
                                           -s.get("confidence", 0)))
            signals_for_prompt = scored[:60]
            print(f"  (truncated to top 60 signals by priority)")

        signals_text = build_signals_summary(signals_for_prompt)

        prompt = f"""Analyze these security signals from the codebase and find kill chains.

{signals_text}

Look for combinations that form complete exploit chains.
Output one JSON chain per line. If none viable, output the no_chains sentinel."""

        print(f"🔗 运行 kill chain builder (model={model})...")
        try:
            resp = client.chat.completions.create(
                model=model,
                messages=[
                    {"role": "system", "content": CHAIN_SYSTEM},
                    {"role": "user", "content": prompt},
                ],
                max_tokens=4096,
                temperature=0.1,
            )
            raw = resp.choices[0].message.content.strip()
        except Exception as e:
            print(f"❌ LLM call failed: {e}")
            out.write(json.dumps({"error": str(e)}) + "\n")
            return

        # 解析输出 — GLM 可能返回单行 JSON 或缩进多行 JSON
        # 用深度匹配提取所有顶层 JSON 对象（兼容两种格式）
        chain_count = 0
        extracted = _extract_json_objects(raw)
        if extracted:
            for obj in extracted:
                obj["_source"] = "chain_builder"
                out.write(json.dumps(obj, ensure_ascii=False) + "\n")
                chains.append(obj)
                if "chain_id" in obj:
                    chain_count += 1
        else:
            # 完全无法解析，保存原始响应（不截断）
            out.write(json.dumps({"raw_response": raw,
                                  "_source": "chain_builder"}) + "\n")
            print(f"⚠️  无法从响应中提取 JSON，已保存原始响应 ({len(raw)} chars)")

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
