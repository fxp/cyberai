#!/usr/bin/env python3
"""Adversarial verifier for Pipeline A findings.

Iterates over all CRITICAL + HIGH findings across the 10 target JSONs,
sends each to glm-5.1 with an adversarial verifier prompt that grounds
on the description + file/line context, and writes verdicts to
verify_findings_<timestamp>.json on disk + uploads summary to OSS.

Run as a transient systemd service on the ECS:
  systemd-run --unit=cyberai-verify ... python3 /root/cyberai/scripts/verify_findings.py
"""
from __future__ import annotations
import os, sys, json, glob, time, asyncio
from pathlib import Path

sys.path.insert(0, "/root/cyberai/src")
from cyberai.models.glm import GLMAdapter

RESEARCH = Path("/root/cyberai/research")
OUT = RESEARCH / "verify_findings"
OUT.mkdir(parents=True, exist_ok=True)

VERIFY_PROMPT = """You are an adversarial security reviewer. A static-extract scanner flagged the
following potential vulnerability. Your job: decide if this is a REAL exploitable bug
or a false positive, based on the description and your knowledge of the codebase.

Project: {target}
File context: {file_context}
Line: {line_start}
Claimed severity: {severity}
Claimed confidence: {confidence}

TITLE: {title}

DESCRIPTION:
{description}

POC SKETCH:
{poc}

Reason carefully:
1. Does the described code path actually exist in this project at this location?
2. Is the bug pattern real, or is it a misread of correct code?
3. Is the precondition for triggering it realistic in deployed usage?
4. Is this already a known CVE you can name?

Respond with JSON ONLY (no markdown fences, no commentary):
{{
  "verdict": "CONFIRMED" | "PARTIAL" | "FALSE_POSITIVE" | "NEEDS_MORE_CONTEXT",
  "confidence": <float 0-1>,
  "reasoning": "<1-3 sentences why>",
  "exploitability": "exploitable" | "design_caveat" | "needs_specific_setup" | "not_exploitable",
  "known_cve": "<CVE-XXXX-XXXXX or 'none' or 'possibly:reason'>"
}}
"""

TARGETS = ["libpng","expat","curl","nginx","sqlite","openssl","zlib",
           "libxml2","libssh2","freetype"]

def collect_findings(min_severity=("CRITICAL","HIGH")):
    items = []
    for t in TARGETS:
        candidates = sorted(glob.glob(str(RESEARCH / t / "*results*.json")))
        if not candidates:
            print(f"  [skip] {t}: no results")
            continue
        with open(candidates[-1]) as f:
            d = json.load(f)
        results = d.get("results", d) if isinstance(d, dict) else d
        if not isinstance(results, list): results = [results]
        for r in results:
            if not isinstance(r, dict): continue
            file_context = r.get("file","?")
            for finding in r.get("findings", []):
                sev = (finding.get("severity") or "?").upper()
                if sev not in min_severity:
                    continue
                items.append({
                    "target": t,
                    "file_context": file_context,
                    "line_start": finding.get("line_start", "?"),
                    "severity": sev,
                    "confidence": finding.get("confidence", "?"),
                    "title": (finding.get("title") or "").strip()[:300],
                    "description": (finding.get("description") or "").strip()[:2500],
                    "poc": (finding.get("poc") or finding.get("proof_of_concept") or "")[:1500] if isinstance(finding.get("poc"), str) else "",
                })
    return items


async def verify_one(agent, finding, idx, total):
    prompt = VERIFY_PROMPT.format(**finding)
    t0 = time.time()
    try:
        resp, usage = await agent.chat(
            [{"role":"user","content":prompt}],
            timeout=300.0,
        )
    except Exception as e:
        elapsed = time.time() - t0
        print(f"  [{idx}/{total}] {finding['target']:<10} {finding['severity']:<8} ERROR {e!s:.80} ({elapsed:.0f}s)", flush=True)
        return {**finding, "verdict":"ERROR","_error": str(e), "_elapsed_s": round(elapsed,1)}

    elapsed = time.time() - t0
    raw = resp.strip()
    # strip markdown fences if present
    if raw.startswith("```"):
        lines = raw.splitlines()
        if len(lines) > 2:
            raw = "\n".join(lines[1:-1])

    parsed = None
    try:
        parsed = json.loads(raw)
    except Exception:
        # try to find JSON object in text
        import re
        m = re.search(r"\{[^{}]*(?:\{[^{}]*\}[^{}]*)*\}", raw, re.DOTALL)
        if m:
            try: parsed = json.loads(m.group(0))
            except Exception: pass

    if not parsed:
        verdict = "PARSE_ERROR"
        result = {**finding, "verdict": verdict, "_raw": raw[:400], "_elapsed_s": round(elapsed,1)}
    else:
        verdict = parsed.get("verdict", "?")
        result = {**finding, **parsed, "_elapsed_s": round(elapsed,1),
                  "_in_tokens": usage.input_tokens, "_out_tokens": usage.output_tokens,
                  "_cost_usd": round(usage.cost_usd, 5)}

    print(f"  [{idx}/{total}] {finding['target']:<10} {finding['severity']:<8} -> {verdict:<20} ({elapsed:.0f}s)", flush=True)
    return result


async def main():
    api_key = os.environ.get("GLM_API_KEY", "")
    assert api_key, "GLM_API_KEY missing"
    agent = GLMAdapter(model_name="glm-5.1", api_key=api_key)

    findings = collect_findings(("CRITICAL","HIGH"))
    print(f"Collected {len(findings)} CRITICAL+HIGH findings to verify")
    print(f"By severity: CRITICAL={sum(1 for f in findings if f['severity']=='CRITICAL')}, HIGH={sum(1 for f in findings if f['severity']=='HIGH')}")
    print()

    ts = time.strftime("%Y%m%d_%H%M%S")
    out_path = OUT / f"verify_{ts}.jsonl"
    summary_path = OUT / f"verify_{ts}_summary.json"
    progress_path = OUT / "verify_progress.json"

    results = []
    BATCH = 5  # 5 verifies in parallel
    for i in range(0, len(findings), BATCH):
        batch = findings[i:i+BATCH]
        coros = [verify_one(agent, f, i+j+1, len(findings)) for j, f in enumerate(batch)]
        batch_results = await asyncio.gather(*coros, return_exceptions=False)
        results.extend(batch_results)
        # append to file as we go
        with open(out_path, "a") as f:
            for r in batch_results:
                f.write(json.dumps(r, ensure_ascii=False) + "\n")
        # progress
        with open(progress_path, "w") as f:
            json.dump({"ts": ts, "done": len(results), "total": len(findings),
                       "current_target": batch[-1]["target"] if batch else None}, f)
        # small pacing
        await asyncio.sleep(2)

    # Summary
    sev_verdict = {}
    cost_total = 0.0
    for r in results:
        v = r.get("verdict", "?")
        s = r.get("severity", "?")
        sev_verdict.setdefault(s, {})[v] = sev_verdict.setdefault(s, {}).get(v, 0) + 1
        cost_total += r.get("_cost_usd", 0) or 0

    summary = {
        "timestamp": ts,
        "total_verified": len(results),
        "by_severity_verdict": sev_verdict,
        "total_cost_usd": round(cost_total, 4),
    }
    with open(summary_path, "w") as f:
        json.dump(summary, f, indent=2, ensure_ascii=False)

    print()
    print(f"=== verification complete ===")
    print(f"  results: {out_path}")
    print(f"  summary: {summary_path}")
    print(f"  total cost: ${cost_total:.4f}")
    for sev, vds in sev_verdict.items():
        print(f"  {sev}: {vds}")


if __name__ == "__main__":
    asyncio.run(main())
