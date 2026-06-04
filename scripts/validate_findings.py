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
# Optional: directory containing upstream source checkouts, one per target
# (e.g. UPSTREAM_DIR/libpng/{png.c,pngrutil.c,...}). When set, find_extracts()
# pulls the WHOLE referenced file from upstream as an additional sibling.
# This catches false positives where the load-time validator or invariant-
# establishing function was never extracted by Pipeline A (the libpng
# png_check_IHDR case is the canonical example).
UPSTREAM_DIR = Path(os.environ["CYBERAI_UPSTREAM_DIR"]) if os.environ.get("CYBERAI_UPSTREAM_DIR") else None
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


def find_extracts(target: str, file_context: str,
                  return_upstream: bool = False
) -> tuple[Path | None, list[Path]] | tuple[Path | None, list[Path], set[Path]]:
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

    # Upstream fallback (see comment on UPSTREAM_DIR above).
    upstream_added: set[Path] = set()
    if UPSTREAM_DIR:
        # Try to extract a .c filename from file_context. Accept either a
        # nested path (foo/bar/baz.c) or a bare basename.
        path_m = re.search(r"([\w\-+]+/[\w\-+/.]+\.c\b)", file_context)
        basename = None
        if path_m:
            basename = path_m.group(1).rsplit("/", 1)[-1]
        else:
            bare_m = re.search(r"\b([\w\-+]+\.c)\b", file_context)
            if bare_m:
                basename = bare_m.group(1)
        up_root = UPSTREAM_DIR / target.lower()
        candidates: list[Path] = []
        if basename and up_root.exists():
            # Prefer exact relative-path matches first, then recursive glob.
            if path_m:
                rel = path_m.group(1)
                for direct in (up_root / rel, up_root / rel.split("/", 1)[-1]):
                    if direct.is_file():
                        candidates.append(direct)
            # Recursive: pick the shortest path (most likely the "main" copy
            # — avoids fuzz/test mirrors of the same filename).
            globbed = sorted(up_root.rglob(basename), key=lambda p: (len(p.parts), str(p)))
            for g in globbed:
                # Skip obvious test/fuzz directories.
                if any(seg in {"test", "tests", "fuzz", "fuzzers", "demos", "examples"}
                       for seg in g.parts):
                    continue
                if g not in candidates:
                    candidates.append(g)
                    break  # one canonical hit is enough
        for cand in candidates:
            if not cand.is_file():
                continue
            # Promote the upstream file to PRIMARY: it holds the full
            # vulnerable function AND its load-time validator / init helpers
            # in the same translation unit. The original "primary" extract
            # (e.g. xpath_axis_A.c) is just a ~10KB slice — letting it stay
            # primary starves the budget for alphabetical neighbors.
            if primary is not None and primary not in all_files:
                all_files.append(primary)
            primary = cand
            upstream_added.add(cand)
            # Same-directory upstream siblings. Rank: filenames literally
            # named in file_context first, then prefix-match with primary,
            # then alphabetical. This is what makes png.c (the validator
            # next to pngrutil.c) reliably reach the prompt.
            fc_basenames = set(re.findall(r"\b[\w\-+]+\.c\b", file_context))
            cand_prefix = _name_prefix(cand.name)
            same_dir = [s for s in sorted(cand.parent.iterdir())
                        if s.is_file() and s.suffix == ".c" and s != cand]
            def _common_prefix_len(a: str, b: str) -> int:
                n = 0
                for ca, cb in zip(a, b):
                    if ca != cb:
                        break
                    n += 1
                return n
            cand_stem = cand.stem.lower()

            def _udir_rank(s: Path) -> tuple[int, int, str]:
                # bucket 0: filename literally cited in file_context
                if s.name in fc_basenames:
                    return (0, 0, s.name)
                # bucket 1: exact name-prefix match (xpath_axis ~ xpath_nodeset)
                if _name_prefix(s.name) == cand_prefix:
                    return (1, 0, s.name)
                # bucket 2: share a 3+ char common stem prefix with primary
                # (libpng case: png.c shares "png" with pngrutil.c, beats
                # alphabetically-earlier-but-unrelated example.c)
                cpl = _common_prefix_len(s.stem.lower(), cand_stem)
                if cpl >= 3:
                    return (2, -cpl, s.name)  # negate so longer overlap wins
                return (3, 0, s.name)
            for s in sorted(same_dir, key=_udir_rank):
                if s not in all_files:
                    all_files.append(s)
                    upstream_added.add(s)
            break

    siblings = [f for f in all_files if f != primary]
    if return_upstream:
        return primary, siblings, upstream_added
    return primary, siblings


# Backwards-compat shim so callers / tests using the old name still work.
def find_extract(target: str, file_context: str) -> Path | None:
    primary, _ = find_extracts(target, file_context)
    return primary


def _name_prefix(name: str) -> str:
    """Extract the subsystem prefix of an extract filename.

    Conventions seen in `scripts/extracts/<target>/`:
      xpath_axis_A.c       → 'xpath'
      pngrutil_G.c         → 'pngrutil'
      parser_name_A.c      → 'parser'
      cffdecode_curve_A.c  → 'cffdecode'
    Strategy: take everything up to the first '_' OR the first capital-letter
    suffix marker (`_A`, `_B`, etc.). Files without underscores fall back to
    the basename minus extension.
    """
    stem = name.rsplit(".", 1)[0]
    if "_" in stem:
        # 'xpath_axis_A' → 'xpath' (first token before first underscore)
        return stem.split("_", 1)[0].lower()
    return stem.lower()


def _rank_siblings(primary: Path, siblings: list[Path],
                   upstream_set: set[Path] | None = None) -> list[Path]:
    """Rank siblings: upstream files first, then by subsystem-prefix overlap
    with primary, then by function-name overlap, then alphabetically.

    Upstream files are the highest priority because (a) they're the deepest
    available context — actual project source rather than scanner extracts,
    and (b) the upstream fallback is only triggered when something is
    missing from the extract pool, so they're known to be relevant. Without
    this boost, the budget fills with prefix-matched extracts first (e.g.
    `pngrutil_A..F.c`) and the upstream validator file (`png.c`) gets
    dropped — exactly the libpng failure mode the upstream fallback exists
    to fix.
    """
    if not primary:
        return siblings
    upstream_set = upstream_set or set()
    primary_prefix = _name_prefix(primary.name)
    try:
        primary_text = primary.read_text(errors="ignore")
    except Exception:
        primary_text = ""
    fn_defs = set(re.findall(r"\b([a-zA-Z_][a-zA-Z0-9_]{3,})\s*\(", primary_text))

    primary_stem = primary.stem.lower()
    # Pattern for validator/checker/init/parser/open function definitions —
    # the things sibling files use to enforce invariants the primary relies
    # on. Counting matches in a sibling's head gives a content-based proxy
    # for "is this likely to refute the finding?". Empirically: libpng's
    # png.c has 8 such functions including png_check_IHDR, while siblings
    # pngrutil.c / pngread.c / pngrio.c have 0-1. Naive prefix-ranking
    # would put pngrtran ahead of png.c because it shares "pngr" with the
    # primary pngrutil.c — content-counting flips that the right way.
    _VALIDATOR_RE = re.compile(
        r"^[a-z][a-z_0-9]*(?:_check_|_validate|_init_|_open_|_verify_|_sanity_)[a-z_0-9]*\s*\(",
        re.MULTILINE | re.IGNORECASE,
    )

    def _common_prefix_len(a: str, b: str) -> int:
        n = 0
        for ca, cb in zip(a, b):
            if ca != cb:
                break
            n += 1
        return n

    def _validator_count(p: Path) -> int:
        try:
            head = p.read_text(errors="ignore")[:80000]
        except Exception:
            return 0
        return len(_VALIDATOR_RE.findall(head))

    def score(s: Path) -> tuple[int, int, int, str]:
        # Lower tuple = higher priority (sorted ascending).
        # Bucket 0: upstream (the deepest context we can offer).
        if s in upstream_set:
            cpl = _common_prefix_len(s.stem.lower(), primary_stem)
            vc = _validator_count(s)
            # Sub-rank within upstream: more validator-style functions first,
            # then longer shared stem prefix, then alphabetical.
            return (0, -vc, -cpl, s.name.lower())
        prefix_match = _name_prefix(s.name) == primary_prefix
        if prefix_match:
            return (1, 0, 0, s.name.lower())
        try:
            body = s.read_text(errors="ignore")
        except Exception:
            body = ""
        refs_primary = any(fn in body for fn in fn_defs) if fn_defs else False
        return (2 if refs_primary else 3, 0, 0, s.name.lower())

    return sorted(siblings, key=score)


def _slice_file(path: Path, max_chars: int, line_start=None) -> str:
    """Read a file and return up to max_chars of content.

    If the file fits in budget, return the whole body. Otherwise return a
    head + line-window: the first ~35% of the budget from the start of the
    file (where declarations, type definitions, and early helpers — often
    validators — live), then a window of ~60% centred on `line_start`
    (where the vulnerable function lives), with a truncation marker between.

    This handles the canonical case from refuted findings: in libxml2's
    xpath.c the validator (xmlXPathNodeSetDupNs at line 2604) and the vulnerable
    function (xmlXPathNextAncestor at line 6425) are 14000+ lines apart in a
    single 338KB file. A flat head-only slice misses one of them; this slicer
    gets both.
    """
    try:
        body = path.read_text(errors="ignore")
    except Exception:
        return ""
    if len(body) <= max_chars:
        return body
    try:
        ls = int(line_start) if line_start not in (None, "", "?") else None
    except (TypeError, ValueError):
        ls = None
    if not ls:
        return body[:max_chars] + "\n/* [tail truncated] */\n"
    # Byte offset of the start of line ls.
    lines = body.splitlines(keepends=True)
    offset = sum(len(l) for l in lines[: max(0, ls - 1)])
    # 50% head, ~50% window: enough head to catch in-file helpers like
    # xmlXPathNodeSetDupNs (line 2604 in 12K-line xpath.c needs ~50% of a
    # 200KB cap), enough window to capture the vulnerable function body.
    head_size = int(max_chars * 0.5)
    win_size = max_chars - head_size - 80  # 80 chars for markers
    head = body[:head_size]
    win_start = max(0, offset - win_size // 2)
    win_end = min(len(body), win_start + win_size)
    if win_start < head_size:
        # Window overlaps head — just give head a bit more and skip slicing.
        return body[: head_size + win_size] + "\n/* [tail truncated] */\n"
    skipped_lines = body[head_size:win_start].count("\n")
    return (
        head
        + f"\n/* [skipped ~{skipped_lines} lines] */\n"
        + body[win_start:win_end]
        + ("\n/* [tail truncated] */\n" if win_end < len(body) else "")
    )


def assemble_context(primary: Path | None, siblings: list[Path],
                     max_total_chars: int = 300000,
                     upstream_set: set[Path] | None = None,
                     line_start=None
                     ) -> tuple[str, list[str]]:
    """Build a concatenated source context with file separators.

    Primary file is shown in full first; sibling files are ranked by
    subsystem-prefix overlap + cross-reference to primary, then appended
    until the char budget is exhausted. Returns (context, included_filenames).

    Budget is generous (60KB) because glm-4-plus has 128K context window and
    sibling files are where load-time validators / init invariants live —
    starving them is the same failure mode that produced 3/3 false positives
    in the original 2026-05-04 cycle. Cost is ~$0.02/J3-call at this budget,
    well within the $1-per-cycle envelope.
    """
    parts: list[str] = []
    included: list[str] = []
    used = 0
    ranked = _rank_siblings(primary, siblings, upstream_set) if primary else siblings

    # Per-file caps. Primary gets up to 200KB so both an early validator
    # (e.g. xmlXPathNodeSetDupNs at xpath.c:2604) and the vulnerable function
    # (e.g. xmlXPathNextAncestor at xpath.c:6988) both fit. First sibling gets
    # 80KB so its head reaches a load-time validator typically at line
    # 1500-2500 (e.g. png_check_IHDR in png.c at line 1933).
    primary_cap = min(max_total_chars * 7 // 10, 200000)
    sibling_caps = [80000, 40000, 20000, 10000, 5000]

    def add(p: Path, label: str, cap: int, use_line_start) -> bool:
        nonlocal used
        header = f"\n/* ===== {label}: {p.name} ===== */\n"
        budget_left = max_total_chars - used - len(header)
        if budget_left <= 500:
            return False
        chunk = _slice_file(p, min(cap, budget_left), use_line_start)
        if not chunk:
            return False
        parts.append(header + chunk)
        used += len(header) + len(chunk)
        included.append(p.name)
        return True

    if primary:
        add(primary, "PRIMARY", primary_cap, line_start)
    for i, s in enumerate(ranked):
        if used >= max_total_chars - 500:
            break
        cap = sibling_caps[i] if i < len(sibling_caps) else 5000
        # No line_start for siblings — we don't know where the validator is
        # inside them. Slicer falls back to head-only.
        add(s, "SIBLING", cap, None)

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
    # 300KB code budget matches assemble_context() default; glm-4-plus has
    # 128K-token context (~500KB raw text at C-code density). Code arrives
    # pre-sliced (head + line-window) so the validator and the vulnerable
    # function both make it in.
    prompt = CROSS_PROMPT.format(
        target=finding["target"],
        file_context=finding["file_context"],
        line_start=finding.get("line_start", "?"),
        severity=finding["severity"],
        title=(finding.get("title") or "").strip()[:200],
        description=(finding.get("description") or "").strip()[:1500],
        included_note=note,
        code=code[:300000],
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
            primary, siblings, upstream_set = find_extracts(
                f["target"], f.get("file_context", ""), return_upstream=True)
            code, included = assemble_context(
                primary, siblings, upstream_set=upstream_set,
                line_start=f.get("line_start"))

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
