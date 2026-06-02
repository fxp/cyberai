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

sys.path.insert(0, "/root/cyberai/src")
import httpx
from cyberai.models.glm import GLMAdapter

VF = Path("/root/cyberai/research/verify_findings")
EXTRACTS = Path("/root/cyberai/scripts/extracts")
OUT = Path("/root/cyberai/research/validate_findings")
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


def find_extract(target: str, file_context: str) -> Path | None:
    """Locate the source extract that was scanned for this finding."""
    target_dirs = sorted(p for p in EXTRACTS.iterdir() if p.is_dir() and p.name.lower().startswith(target.lower()))
    if not target_dirs:
        return None

    m = re.search(r"\[([^\]]+)\]", file_context)
    if not m:
        return None
    label = m.group(1)
    fn_name = label.split(":")[0].split()[0].strip()
    if not fn_name or len(fn_name) < 3:
        return None

    for tdir in target_dirs:
        for f in sorted(tdir.iterdir()):
            if not f.is_file():
                continue
            try:
                content = f.read_text(errors="ignore")
                if re.search(rf"\b{re.escape(fn_name)}\s*\(", content):
                    return f
            except Exception:
                continue
    return None


CROSS_PROMPT = """Re-judge a security finding given the actual source code that was scanned.

Project: {target}
Location: {file_context} (line ~{line_start})
Claimed severity: {severity}
Title: {title}

Original-scanner description:
{description}

ACTUAL SOURCE EXCERPT (this is what the model was given):
```c
{code}
```

Decide adversarially:
1. Does the finding's claim accurately describe the code shown? (code_match)
2. Is the bug pattern actually present and exploitable? (verdict)
3. How confident are you?

Respond with JSON ONLY (no markdown fence, no preamble):
{{"code_match":"yes"|"no"|"partial","verdict":"CONFIRMED"|"PARTIAL"|"FALSE_POSITIVE"|"NEEDS_MORE_CONTEXT","confidence":0.0-1.0,"reasoning":"<1-2 sentences>"}}
"""


async def cross_check(agent: GLMAdapter, finding: dict, code: str) -> dict:
    prompt = CROSS_PROMPT.format(
        target=finding["target"],
        file_context=finding["file_context"],
        line_start=finding.get("line_start", "?"),
        severity=finding["severity"],
        title=(finding.get("title") or "").strip()[:200],
        description=(finding.get("description") or "").strip()[:1500],
        code=code[:3500],
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

            # J2: locate source extract
            ext_path = find_extract(f["target"], f.get("file_context", ""))
            code = ""
            if ext_path:
                try:
                    code = ext_path.read_text(errors="ignore")
                except Exception:
                    pass

            # J3: glm-4-plus cross-check
            if code:
                cross = await cross_check(glm_plus, f, code)
            else:
                cross = {"verdict": "NO_EXTRACT", "_reason": "extract file not located"}

            result = {
                **f,
                "_nvd_match": nvd,
                "_nvd_ids": cve_ids,
                "_extract_used": str(ext_path) if ext_path else None,
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
