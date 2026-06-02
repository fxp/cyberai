#!/usr/bin/env python3
"""Generate coordinated-disclosure email drafts (J5) for survivor findings.

Reads /root/cyberai/research/validate_findings/survivors_*.jsonl
Writes /root/cyberai/research/validate_findings/drafts/<idx>_<target>_<ctx>.md
"""
from __future__ import annotations
import asyncio, json, os, re, sys, glob, time
from pathlib import Path

sys.path.insert(0, "/root/cyberai/src")
from cyberai.models.glm import GLMAdapter

VAL = Path("/root/cyberai/research/validate_findings")
DRAFTS = VAL / "drafts"
DRAFTS.mkdir(parents=True, exist_ok=True)

DRAFT_PROMPT = """Write a coordinated-disclosure email draft to send to the upstream maintainer
of an open-source project, based on this verified security finding. The
finding has passed three independent checks: GLM-5.1 self-verify (CONFIRMED
or PARTIAL), NVD cross-check (no published CVE match), and GLM-4-plus
code-grounded cross-check. So you may treat it as a credible candidate.

Project: {target}
Version scanned: latest stable as of 2026-05-04
Claimed severity: {severity}
Title: {title}
File: {file_context}, line ~{line_start}

ORIGINAL DESCRIPTION (from static-extract scanner):
{description}

GLM-5.1 ADVERSARIAL VERIFY REASONING:
{verify_reason}

GLM-4-PLUS CODE-GROUNDED CROSS-CHECK:
verdict={cross_verdict}, code_match={cross_code_match}, confidence={cross_conf}
{cross_reason}

Output a coordinated-disclosure email draft as plain markdown. Keep it
professional, technical, concise (under 500 words). No emojis. No marketing
phrasing. Use these exact section headings:

# Subject: <one-line subject — start with vendor short-name and severity>

## Summary
2-3 sentences on impact in plain terms.

## Affected versions
The version we tested. Note older/newer not yet verified but pattern likely
present.

## Vulnerability detail
Walk through what goes wrong, citing file path and approximate line numbers.
Include the relevant pattern (integer overflow, missing bounds check,
TOCTOU, etc.) and why it is exploitable.

## Reproduction sketch
Conceptual steps an attacker would take. Not a working exploit. Enough that
a maintainer can reproduce locally.

## Suggested mitigation
1-2 sentences pointing to the fix location and the kind of check needed
(e.g., "validate length <= INT_MAX before allocation", "use atomic compare-
swap", etc.).

## Disclosure timeline
We follow Google Project Zero 90-day coordinated disclosure. We will not
publish technical detail or proof-of-concept code before a patch is shipped.

— CyberAI research team (security@<placeholder>)

After the email body, output a SINGLE BLANK LINE then ONE final line:
RECOMMENDED_RECIPIENT: <best-guess upstream security email>"""


async def main():
    api_key = os.environ.get("GLM_API_KEY", "")
    assert api_key, "GLM_API_KEY missing"
    glm = GLMAdapter(model_name="glm-5.1", api_key=api_key)

    surv_files = sorted(glob.glob(str(VAL / "survivors_*.jsonl")))
    assert surv_files, f"no survivors file in {VAL}"
    src = surv_files[-1]
    findings = []
    with open(src) as fh:
        for line in fh:
            if not line.strip():
                continue
            try:
                findings.append(json.loads(line))
            except Exception:
                continue
    print(f"Generating drafts for {len(findings)} survivors from {src}")
    print()

    if not findings:
        print("(zero survivors — nothing to draft)")
        return

    total_cost = 0.0
    index_lines = ["# Disclosure draft index", "",
                   f"Generated {time.strftime('%Y-%m-%d %H:%M:%S UTC')} from {Path(src).name}",
                   "", "| # | Target | Severity | Conf | Title | Draft |", "|---|---|---|---:|---|---|"]

    for i, f in enumerate(findings, 1):
        cross = f.get("_cross_glm4plus", {})
        prompt = DRAFT_PROMPT.format(
            target=f["target"],
            severity=f["severity"],
            title=(f.get("title") or "").strip()[:200],
            file_context=f.get("file_context", "?"),
            line_start=f.get("line_start", "?"),
            description=(f.get("description") or "").strip()[:1500],
            verify_reason=(f.get("reasoning") or "")[:400],
            cross_verdict=cross.get("verdict", "?"),
            cross_code_match=cross.get("code_match", "?"),
            cross_conf=cross.get("confidence", "?"),
            cross_reason=(cross.get("reasoning") or "")[:400],
        )
        try:
            resp, usage = await glm.chat(
                [{"role": "user", "content": prompt}],
                timeout=300.0,
            )
            draft = resp.strip()
            total_cost += usage.cost_usd
            tok = f"in={usage.input_tokens} out={usage.output_tokens}"
        except Exception as e:
            draft = f"# DRAFT GENERATION FAILED\n\nError: {e}"
            tok = "ERROR"

        target = f["target"]
        ctx_safe = re.sub(r"[^a-zA-Z0-9_]", "_", f.get("file_context", ""))[:60]
        fname = f"{i:03d}_{target}_{ctx_safe}.md"
        out_path = DRAFTS / fname
        with open(out_path, "w") as fh:
            fh.write(draft + "\n\n---\n\n## Source finding\n\n```json\n")
            json.dump(f, fh, indent=2, ensure_ascii=False)
            fh.write("\n```\n")

        # Index row
        title_short = (f.get("title") or "").strip().replace("|", "\\|")[:80]
        index_lines.append(
            f"| {i} | {target} | {f['severity']} | {cross.get('confidence','?')} | "
            f"{title_short} | [`{fname}`](./{fname}) |"
        )
        print(f"[{i}/{len(findings)}] wrote {fname}  ({len(draft)} chars, {tok})", flush=True)

    (DRAFTS / "INDEX.md").write_text("\n".join(index_lines) + "\n")
    print()
    print(f"=== drafts done ===")
    print(f"  generated: {len(findings)}")
    print(f"  total cost: ${total_cost:.4f}")
    print(f"  index: {DRAFTS / 'INDEX.md'}")


if __name__ == "__main__":
    asyncio.run(main())
