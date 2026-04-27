#!/usr/bin/env python3
"""
二次验证脚本 — 对 Pipeline A/B 的 findings 做 skeptical 深度确认

两种运行模式:

  # Pipeline B 模式（GitHub Actions 用）
  python verify_candidates.py \
      --input-dir results/ \
      --model glm-4-flash \
      --output results/verified.jsonl

  # Phase 2 研究模式（本地用，不带参数调用原有逻辑）
  python verify_candidates.py --phase2
"""
import os, sys, json, time, argparse
from pathlib import Path

try:
    from zhipuai import ZhipuAI
except ImportError:
    print("请安装 zhipuai: pip install zhipuai"); sys.exit(1)

BASE = Path(__file__).resolve().parent.parent

VERIFY_SYSTEM = """You are a senior security researcher performing skeptical vulnerability verification.
Your ONLY job is to REJECT false positives. Assume the finding is wrong until proven otherwise.

## Known-safe patterns → always FALSE_POSITIVE

- `strdup(s)` / `strndup(s,n)` / `mosquitto_strdup(s)`: heap-allocates exactly len(s)+1 bytes.
  There is NO fixed-size buffer. This CANNOT cause a buffer overflow by itself.
- `malloc(strlen(s) + 1)`: same — dynamic allocation sized to the string.
- `snprintf(dst, sizeof(dst), ...)` or `snprintf(dst, N, ...)` with explicit size N: bounded, safe.
- `calloc(n, sz)`: POSIX guarantees NULL return on n*sz overflow; if caller checks NULL it is safe.
- Any code path inside test/, tests/, examples/, fuzz/ — not reachable in production.
- A NULL-check after malloc that aborts on failure is NOT a UAF vulnerability.
- `atoi()` followed by `if (val > 0)` guards a PID kill — not a command injection.

## Verification checklist

1. Does the vulnerable pattern ACTUALLY exist in the provided code? (not hallucinated)
2. Is there a guard in the code or in any named caller that prevents the exploit?
3. Is the input reachable from an unauthenticated external attacker?
4. Would a real attacker be able to trigger this without special privileges?

## Output — EXACTLY one JSON line

{"verdict":"CONFIRMED","confidence":<80-100>,"reason":"<precise exploit chain>","poc_path":"<exact input>"}
{"verdict":"FALSE_POSITIVE","confidence":<80-100>,"reason":"<specific guard or safe pattern>"}
{"verdict":"NEEDS_MORE_CONTEXT","confidence":<50-79>,"reason":"<what specific code is missing>"}

Only CONFIRMED when you can name the exact input value and trace every step with no blocking guard."""


def get_glm_client(model: str = "glm-4-flash"):
    key = os.environ.get("GLM_API_KEY", "")
    if not key:
        env_file = BASE / ".env"
        if env_file.exists():
            for line in env_file.read_text().splitlines():
                if line.startswith("GLM_API_KEY="):
                    key = line.split("=", 1)[1].strip()
    if not key:
        print("❌ GLM_API_KEY not found"); sys.exit(1)
    return ZhipuAI(api_key=key), model


def verify_one(finding: dict, client, model: str,
               repo_root: Path | None = None) -> dict:
    """Run second-pass verification on a single finding."""

    # 构建代码上下文
    code_context = ""

    # Pipeline B findings 包含 exploit_path，直接用
    if finding.get("exploit_path"):
        code_context = f"[Exploit path traced by detector]: {finding['exploit_path']}"

    # Pipeline A findings 有 source_file，尝试读取 extract
    src_file = finding.get("source_file", "")
    if not code_context and src_file and repo_root:
        candidate = repo_root / src_file
        if candidate.exists():
            code_context = candidate.read_text(errors="replace")[:6000]

    if not code_context:
        code_context = "(source code not available — verify based on description only)"

    claim = f"""Claimed vulnerability:
- Type: {finding.get('type', '?')}
- Severity: {finding.get('severity', '?')}
- Description: {finding.get('description', '?')}
- PoC hint: {finding.get('poc_hint') or finding.get('exploit_path') or '?'}
- Location: {finding.get('location') or finding.get('source_file', '?')}

Code / context:
{code_context}"""

    try:
        resp = client.chat.completions.create(
            model=model,
            messages=[
                {"role": "system", "content": VERIFY_SYSTEM},
                {"role": "user", "content": claim}
            ],
            max_tokens=600,
            temperature=0.05,
        )
        raw = resp.choices[0].message.content.strip()
        for line in raw.splitlines():
            line = line.strip()
            if line.startswith('{'):
                try:
                    return json.loads(line)
                except Exception:
                    pass
        return {"verdict": "PARSE_ERROR", "raw": raw[:200]}
    except Exception as e:
        return {"verdict": "ERROR", "reason": str(e)}


# ── Pipeline B 模式 ───────────────────────────────────────────────
def run_pipeline_b_mode(input_dir: Path, output: Path, model: str,
                        top_n: int = 30, repo_root: Path | None = None):
    """Read all JSONL from input_dir, verify top candidates, write verified.jsonl"""
    client, model = get_glm_client(model)

    # 收集所有 findings
    all_findings = []
    for jf in sorted(input_dir.glob("**/*.jsonl")):
        with open(jf) as f:
            for line in f:
                line = line.strip()
                if line:
                    try:
                        obj = json.loads(line)
                        obj.setdefault("_source_file", str(jf.name))
                        all_findings.append(obj)
                    except Exception:
                        pass

    if not all_findings:
        print("⚠️  No findings found in input-dir")
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text("")
        return

    # 排序：CRITICAL 优先，然后按置信度
    all_findings.sort(key=lambda x: (
        0 if x.get("severity") == "CRITICAL" else 1,
        -x.get("confidence", 0)
    ))

    # 去重（同类型+同文件最多保留1个）
    seen = set()
    unique = []
    for f in all_findings:
        key = (f.get("type", "")[:25], f.get("source_file", f.get("location", ""))[:35])
        if key not in seen:
            seen.add(key)
            unique.append(f)

    candidates = unique[:top_n]
    print(f"🔍 二次验证: {len(candidates)} 个候选 (原始 {len(all_findings)} findings)")
    print(f"{'#':<4} {'Type':<28} {'File':<35} {'Verdict':<15} Reason")
    print("-" * 110)

    results = []
    confirmed = []
    output.parent.mkdir(parents=True, exist_ok=True)

    with open(output, "w") as out:
        for i, cand in enumerate(candidates):
            src = cand.get("source_file") or cand.get("location") or "?"
            print(f"  [{i+1:2d}/{len(candidates)}] "
                  f"{cand.get('type','?')[:26]:<28} {src[:33]:<35}", end=" ", flush=True)

            result = verify_one(cand, client, model, repo_root)
            verdict = result.get("verdict", "?")
            reason = result.get("reason", "")[:55]
            print(f"{verdict:<15} {reason}")

            result["_finding"] = cand
            results.append(result)
            out.write(json.dumps(result, ensure_ascii=False) + "\n")

            if verdict == "CONFIRMED":
                confirmed.append((cand, result))

            time.sleep(3)  # rate limit

    print(f"\n{'='*70}")
    print(f"✅ 验证完成: {len(results)} 个")
    print(f"{'🔴' if confirmed else '✅'} CONFIRMED: {len(confirmed)}")
    print(f"📄 结果: {output}")

    if confirmed:
        print("\n🔴 已确认漏洞:")
        for finding, result in confirmed:
            print(f"\n  {finding.get('source_file') or finding.get('location')}")
            print(f"  类型: {finding.get('type')} | 严重性: {finding.get('severity')} "
                  f"| 置信度: {finding.get('confidence')}")
            print(f"  描述: {str(finding.get('description',''))[:120]}")
            print(f"  验证: {result.get('reason','')[:120]}")
            print(f"  PoC:  {result.get('poc_path','N/A')[:120]}")
    else:
        print("\n⚠️  没有候选通过二次验证")


# ── Phase 2 研究模式（原有逻辑，保留向后兼容）────────────────────
def run_phase2_mode():
    """Original Phase 2 research verification (local, hardcoded paths)."""
    targets_map = {
        "linux_bpf":   BASE / "scripts/extracts/linux_bpf",
        "linux_kvm":   BASE / "scripts/extracts/linux_kvm",
        "linux_usb":   BASE / "scripts/extracts/linux_usb",
        "libxml2":     BASE / "scripts/extracts/libxml2",
        "poppler":     BASE / "scripts/extracts/poppler",
        "imagemagick": BASE / "scripts/extracts/imagemagick_712",
    }
    client, model = get_glm_client()

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
        findings.sort(key=lambda x: (
            0 if x.get("severity") == "CRITICAL" else 1,
            -x.get("confidence", 0)
        ))
        seen = set()
        for f in findings:
            key = (f.get("type","")[:20], f.get("source_file","")[:30])
            if key not in seen:
                seen.add(key)
                f["_target"] = target
                f["_extracts_dir"] = str(extracts_dir)
                candidates.append(f)
            if len([c for c in candidates if c.get("_target") == target]) >= 5:
                break

    print(f"🔍 二次验证: {len(candidates)} 个候选")
    results = []
    confirmed = []

    for i, cand in enumerate(candidates):
        target = cand.get("_target","?")
        extracts_dir = Path(cand.get("_extracts_dir", "."))
        src_file = cand.get("source_file","")
        extract_path = extracts_dir / src_file

        print(f"  [{i+1:2d}/{len(candidates)}] {target:<12} "
              f"{cand.get('type','?')[:28]:<30} {src_file[:33]:<35}", end=" ", flush=True)

        # Load extract with adjacent segments
        code = ""
        if extract_path.exists():
            parts = extract_path.stem.rsplit("_", 1)
            if len(parts) == 2:
                siblings = sorted(extract_path.parent.glob(f"{parts[0]}_*.c"))
                idx = siblings.index(extract_path) if extract_path in siblings else -1
                chunks = []
                if idx > 0:
                    chunks.append(f"// PREV\n{siblings[idx-1].read_text(errors='replace')[:2000]}")
                chunks.append(f"// CURRENT\n{extract_path.read_text(errors='replace')}")
                if 0 <= idx < len(siblings)-1:
                    chunks.append(f"// NEXT\n{siblings[idx+1].read_text(errors='replace')[:2000]}")
                code = "\n\n".join(chunks)[:8000]

        cand_with_code = dict(cand)
        if code:
            cand_with_code["_code"] = code  # verify_one will prefer exploit_path or source_file

        result = verify_one(cand, client, model)
        verdict = result.get("verdict","?")
        print(f"{verdict:<15} {result.get('reason','')[:60]}")

        result["_target"] = target
        result["_finding"] = cand
        results.append(result)
        if verdict == "CONFIRMED":
            confirmed.append((target, cand, result))
        time.sleep(15)

    out_file = BASE / "research/phase2_verification.jsonl"
    with open(out_file, "w") as f:
        for r in results:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")

    print(f"\n{'='*80}")
    print(f"✅ 验证完成: {len(results)} | CONFIRMED: {len(confirmed)}")
    print(f"📄 结果: {out_file}")
    for target, finding, result in confirmed:
        print(f"\n  [{target}] {finding.get('source_file')}")
        print(f"  {finding.get('type')} | {finding.get('severity')} | conf={finding.get('confidence')}")
        print(f"  {str(finding.get('description',''))[:120]}")
        print(f"  验证: {result.get('reason','')[:120]}")


# ── Entry point ───────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(description="Skeptical vulnerability verifier")
    parser.add_argument("--input-dir", type=Path, default=None,
                        help="Pipeline B: directory containing chunk_*.jsonl files")
    parser.add_argument("--output", type=Path, default=None,
                        help="Pipeline B: output JSONL path")
    parser.add_argument("--model", default="glm-4-flash",
                        help="Model to use for verification")
    parser.add_argument("--top-n", type=int, default=30,
                        help="Max candidates to verify (default 30)")
    parser.add_argument("--repo", type=Path, default=None,
                        help="Target repo root (for reading source files)")
    parser.add_argument("--phase2", action="store_true",
                        help="Run original Phase 2 research mode (local paths)")
    args = parser.parse_args()

    if args.phase2:
        run_phase2_mode()
    elif args.input_dir:
        if not args.output:
            parser.error("--output is required with --input-dir")
        run_pipeline_b_mode(
            input_dir=args.input_dir,
            output=args.output,
            model=args.model,
            top_n=args.top_n,
            repo_root=args.repo,
        )
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
