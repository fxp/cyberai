#!/usr/bin/env python3
"""
Phase 2 二次验证脚本 — 高置信候选逐一深度确认
策略: 对每个 extract 提供相邻段落 (前后各一段), 使用更严格的 FP 过滤 prompt.
"""
import os, sys, json, time, glob
from pathlib import Path

try:
    from zhipuai import ZhipuAI
except ImportError:
    print("请安装 zhipuai: pip install zhipuai"); sys.exit(1)

BASE = Path("/Users/xiaopingfeng/Projects/cyberai")
GLM_KEY = os.environ.get("GLM_API_KEY", "")
if not GLM_KEY:
    env_file = BASE / ".env"
    if env_file.exists():
        for line in env_file.read_text().splitlines():
            if line.startswith("GLM_API_KEY="):
                GLM_KEY = line.split("=",1)[1].strip()
if not GLM_KEY:
    print("❌ GLM_API_KEY not found"); sys.exit(1)

client = ZhipuAI(api_key=GLM_KEY)

VERIFY_SYSTEM = """You are a senior security researcher performing skeptical vulnerability verification.
Your job is to REJECT false positives, not find new bugs.

Given a code excerpt and a claimed vulnerability, determine:
1. Does the vulnerable code actually exist in this excerpt?
2. Is there an existing bounds check, guard, or validation that prevents exploitation?
3. Is the claimed exploit path actually reachable with realistic inputs?
4. Has this pattern already been fixed by a known CVE?

Output EXACTLY one of:
{"verdict":"CONFIRMED","confidence":<70-100>,"reason":"<why it's real>","poc_path":"<exact trigger>"}
{"verdict":"FALSE_POSITIVE","confidence":<70-100>,"reason":"<which existing guard prevents it>"}
{"verdict":"NEEDS_MORE_CONTEXT","confidence":<50-70>,"reason":"<what additional code is needed>"}

Be highly skeptical. Only CONFIRMED if you can trace the full exploit path with no blocking guards."""

def load_extract_with_context(extract_path: Path) -> str:
    """Load extract + previous and next segments for more context."""
    parts = extract_path.stem.rsplit('_', 1)
    if len(parts) == 2:
        base_name, seg_letter = parts[0], parts[1]
        # Find all segments for this file
        siblings = sorted(extract_path.parent.glob(f"{base_name}_*.c"))
        idx = siblings.index(extract_path) if extract_path in siblings else -1

        code_parts = []
        # Previous segment (context)
        if idx > 0:
            code_parts.append(f"// === PREVIOUS SEGMENT (context) ===\n{siblings[idx-1].read_text(errors='replace')[:2000]}")
        # Current segment (focus)
        code_parts.append(f"// === CURRENT SEGMENT (under review) ===\n{extract_path.read_text(errors='replace')}")
        # Next segment (context)
        if idx >= 0 and idx < len(siblings) - 1:
            code_parts.append(f"// === NEXT SEGMENT (context) ===\n{siblings[idx+1].read_text(errors='replace')[:2000]}")

        return "\n\n".join(code_parts)[:8000]
    return extract_path.read_text(errors='replace')[:6000]


def verify_candidate(target: str, finding: dict, extracts_dir: Path) -> dict:
    """Run second-pass verification on a single finding."""
    src_file = finding.get('source_file', '')
    extract_path = extracts_dir / src_file

    if not extract_path.exists():
        return {"verdict": "SKIP", "reason": "extract not found"}

    code = load_extract_with_context(extract_path)

    claim = f"""Claimed vulnerability:
- Type: {finding.get('type', '?')}
- Description: {finding.get('description', '?')}
- PoC hint: {finding.get('poc_hint', '?')}
- Severity: {finding.get('severity', '?')}

Code to verify:
{code}"""

    try:
        resp = client.chat.completions.create(
            model="glm-4-flash",
            messages=[
                {"role": "system", "content": VERIFY_SYSTEM},
                {"role": "user", "content": claim}
            ],
            max_tokens=600,
            temperature=0.05,
        )
        raw = resp.choices[0].message.content.strip()
        # Parse JSON response
        for line in raw.splitlines():
            line = line.strip()
            if line.startswith('{'):
                try:
                    return json.loads(line)
                except:
                    pass
        return {"verdict": "PARSE_ERROR", "raw": raw[:200]}
    except Exception as e:
        return {"verdict": "ERROR", "reason": str(e)}


def main():
    # Collect top candidates from all targets
    targets_map = {
        "linux_bpf":   BASE / "scripts/extracts/linux_bpf",
        "linux_kvm":   BASE / "scripts/extracts/linux_kvm",
        "linux_usb":   BASE / "scripts/extracts/linux_usb",
        "libxml2":     BASE / "scripts/extracts/libxml2",
        "poppler":     BASE / "scripts/extracts/poppler",
        "imagemagick": BASE / "scripts/extracts/imagemagick_712",
    }

    candidates = []
    for target, extracts_dir in targets_map.items():
        jsonl_files = list((BASE / f"research/{target}").glob("t1_glm_phase2_*.jsonl"))
        if not jsonl_files:
            continue

        findings = []
        for jf in jsonl_files:
            with open(jf) as f:
                for line in f:
                    line = line.strip()
                    if line:
                        try: findings.append(json.loads(line))
                        except: pass

        # Sort by confidence, take top candidates
        # Priority: CRITICAL first, then by confidence, deduplicate by type+file
        findings.sort(key=lambda x: (
            0 if x.get('severity') == 'CRITICAL' else 1,
            -x.get('confidence', 0)
        ))

        seen = set()
        for f in findings:
            key = (f.get('type','')[:20], f.get('source_file','')[:30])
            if key not in seen:
                seen.add(key)
                f['_target'] = target
                f['_extracts_dir'] = str(extracts_dir)
                candidates.append(f)
            if len([c for c in candidates if c.get('_target') == target]) >= 5:
                break

    print(f"🔍 二次验证: {len(candidates)} 个候选 (每 target 最多5个)")
    print(f"{'Target':<14} {'Type':<30} {'File':<35} {'Verdict':<15} {'Reason'}")
    print("-"*130)

    results = []
    confirmed = []

    for i, cand in enumerate(candidates):
        target = cand.get('_target','?')
        extracts_dir = Path(cand.get('_extracts_dir', '.'))

        print(f"  [{i+1:2d}/{len(candidates)}] {target:<12} {cand.get('type','?')[:28]:<30} {cand.get('source_file','?')[:33]:<35}", end=" ", flush=True)

        result = verify_candidate(target, cand, extracts_dir)
        verdict = result.get('verdict', '?')
        reason = result.get('reason', '')[:60]

        print(f"{verdict:<15} {reason}")

        result['_target'] = target
        result['_finding'] = cand
        results.append(result)

        if verdict == 'CONFIRMED':
            confirmed.append((target, cand, result))

        time.sleep(15)  # rate limit

    # Save results
    out_file = BASE / f"research/phase2_verification.jsonl"
    with open(out_file, 'w') as f:
        for r in results:
            f.write(json.dumps(r, ensure_ascii=False) + '\n')

    print(f"\n{'='*80}")
    print(f"✅ 验证完成: {len(results)} 个候选")
    print(f"✅ CONFIRMED: {len(confirmed)}")
    print(f"📄 结果: {out_file}")

    if confirmed:
        print(f"\n🔴 已确认漏洞:")
        for target, finding, result in confirmed:
            print(f"\n  [{target}] {finding.get('source_file')}")
            print(f"  类型: {finding.get('type')} | 严重性: {finding.get('severity')} | 置信度: {finding.get('confidence')}")
            print(f"  描述: {finding.get('description','')[:120]}")
            print(f"  验证原因: {result.get('reason','')[:120]}")
            print(f"  PoC路径: {result.get('poc_path','N/A')[:120]}")
    else:
        print("\n⚠️  没有候选通过二次验证 (均为FP)")
        print("   建议: 使用 Pipeline B 进行容器化扫描")

if __name__ == '__main__':
    main()
