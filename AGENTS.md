# Operations runbook for AI agents

This file is the canonical reference for any AI agent (Claude Code, Cursor,
Gemini CLI, Codex, Hermes, …) operating this repository. Read it before
running anything. It documents the pipeline, infrastructure, scripts,
common operations, and known pitfalls.

If you are a human, you can read it too. It's just denser than a typical
README.

---

## TL;DR

CyberAI is a research project that uses LLMs to find real vulnerabilities
in production C/C++ open-source libraries. The pipeline is:

```
A. Pipeline A scan       scripts/scan_<target>_*.py        glm-5.1
B. Pipeline B scan       .github/workflows/pipeline_b.yml  glm-5.1
C. Run all targets       scripts/run_daily_scans.sh        glm-5.1, ~20h
D. Verify chain          .github/workflows/verify_chain.yml glm-5.1
H. Adversarial verify    scripts/verify_findings.py        glm-5.1
J1. NVD cross-ref        scripts/validate_findings.py      no LLM
J2. Source grounding     scripts/validate_findings.py      no LLM
J3. glm-4-plus check     scripts/validate_findings.py      glm-4-plus
J5. Disclosure drafts    scripts/generate_drafts.py        glm-5.1
```

**Use BigModel GLM models for all analysis.** Do not use Claude / GPT
for verification or drafting; the user is paying for GLM tokens via the
`GLM_API_KEY` set on the ECS and in GH Secrets.

---

## Repository layout

```
cyberai/
├── AGENTS.md                   ← you are here
├── docs/                       ← public site (github.com/fxp/cyberai → Pages)
│   ├── index.html              ← landing
│   └── architecture.html       ← system overview
├── src/cyberai/                ← Python package (model adapters, infra)
│   ├── models/glm.py           ← GLMAdapter, default model_name="glm-5.1"
│   └── infra/eci.py            ← Aliyun ECI launcher (stub, not used)
├── scripts/                    ← all the runnable Python + shell
│   ├── scan_<target>_t1.py     ← Pipeline A scanners (per target)
│   ├── scan_libpng_v3.py       ← libpng v3 scanner
│   ├── run_daily_scans.sh      ← runs all 10 targets sequentially (~20h)
│   ├── pipeline_b_plan.py      ← Pipeline B file selection
│   ├── pipeline_b_detect.py    ← Pipeline B agentic detection
│   ├── pipeline_b_chain.py     ← Pipeline B kill-chain builder
│   ├── deep_verify_chain.py    ← Pipeline B chain verifier
│   ├── verify_findings.py      ← H. adversarial verifier
│   ├── validate_findings.py    ← J1+J2+J3 validator
│   ├── generate_drafts.py      ← J5 disclosure-draft generator
│   └── extracts/<target>_v*/   ← pre-cut C function bodies for scanners
├── .github/workflows/
│   ├── pipeline_a.yml          ← Pipeline A on a single target
│   ├── pipeline_b.yml          ← Pipeline B agentic scan (any GitHub repo)
│   ├── verify_chain.yml        ← Deep verify kill chains
│   └── notify_slack.yml        ← Generic Slack post via SLACK_WEBHOOK_URL
├── deploy/
│   ├── setup_ecs.sh            ← initial ECS bootstrap
│   └── systemd/
│       ├── cyberai-daily-scan.service
│       └── cyberai-daily-scan.timer
├── terraform/                  ← OSS bucket + RAM user IaC (no ECS)
├── research/
│   ├── <target>/               ← per-target Pipeline A outputs (gitignored *.jsonl)
│   ├── verify_findings/        ← H output (jsonl + summary)
│   ├── validate_findings/      ← J1+J2+J3 output + drafts/
│   ├── scan_logs/              ← daily scan log files
│   ├── disclosures/            ← human-written disclosure docs (PRIVATE - some gitignored)
│   └── scan-2026-05-04/        ← consolidated per-run report directory
└── pyproject.toml              ← Python package, requires-python >=3.11
```

---

## Infrastructure

### ECS — `cyberai-main` (us-west-1)

```
InstanceId: i-rj9iivpdwq8g59aa161m
Public IP:  47.251.245.235
Private IP: 172.16.2.231 (cyberai-vpc / cyberai-vsw-a)
Spec:       ecs.t6-c1m4.large (2 vCPU, 8GB)
Disk:       40GB cloud_essd
OS:         Ubuntu 22.04 (latest 2026-04 build)
TZ:         UTC
KeyPair:    cyberai-key (ed25519, fingerprint f9046668653f85b848dff597db2577)
ChargeType: PrePaid 1 month, expires 2026-06-03 (auto-renew OFF)
Cost:       ~420 CNY/月 (instance + disk + 5Mbps PayByBandwidth public IP)
```

**Path layout on ECS:**
```
/root/cyberai/                    ← clone of github.com/fxp/cyberai (HEAD on main)
/root/cyberai/.venv/              ← uv-managed Python 3.12 virtualenv (editable install)
/root/cyberai/.env                ← perms 600, contains GLM_API_KEY + ALIYUN_*
/root/cyberai/research/           ← all scan outputs land here
/var/log/cyberai-daily-scan.log   ← systemd-managed daily scan log
/var/log/cyberai-verify.log       ← H runs
/var/log/cyberai-validate.log     ← J1+J2+J3 runs
/var/log/cyberai-drafts.log       ← J5 runs
```

**SSH access** uses `ssh-ed25519` key. The project owner's local key has a
passphrase, so `ssh root@47.251.245.235` requires either `ssh-add` first or
falls back to `aliyun ecs RunCommand` (recommended for agents — no
passphrase needed, runs through TLS-encrypted Aliyun API).

### OSS bucket — `cyberai-scan-results-us1` (us-west-1)

```
oss://cyberai-scan-results-us1/scans/<DATE>/
├── index.json                              ← per-target severity stats
├── <target>.json                           ← raw Pipeline A output
├── verify/verify_<TS>.jsonl                ← H per-finding verdicts (full)
├── verify/verify_<TS>_summary.json         ← H aggregate stats
├── validate/validated_<TS>.jsonl           ← J1+J2+J3 annotated
├── validate/survivors_<TS>.jsonl           ← J1+J2+J3 survivors only
└── drafts/<NNN>_<target>_<ctx>.md          ← J5 disclosure drafts
```

OSS access creds are in the local `.env` and on the ECS `.env`:
```
ALIYUN_ACCESS_KEY_ID=<...>
ALIYUN_ACCESS_KEY_SECRET=<...>
ALIYUN_REGION=us-west-1
ALIYUN_OSS_BUCKET=cyberai-scan-results-us1
ALIYUN_OSS_ENDPOINT=https://oss-us-west-1.aliyuncs.com
```

### GitHub Actions runners (free tier)

Workflows live in `.github/workflows/`. All runs accessed via `gh`:

```
gh run list -R fxp/cyberai --workflow=pipeline_b.yml --limit 5
gh run view <ID> -R fxp/cyberai
gh run watch <ID> -R fxp/cyberai --exit-status --interval 30
```

### GH Secrets configured (verified)

```
ALIYUN_ACCESS_KEY_ID         (RAM user 'cyberai-github-actions')
ALIYUN_ACCESS_KEY_SECRET
ALIYUN_OSS_BUCKET            (= cyberai-scan-results-us1)
GLM_API_KEY                  (BigModel)
SLACK_WEBHOOK_URL            (used by pipeline_b auto-dispatch + notify_slack.yml)
```

`ANTHROPIC_API_KEY` is referenced in some scripts but is **not required** —
the user prefers GLM-only operation.

---

## Pipeline stages

### A. Pipeline A — static-extract scanner

**Per target.** Each `scan_<target>_t1.py` script:
- Loads pre-cut C function extracts from `scripts/extracts/<target>_*/`
- Calls glm-5.1 per extract with the CTF prompt strategy
- Emits per-finding JSON with severity + line + description + (sometimes) PoC

Run individually:
```bash
ssh-or-RunCommand: cd /root/cyberai && . .venv/bin/activate && set -a && . .env && set +a
python scripts/scan_libpng_v3.py --timeout 240 --delay 20
```

Output: `/root/cyberai/research/<target>/t1_v3_results.json` (or `t1_glm5_results.json`).

### C. run_daily_scans.sh — orchestrate all 10 targets

Runs all scan_*_t1 / scan_*_v3 sequentially. **Wall time ~20h.** Triggered
automatically via the systemd timer at 00:05 UTC daily, OR manually:

```bash
systemd-run --unit=cyberai-once \
  --working-directory=/root/cyberai \
  --property=EnvironmentFile=/root/cyberai/.env \
  --property=Environment="PATH=/root/cyberai/.venv/bin:/root/.local/bin:/usr/bin:/bin" \
  --property=StandardOutput=append:/var/log/cyberai-once.log \
  --property=StandardError=append:/var/log/cyberai-once.log \
  --property=TimeoutStartSec=24h \
  /bin/bash /root/cyberai/scripts/run_daily_scans.sh --all
```

Pass `--libpng`, `--expat`, etc. for a single target.

**Per-call timeout floor:** 240s. Smaller values cause spurious timeouts
on glm-5.1 reasoning paths. Bump higher (360-480s) if you see >40%
timeout rate.

### B. Pipeline B — agentic scanner via GitHub Actions

VIDOC-style: model has tool access to read any file in the cloned repo.
Output is signals + chains. Triggered manually:

```bash
gh workflow run pipeline_b.yml -R fxp/cyberai --ref main \
  -f target_repo=https://github.com/<vendor>/<repo> \
  -f target_ref=v1.2.3 \
  -f model=glm-5.1 \
  -f max_files=30
```

Available `model` choices (in `.github/workflows/pipeline_b.yml`):
`glm-5.1` (default) | `glm-4-plus` | `glm-4-flash` | `glm-z1-flash` | `claude-opus-4-6`

When chains are found, Pipeline B auto-dispatches `verify_chain.yml`
inheriting the parent model (with fallback to glm-5.1 for fast/cheap
detector models).

**Empirical observation (2026-05-20):** the two completed Pipeline B runs
(25355846497, 25355847613) targeted ImageMagick and produced 4 candidates
total — all correctly refuted by deep_verify with sharp reasoning ("#else
fallback is dead code on production builds", "attacker cannot control
OpenMP thread IDs", "only compiles when X and Y both undefined"). Pipeline
B's deep_verify methodology is *more conservative and correct* than
Pipeline A's H verifier — its refutation discipline (reachability,
build-config gating, attacker control) is what the new
`verify_findings.py` and `validate_findings.py` prompts borrow from.
Pipeline B has zero false positives so far, at the cost of high
dismissal rate.

### H. Adversarial verify — `scripts/verify_findings.py`

Re-judges all CRITICAL+HIGH findings produced by Pipeline A. For each
finding, asks glm-5.1: "given this title + description + line, is it real
or a false positive?" Outputs:

```
research/verify_findings/verify_<TS>.jsonl       per-finding verdict
research/verify_findings/verify_<TS>_summary.json aggregate stats
research/verify_findings/verify_progress.json    live progress
```

Verdict values: `CONFIRMED` | `PARTIAL` | `FALSE_POSITIVE` | `NEEDS_MORE_CONTEXT` | `ERROR` | `PARSE_ERROR`.

Run as transient systemd unit on ECS:
```bash
systemd-run --unit=cyberai-verify \
  --working-directory=/root/cyberai \
  --property=EnvironmentFile=/root/cyberai/.env \
  --property=Environment="PATH=/root/cyberai/.venv/bin:/root/.local/bin:/usr/bin:/bin" \
  --property=StandardOutput=append:/var/log/cyberai-verify.log \
  --property=StandardError=append:/var/log/cyberai-verify.log \
  --property=TimeoutStartSec=4h \
  /root/cyberai/.venv/bin/python /root/cyberai/scripts/verify_findings.py
```

**Wall time:** ~3-4h for ~240 findings, 5-wide async. **Cost:** ~$0.45 in
glm-5.1 tokens. **Known issue:** ~25-30% of findings hit a 300s GLM API
timeout and end up as `ERROR`. Bump the per-call timeout in the script
or accept the lossy result.

### J1+J2+J3. Validate — `scripts/validate_findings.py`

Filters CONFIRMED+PARTIAL findings to a high-confidence subset.

- **J1 NVD cross-ref**: HTTPS to public NVD JSON 2.0 API with target +
  title keywords. No LLM. Rate-limited at 5 req / 30s without API key,
  so add 6.5s delay per call. Drops findings whose description matches a
  published CVE.
- **J2 source-extract grounding**: Tries to locate the actual extract
  file scanned (under `scripts/extracts/<target>_*/`) by parsing the
  `file_context` field for a function name and grep-matching extract
  files. Drops `NO_EXTRACT` (model hallucinated a non-extract location).
- **J3 glm-4-plus cross-check**: Sends the actual code excerpt + the
  finding to **glm-4-plus** (different from glm-5.1 used in H, gives
  cross-model signal while staying in BigModel) and asks for verdict +
  code_match boolean.

Output:
```
research/validate_findings/validated_<TS>.jsonl   all 70 with annotations
research/validate_findings/survivors_<TS>.jsonl   high-confidence subset
```

Run:
```bash
systemd-run --unit=cyberai-validate \
  --working-directory=/root/cyberai \
  --property=EnvironmentFile=/root/cyberai/.env \
  --property=Environment="PATH=/root/cyberai/.venv/bin:/root/.local/bin:/usr/bin:/bin" \
  --property=StandardOutput=append:/var/log/cyberai-validate.log \
  --property=StandardError=append:/var/log/cyberai-validate.log \
  --property=TimeoutStartSec=2h \
  /root/cyberai/.venv/bin/python /root/cyberai/scripts/validate_findings.py
```

**Wall time:** ~17 min for 70 findings (NVD rate limit dominates).
**Cost:** ~$1.5 in glm-4-plus tokens.

### J5. Disclosure drafts — `scripts/generate_drafts.py`

For each survivor in `survivors_<TS>.jsonl`, asks glm-5.1 to write a
coordinated-disclosure email draft. Output:

```
research/validate_findings/drafts/<NNN>_<target>_<ctx>.md
research/validate_findings/drafts/INDEX.md
```

Each draft has subject + summary + affected versions + vulnerability
detail + reproduction sketch + suggested mitigation + 90-day disclosure
clause + recommended recipient.

**Drafts are GLM boilerplate.** A human must:
1. Verify the file path + line numbers match upstream HEAD.
2. Check `git blame` / fix history for whether the issue is already patched.
3. Build a real PoC + test under ASAN before sending to maintainer.

---

## Common operations

### Fire a fresh full pipeline run (A → H → J1+J2+J3 → J5)

```bash
# 1. Trigger Pipeline A on ECS (or wait for daily timer at 00:05 UTC)
aliyun ecs RunCommand --RegionId us-west-1 --InstanceId.1 i-rj9iivpdwq8g59aa161m \
  --Type RunShellScript --Timeout 60 --CommandContent '...systemd-run with run_daily_scans.sh --all...'

# 2. After ~20h wall time, fire H verifier
... systemd-run with verify_findings.py ...

# 3. After H, fire validate
... systemd-run with validate_findings.py ...

# 4. After validate, fire drafts
... systemd-run with generate_drafts.py ...

# 5. Pull results to OSS for archival, then to local for git commit
```

The Slack notify workflow at the end:
```bash
gh workflow run notify_slack.yml -R fxp/cyberai --ref main \
  -f title="..." -f color="good" -f summary="<markdown body>"
```

### Pull ECS outputs to OSS, then to local

OSS upload (run on ECS):
```python
import oss2, os
auth = oss2.Auth(os.environ["ALIYUN_ACCESS_KEY_ID"], os.environ["ALIYUN_ACCESS_KEY_SECRET"])
bucket = oss2.Bucket(auth, os.environ["ALIYUN_OSS_ENDPOINT"], os.environ["ALIYUN_OSS_BUCKET"])
bucket.put_object_from_file(f"scans/{date}/<key>", "/root/cyberai/research/<...>")
```

Local pull (from your machine):
```bash
aliyun oss cp oss://cyberai-scan-results-us1/scans/<DATE>/<file> ./<file> \
  --access-key-id $ALIYUN_ACCESS_KEY_ID \
  --access-key-secret $ALIYUN_ACCESS_KEY_SECRET \
  -e oss-us-west-1.aliyuncs.com -f
```

### Tear down the ECS

When you don't need it (saves ~14 CNY/day):
```bash
aliyun ecs DeleteInstance --InstanceId i-rj9iivpdwq8g59aa161m --Force true
```

VPC + Security Group + KeyPair persist. To rebuild:
```bash
aliyun ecs RunInstances --RegionId us-west-1 \
  --InstanceType ecs.t6-c1m4.large \
  --ImageId ubuntu_22_04_x64_20G_alibase_20260413.vhd \
  --SystemDisk.Category cloud_essd --SystemDisk.Size 40 \
  --VSwitchId vsw-rj9x86il8m97guvo38jk9 \
  --SecurityGroupId sg-rj99e1ymp8yqq6wj8ii1 \
  --InstanceName cyberai-main --HostName cyberai \
  --KeyPairName cyberai-key \
  --InstanceChargeType PrePaid --PeriodUnit Month --Period 1 \
  --InternetMaxBandwidthOut 5 --InternetChargeType PayByBandwidth \
  --IoOptimized optimized --Amount 1
```

After re-up, run `deploy/setup_ecs.sh` + `setup-script.sh` from `~/cyberai-work` history (or follow the bootstrap commit log) to re-install Docker + Python 3.12 + clone repo + `.env`.

### Daily timer schedule

```bash
# enabled by default — runs at 00:05 UTC = 08:05 BJT
systemctl status cyberai-daily-scan.timer
systemctl list-timers cyberai-daily-scan.timer

# disable / enable
systemctl disable --now cyberai-daily-scan.timer
systemctl enable --now cyberai-daily-scan.timer
```

---

## Known issues / gotchas (READ BEFORE RUNNING)

### iCloud git is unreliable
The original repo lives at
`/Users/xiaopingfeng/Library/Mobile Documents/iCloud~md~obsidian/Documents/Projects/CyberAI/cyberai/`.
iCloud Drive interferes with git index lookups — `git status` / `git
commit` regularly hang for minutes or indefinitely. **Do not** do git
operations on this path.

Use `~/cyberai-work/` instead, which is a clean local clone of
`github.com/fxp/cyberai`. All git writes happen here. To pull updates
from iCloud working tree to git, do file-level `cp`.

### Aliyun RunCommand quirks

- Shell is `/bin/sh` (dash on Ubuntu). `source` does not exist; use `.`
  (POSIX). `set -a; . .env; set +a` works; `source .env` does not.
- For complex scripts, wrap in `bash -c "$cmd"` or `bash <<'EOF' ... EOF`.
- Output is base64-encoded by Aliyun **with a 24KB cap**. Shell stdout
  exceeding 24KB is truncated. For larger outputs (whole JSON files),
  **upload to OSS** and pull from there, don't try to base64 over
  RunCommand.
- When working with multiline Python scripts, **upload via base64**:
  ```bash
  B64=$(base64 -i /tmp/script.py | tr -d '\n')
  aliyun ecs RunCommand --CommandContent "echo '$B64' | base64 -d > /tmp/script.py && cd /root/cyberai && . .venv/bin/activate && set -a && . /root/cyberai/.env && set +a && python3 /tmp/script.py"
  ```
  This avoids escaping hell with nested quotes and `$` substitutions.

### zsh array indexing

The default macOS shell is zsh. Arrays are 1-indexed in zsh; bash is
0-indexed. If your monitor script uses `${arr[0]}` and is run by zsh, you
silently miss the first element. Always `bash <<'EOF' ... EOF` your
monitor command bodies, or use `for elem in "${arr[@]}"`.

### Python script subprocess shells use `dash`

`subprocess.check_output(["bash", "-c", ...])` on Ubuntu means bash,
**but** if you call without `bash -c`, Python's default shell on
Ubuntu is `/bin/sh` = dash. `source` and `[[ ]]` don't exist in dash.
Use explicit `bash` invocation.

### glm-5.1 timeout tuning

The default `--timeout 90` (or 120, 150) in older `run_daily_scans.sh`
is too short for glm-5.1 reasoning. Bump to **240+ for normal scan,
360-480 for complex segments**. The `glm.py` `chat()` method has its own
default timeout (300s) that gets overridden by callers; check.

### NVD rate limit

Public NVD API: **5 requests / 30 seconds without an API key.** Add
`asyncio.sleep(6.5)` between calls. With API key, can go to 50 / 30s.

### JSONL files are gitignored

`.gitignore` excludes `*.jsonl` (project default). Big result files
won't accidentally bloat git, but you must remember to also push them
to OSS or commit summaries instead. The summary `.json` files are NOT
gitignored.

### SSH key has passphrase

`~/.ssh/id_ed25519` on the project owner's machine is passphrase-
protected. Direct `ssh root@47.251.245.235` requires `ssh-add` first.
Agents: prefer `aliyun ecs RunCommand` over SSH. It's TLS-authenticated
via the AK and works without the user's SSH passphrase.

### Slack webhook is not in local .env

`SLACK_WEBHOOK_URL` is in GH Secrets only. The local `.env` doesn't
have it. To post to Slack, dispatch the `notify_slack.yml` workflow:
```bash
gh workflow run notify_slack.yml -R fxp/cyberai --ref main \
  -f title="..." -f color="good" -f summary="$BODY"
```

### Source-extract location heuristics

Each `scan_<target>_*.py` has a SCANS list mapping `(extract_filename,
human_label)`. The `validate_findings.py` script reverse-greps the
extract directory by function name from the label. If the model emitted
a finding at line numbers OUTSIDE the extract (which happens — the model
sometimes invents plausible-looking line numbers from training data),
the grounding fails. These are auto-classified `NO_EXTRACT` and dropped
in J2.

---

## Cost reference (BigModel pricing, 2026)

| Stage | Per-run cost | Wall time |
|---|---|---|
| Pipeline A on 1 target (~30 segments) | $1-3 | 60-120 min |
| Pipeline A `--all` (10 targets, ~300 segments) | $15-30 | ~20h |
| Pipeline B on 1 target (max_files=30) | $5-15 | ~25 min |
| H adversarial verify (~240 CRITICAL+HIGH) | $0.50 | ~3.5h |
| J1 NVD lookup | $0 | ~10 min for 70 |
| J2 source grounding | $0 | seconds |
| J3 glm-4-plus cross-check (~70) | $1.50 | ~10 min |
| J5 disclosure drafts (~5 survivors) | $0.01 | ~5 min |
| ECS lease (1 month, 2c8G + EIP) | 420 CNY | continuous |
| OSS storage (~5MB / scan) | <0.01 CNY | continuous |

A complete A→H→J1+J2+J3→J5 cycle with 10 targets: **~$20 + 1 day wall.**

---

## Disclosure protocol

We follow Google Project Zero 90-day coordinated disclosure:

1. After J5 produces a draft, a **human** must:
   - Verify file:line still maps to the bug at upstream HEAD.
   - Check `git blame` and bug-fix history for whether it's already patched
     in master / a release branch.
   - Build a minimal PoC under ASAN/UBSAN and confirm the sanitizer fires.
   - Rewrite the draft with verified detail (the GLM draft is template
     boilerplate — never send unverified).
2. Send to upstream `security@<vendor>` (or maintainer-recommended
   address) using a real email, not a placeholder.
3. Wait up to 90 days for a fix. Send a follow-up at +30 days if no
   acknowledgement. Set a calendar reminder for +90.
4. After patch lands, request a CVE assignment from MITRE if the vendor
   hasn't.
5. Update `research/disclosures/<target>_<id>_draft.md` with timeline
   updates as they happen.
6. Public disclosure (writeup, blog post, github issue) only **after**
   patch is shipped or the 90-day window closes, whichever first.

The first disclosure cycle (CAND-008, Mosquitto integer underflow) was
reported to security@mosquitto.org on 2026-04-18 and **deprioritized on
2026-05-07 after expert review judged it low-severity** (Medium DoS,
not a credible risk for typical Mosquitto deployments). No follow-up
email was sent. Public disclosure deadline (2026-07-17) is moot. If the
maintainer responds at any point, fold the response back into research
notes; otherwise treat as closed-out research.

The libpng `png_combine_row` heap overflow was investigated as the lead
candidate but **REFUTED on 2026-05-20** by a 32-bit + ASAN PoC: libpng's
`png_check_IHDR` architecture guard rejects any overflowing width at
`png_read_info()` (even with `png_set_user_limits()` overriding the soft
1M ceiling), so `png_combine_row` is never reached. Documented as a
negative result in `research/disclosures/libpng_png_combine_row_overflow.md`.
Notably both glm-5.1 (conf 0.85) and glm-4-plus (conf 0.9) plus a human
code-read had judged it exploitable — only the empirical PoC caught it.

The other PARTIAL survivor (libxml2 `xmlXPathNextAncestor` namespace
"type confusion") was also **REFUTED on 2026-05-20** — by code semantics,
not even needing a PoC. The flagged `xmlNsPtr`→`xmlNodePtr` cast is
intentional libxml2 design: XPath namespace nodes store the parent
`xmlNode` in `ns->next` (see comment in `xmlXPathNodeSetCreate` and the
assignment in `xmlXPathNodeSetDupNs`). The exploit presupposes a separate
memory-corruption primitive, so it is not independently triggerable. The
J5 draft's "CVE-2023-39615" attribution was also a hallucination (that CVE
is `xmlSAX2StartElement` in SAX2.c). Documented in
`research/disclosures/libxml2_xmlXPathNextAncestor_type_confusion.md`.

**No active disclosure candidate.** Both 2026-05-04-cycle survivors are
refuted. A re-grounding pass against the next-highest 70-finding pool also
refuted the top freetype candidate (`tt_cmap4_char_map_binary` OOB reads,
2 independent flags; documented as a negative result in
`research/disclosures/freetype_tt_cmap4_oob_reads.md`). That makes **3 for 3
independent attempts** refuted by whole-file context, all failing the same
way: single extracted function read in isolation, missing the
invariant/guard established at a sibling function (load-time validator
in freetype, IHDR check in libpng, namespace-node design contract in
libxml2). The pipeline has a structural blind spot. Whole-subsystem
grounding (improvement #5/#8) is mandatory before any future "survivor"
should be trusted.

---

## Improvements / open work

These are documented for the next agent:

1. **~~Pursue libpng / libxml2 disclosures~~ — DONE, BOTH REFUTED 2026-05-20.**
   - libpng `png_combine_row`: 32-bit ASAN PoC proved the overflow
     unreachable (png_check_IHDR guard fires first).
     `research/disclosures/libpng_png_combine_row_overflow.md`.
   - libxml2 `xmlXPathNextAncestor`: refuted by code semantics — the cast
     is intentional design (`ns->next` = parent node). CVE attribution was
     hallucinated.
     `research/disclosures/libxml2_xmlXPathNextAncestor_type_confusion.md`.
   Root cause of both: single-function extracts miss invariants/guards in
   sibling functions. **Top priority is now improvement #5 + #8** (whole-
   subsystem grounding + sanitizer pipeline) so future survivors are
   trustworthy — there is currently no validated novel finding to disclose.
2. **Fix per-target timeout** in `run_daily_scans.sh` to be 360s+. The
   current 240s causes ~30% spurious timeouts on glm-5.1.
3. **Retry mechanism for verify ERRORs.** 68/236 verifies hit 300s
   timeout in the 2026-05-04 run. A second-pass retry would recover
   most.
4. **Batch upload to OSS.** Currently each script writes to ECS local
   then a separate OSS upload step. Should write directly to OSS with
   a temp local copy.
5. **~~Fix J2 source-extract grounding~~ — IMPLEMENTED 2026-05-20.**
   `find_extract` is now `find_extracts` and returns the matching primary
   file PLUS all sibling extracts under the same target directory.
   `assemble_context()` concatenates them with PRIMARY/SIBLING headers
   (budget 18000 chars). This closes the structural blind spot that
   refuted libpng / libxml2 / freetype top candidates: sibling extracts
   often contain the load-time validator or invariant-establishing init
   path that defeats the finding. The cross-check prompt
   (`CROSS_PROMPT`) was also rewritten with a 5-step refutation
   checklist (validator/guard, build-config gating, attacker control,
   already-patched, by-design invariant) — DEFAULT verdict is now
   FALSE_POSITIVE; CONFIRMED requires all 5 refutations to fail. The H
   verifier prompt (`VERIFY_PROMPT`) got the same refutation discipline,
   though without source-code context. Paths in `validate_findings.py`
   are now env-overridable (`CYBERAI_ROOT`, `CYBERAI_EXTRACTS_DIR`, etc.)
   for local testing. Next: re-run validate on the existing
   `verify_*.jsonl` to see how the survivor set shifts under the new
   methodology.
6. **Add Anthropic-backed verifier** as optional J4 (when
   `ANTHROPIC_API_KEY` is configured) for a true cross-vendor check.
   Currently we only have BigModel diversity (glm-5.1 vs glm-4-plus).
   The user explicitly prefers GLM-only, so this is opt-in only.
7. **Fix Pipeline B scan plan keyword bias** (`pipeline_b_plan.py`).
   Image format keywords (`tiff`, `png`, etc.) are missing from the
   priority weights, so codec-heavy targets get poor file selection.
   Add format/coder keywords or move plan to LLM-driven.
8. **Build sanitizer pipeline.** Currently we have no automated way to
   confirm a finding triggers ASAN. Should add a `build_sanitizer.sh`
   that compiles target with ASAN+UBSAN and a `try_poc.sh` that runs
   the PoC sketch.
9. **Re-run with newer model when it ships.** glm-5.1 was current as
   of 2026-05. Periodic re-scans with newer models (or different
   families like glm-z2-flash-reasoning) for benchmark progression.

---

## When in doubt

- Test commands on ECS via `aliyun ecs RunCommand` first; only SSH if
  you need an interactive shell.
- Push everything to OSS before doing anything destructive.
- Read the existing `verify_findings.py` / `validate_findings.py` /
  `generate_drafts.py` for tested patterns. They're idempotent and
  re-runnable.
- `gh run list` and `gh run view` are your friends for any GHA debugging.
- Cost-sensitive operations: fire one target as a smoke test, observe
  the per-segment latency + finding count, then decide whether to scale.
- The user prefers GLM-5.1 / GLM-4-plus for analysis. Do not silently
  swap in Claude or GPT.
