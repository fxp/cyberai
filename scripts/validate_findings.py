#!/usr/bin/env python3
"""Validate (J1 NVD + J2 source grounding + J3 glm-4-plus cross-check)
the CONFIRMED+PARTIAL findings from the H verify pass.

Reads /root/cyberai/research/verify_findings/verify_*.jsonl.
Writes:
  /root/cyberai/research/validate_findings/validated_<ts>.jsonl   (all 70)
  /root/cyberai/research/validate_findings/survivors_<ts>.jsonl   (filter passed)
"""
from __future__ import annotations
import asyncio, json, os, re, sys, glob, time
from pathlib import Path

# Paths default to the ECS layout but can be overridden via env so the
# module imports cleanly on a workstation for testing.
CYBERAI_ROOT = Path(os.environ.get("CYBERAI_ROOT", "/root/cyberai"))
sys.path.insert(0, str(CYBERAI_ROOT / "src"))
import httpx
from cyberai.models.glm import GLMAdapter

VF = Path(os.environ.get("CYBERAI_VERIFY_DIR", str(CYBERAI_ROOT / "research/verify_findings")))
EXTRACTS = Path(os.environ.get("CYBERAI_EXTRACTS_DIR", str(CYBERAI_ROOT / "scripts/extracts")))
OUT = Path(os.environ.get("CYBERAI_VALIDATE_DIR", str(CYBERAI_ROOT / "research/validate_findings")))
if __name__ == "__main__":
    OUT.mkdir(parents=True, exist_ok=True)

NVD_BASE = "https://services.nvd.nist.gov/rest/json/cves/2.0"


async def nvd_lookup(client: httpx.AsyncClient, target: str, title: str) -> list[dict]:
    """Search NVD by keywords. Returns up to 3 matching CVE entries."""
    title_kw = re.sub(r"[^a-zA-Z0-9 ]", " ", title)
    title_kw = " ".join(title_kw.split()[:6])
    keyword = f"{target} {title_kw}".strip()[:128]
    try:
        r = await client.get(
            NVD_BASE,
            params={"keywordSearch": keyword, "resultsPerPage": 5},
            timeout=20.0,
        )
        if r.status_code == 200:
            data = r.json()
            cves = []
            for v in data.get("vulnerabilities", []):
                cve = v.get("cve", {})
                cve_id = cve.get("id")
                descs = cve.get("descriptions", [])
                desc = next((d["value"][:140] for d in descs if d.get("lang") == "en"), "")
                cves.append({"id": cve_id, "desc": desc})
            return cves[:3]
        return [{"_error": f"http {r.status_code}"}]
    except Exception as e:
        return [{"_error": str(e)[:100]}]


def find_extracts(target: str, file_context: str) -> tuple[Path | None, list[Path]]:
    """Locate the matching source extract + sibling extracts in the same target dir.

    Returns (primary_extract, siblings). The primary is the file whose body
    contains the function named in file_context. siblings are the other files
    under the same target extract directory — these typically contain
    load-time validators, init paths, and helper functions that establish
    the invariants the primary function relies on. Passing them to the
    cross-check model is what catches false positives where the bug is
    "patched" by a guard in a sibling function (which is the failure mode
    that refuted libpng, libxml2, and freetype top candidates on first
    grounding pass).
    """
    target_dirs = sorted(p for p in EXTRACTS.iterdir() if p.is_dir() and p.name.lower().startswith(target.lower()))
    if not target_dirs:
        return None, []

    m = re.search(r"\[([^\]]+)\]", file_context)
    fn_name = None
    if m:
        label = m.group(1)
        fn_name = label.split(":")[0].split()[0].strip()
        if not fn_name or len(fn_name) < 3:
            fn_name = None

    primary: Path | None = None
    all_files: list[Path] = []
    for tdir in target_dirs:
        for f in sorted(tdir.iterdir()):
            if not f.is_file():
                continue
            try:
                content = f.read_text(errors="ignore")
            except Exception:
                continue
            all_files.append(f)
            if primary is None and fn_name and re.search(rf"\b{re.escape(fn_name)}\s*\(", content):
                primary = f
    siblings = [f for f in all_files if f != primary]
    return primary, siblings


# Backwards-compat shim so callers / tests using the old name still work.
def find_extract(target: str, file_context: str) -> Path | None:
    primary, _ = find_extracts(target, file_context)
    return primary


def assemble_context(primary: Path | None, siblings: list[Path],
                     max_total_chars: int = 18000) -> tuple[str, list[str]]:
    """Build a concatenated source context with file separators.

    Primary file is shown in full first; sibling files are appended until
    the char budget is exhausted. Returns (context, included_filenames).
    """
    parts: list[str] = []
    included: list[str] = []
    used = 0

    def add(p: Path, label: str) -> bool:
        nonlocal used
        try:
            body = p.read_text(errors="ignore")
        except Exception:
            return False
        # Allow primary to take up to half the budget; siblings split the rest.
        header = f"\n/* ===== {label}: {p.name} ===== */\n"
        budget_left = max_total_chars - used - len(header)
        if budget_left <= 200:
            return False
        chunk = body if len(body) <= budget_left else body[:budget_left] + "\n/* [truncated] */\n"
        parts.append(header + chunk)
        used += len(header) + len(chunk)
        included.append(p.name)
        return True

    if primary:
        add(primary, "PRIMARY")
    for s in siblings:
        if used >= max_total_chars - 500:
            break
        add(s, "SIBLING")

    return "".join(parts), included


CROSS_PROMPT = """You are an adversarial reviewer re-judging a security finding. Your DEFAULT
position is FALSE_POSITIVE. You may only return CONFIRMED if you can rule out
every refutation below.

Project: {target}
Location: {file_context} (line ~{line_start})
Claimed severity: {severity}
Title: {title}

Original-scanner description:
{description}

ACTUAL SOURCE CONTEXT (PRIMARY file contains the flagged function;
SIBLING files are other extracts from the same target — they often contain
load-time validators, init guards, and helpers that establish invariants
the primary function relies on. Read them before judging):
{included_note}
```c
{code}
```

REFUTATION CHECKLIST — work through every item before deciding:
1. **Validator/guard in a sibling?** Search SIBLING blocks for any
   `*_validate`, `*_check`, `*_verify`, `init`, or `parse` function that runs
   BEFORE the primary function and bounds the values it reads. If found and
   sufficient → FALSE_POSITIVE.
2. **Build-config gated?** Is the vulnerable path behind `#ifdef`/`#if`
   conditionals that exclude it from production builds (e.g., debug-only,
   legacy fallback, platform-specific)? If yes → FALSE_POSITIVE.
3. **Attacker control?** Can attacker-controlled INPUT (file bytes, network
   bytes, env vars, args) actually drive the precondition the finding assumes,
   or does it require an already-corrupted internal state (which presupposes a
   separate primitive — circular)? If circular → FALSE_POSITIVE.
4. **Already patched in this code?** Does the visible code already contain
   the fix (saturating arithmetic, bounded loop, NULL check) that defeats the
   described primitive? If yes → FALSE_POSITIVE.
5. **By-design invariant?** Is the flagged cast/access actually documented
   intentional behavior (e.g., a tagged-union pattern, a data-structure
   contract enforced by sibling constructors)? Comments or sibling assignment
   sites are evidence. If yes → FALSE_POSITIVE.

Only if all five refutations fail: CONFIRMED. If you can partially refute but
some scenario remains, return PARTIAL with the surviving scenario stated.
If you cannot decide because critical sibling code is missing from the
context, return NEEDS_MORE_CONTEXT and name the function/file you needed.

Respond with JSON ONLY (no markdown fence, no preamble):
{{"code_match":"yes"|"no"|"partial","verdict":"CONFIRMED"|"PARTIAL"|"FALSE_POSITIVE"|"NEEDS_MORE_CONTEXT","confidence":0.0-1.0,"refutation_attempted":["1","2","3","4","5"],"surviving_scenario":"<empty if FALSE_POSITIVE>","missing_context":"<empty unless NEEDS_MORE_CONTEXT>","reasoning":"<2-4 sentences citing the specific sibling/line that refutes or confirms>"}}
"""


async def cross_check(agent: GLMAdapter, finding: dict, code: str,
                      included_files: list[str] | None = None) -> dict:
    included = included_files or []
    if included:
        note = f"Included {len(included)} file(s) in context: {', '.join(included)}."
    else:
        note = "Only the primary extract is included."
    # Allow up to ~18000 chars of code so multiple files fit; cross-check
    # cost is ~$0.01/call so the extra tokens are well worth it for refutation accuracy.
    prompt = CROSS_PROMPT.format(
        target=finding["target"],
        file_context=finding["file_context"],
        line_start=finding.get("line_start", "?"),
        severity=finding["severity"],
        title=(finding.get("title") or "").strip()[:200],
        description=(finding.get("description") or "").strip()[:1500],
        included_note=note,
        code=code[:18000],
    )
    try:
        resp, usage = await agent.chat(
            [{"role": "user", "content": prompt}],
            timeout=180.0,
        )
        raw = resp.strip()
        if raw.startswith("```"):
            lines = raw.splitlines()
            if len(lines) > 2:
                raw = "\n".join(lines[1:-1])
        try:
            d = json.loads(raw)
        except Exception:
            m = re.search(r"\{[^{}]*(?:\{[^{}]*\}[^{}]*)*\}", raw, re.DOTALL)
            if m:
                try:
                    d = json.loads(m.group(0))
                except Exception:
                    d = {"verdict": "PARSE_ERROR", "_raw": raw[:200]}
            else:
                d = {"verdict": "PARSE_ERROR", "_raw": raw[:200]}
        d["_cost"] = round(usage.cost_usd, 5)
        return d
    except Exception as e:
        return {"verdict": "ERROR", "_error": str(e)[:120]}


async def main():
    api_key = os.environ.get("GLM_API_KEY", "")
    assert api_key, "GLM_API_KEY missing"
    glm_plus = GLMAdapter(model_name="glm-4-plus", api_key=api_key)

    jsonls = sorted(glob.glob(str(VF / "verify_*.jsonl")))
    assert jsonls, f"no verify jsonl in {VF}"
    src = jsonls[-1]
    findings = []
    with open(src) as fh:
        for line in fh:
            if not line.strip():
                continue
            try:
                r = json.loads(line)
            except Exception:
                continue
            if r.get("verdict") in ("CONFIRMED", "PARTIAL"):
                findings.append(r)
    print(f"Loaded {len(findings)} CONFIRMED+PARTIAL from {src}")
    print()

    ts = time.strftime("%Y%m%d_%H%M%S")
    out_path = OUT / f"validated_{ts}.jsonl"

    async with httpx.AsyncClient() as client:
        for i, f in enumerate(findings, 1):
            label = f"{f['target']}/{f['severity']}"
            print(f"[{i:>2}/{len(findings)}] {label:<18} {f.get('title','')[:55]}", flush=True)

            # J1: NVD
            nvd = await nvd_lookup(client, f["target"], f.get("title", ""))
            cve_ids = [x.get("id") for x in nvd if x.get("id")]
            await asyncio.sleep(6.5)  # NVD: 5 req/30s without API key

            # J2: locate primary source extract + sibling extracts.
            # The siblings are the key methodology improvement: they often
            # contain the load-time validator or invariant-establishing init
            # path that refutes false-positive findings. See AGENTS.md
            # improvement #5 and the libpng/libxml2/freetype refutation
            # writeups under research/disclosures/.
            primary, siblings = find_extracts(f["target"], f.get("file_context", ""))
            code, included = assemble_context(primary, siblings)

            # J3: glm-4-plus cross-check with multi-file context + refutation prompt.
            if code:
                cross = await cross_check(glm_plus, f, code, included_files=included)
            else:
                cross = {"verdict": "NO_EXTRACT", "_reason": "extract file not located"}

            result = {
                **f,
                "_nvd_match": nvd,
                "_nvd_ids": cve_ids,
                "_extract_used": str(primary) if primary else None,
                "_siblings_used": [str(p) for p in siblings] if primary else [],
                "_cross_glm4plus": cross,
            }
            with open(out_path, "a") as fh:
                fh.write(json.dumps(result, ensure_ascii=False) + "\n")

            cv = cross.get("verdict", "?")
            cm = cross.get("code_match", "?")
            print(f"           NVD={'/'.join(cve_ids) or 'none':<24} cross={cv:<16} match={cm}", flush=True)

    # Filter survivors
    print()
    print("=== filtering ===")
    survivors = []
    nvd_skip = grounding_skip = cross_skip = 0
    with open(out_path) as fh:
        for line in fh:
            r = json.loads(line)
            if r.get("_nvd_ids"):
                nvd_skip += 1
                continue
            cross = r.get("_cross_glm4plus", {})
            v = cross.get("verdict")
            if v == "NO_EXTRACT":
                grounding_skip += 1
                continue
            if v in ("FALSE_POSITIVE", "NEEDS_MORE_CONTEXT", "ERROR", "PARSE_ERROR"):
                cross_skip += 1
                continue
            if cross.get("code_match") == "no":
                cross_skip += 1
                continue
            survivors.append(r)

    print(f"  filtered by NVD match:           {nvd_skip}")
    print(f"  filtered by no-extract found:    {grounding_skip}")
    print(f"  filtered by glm-4-plus cross:    {cross_skip}")
    print(f"  survivors (high-confidence):     {len(survivors)}")

    surv_path = OUT / f"survivors_{ts}.jsonl"
    with open(surv_path, "w") as fh:
        for s in survivors:
            fh.write(json.dumps(s, ensure_ascii=False) + "\n")
    print(f"  saved → {surv_path}")


if __name__ == "__main__":
    asyncio.run(main())
