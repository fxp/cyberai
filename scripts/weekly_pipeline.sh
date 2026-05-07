#!/usr/bin/env bash
# weekly_pipeline.sh — fires Sunday 06:00 UTC via cyberai-weekly-verify.timer
#
# Runs the verify → validate → drafts pipeline on the most recent daily
# scan output, then has GLM-5.1 author a detailed Slack summary.
#
# Total wall: ~4h, cost ~$2 in glm-5.1 + glm-4-plus tokens.

set -euo pipefail
cd "$(dirname "$0")/.."
. .venv/bin/activate
set -a; . .env; set +a

WEEK_TAG=$(date -u +%Y-W%V)

echo "[weekly] $(date -u +%FT%TZ) start — week=$WEEK_TAG"

# 1. H — adversarial verify
echo "[weekly] H verify..."
python3 scripts/verify_findings.py
H_DONE=$?

# 2. J1+J2+J3 validate
echo "[weekly] J1+J2+J3 validate..."
python3 scripts/validate_findings.py
J_DONE=$?

# 3. J5 disclosure drafts
echo "[weekly] J5 drafts..."
python3 scripts/generate_drafts.py
D_DONE=$?

# 4. Author Slack summary via GLM-5.1
PROSE_FILE=/tmp/weekly_summary_$(date -u +%s).txt
python3 <<'PYEOF' > "$PROSE_FILE"
import os, json, glob, asyncio, sys
sys.path.insert(0, "src")
from cyberai.models.glm import GLMAdapter

# Latest verify summary
ver_files = sorted(glob.glob("research/verify_findings/verify_*_summary.json"))
ver = json.load(open(ver_files[-1])) if ver_files else {}

# Latest survivors
surv_files = sorted(glob.glob("research/validate_findings/survivors_*.jsonl"))
survivors = []
if surv_files:
    with open(surv_files[-1]) as f:
        for line in f:
            if line.strip():
                try: survivors.append(json.loads(line))
                except: pass

compact = [{
    "target": s.get("target"),
    "severity": s.get("severity"),
    "title": (s.get("title") or "")[:120],
    "h_conf": s.get("confidence"),
    "j3_verdict": s.get("_cross_glm4plus",{}).get("verdict"),
    "j3_match": s.get("_cross_glm4plus",{}).get("code_match"),
} for s in survivors]

prompt = f"""Write a Slack weekly verify summary for the CyberAI research project.
Plain text, ~150 words, 1-2 emoji max. Cover:
1. H verifier stats (CONFIRMED + PARTIAL vs FALSE_POSITIVE).
2. J1+J2+J3 narrowed survivors count.
3. Name each survivor (target + 1-line title).
4. Reminder that survivors need human triage before disclosure.

H verify summary: {json.dumps(ver)}
Survivors: {json.dumps(compact, indent=2, ensure_ascii=False)}

Output text only, no preamble."""

async def go():
    api_key = os.environ.get("GLM_API_KEY","")
    g = GLMAdapter(model_name="glm-5.1", api_key=api_key)
    resp, _ = await g.chat([{"role":"user","content":prompt}], timeout=240.0)
    print(resp.strip())

asyncio.run(go())
PYEOF

BODY="*CyberAI weekly verify pipeline — ${WEEK_TAG}*

$(cat "$PROSE_FILE")

Reports: https://github.com/fxp/cyberai/tree/main/research
Drafts: research/validate_findings/drafts/INDEX.md"

gh workflow run notify_slack.yml -R fxp/cyberai --ref main \
  -f title="CyberAI weekly verify — ${WEEK_TAG}" \
  -f color="good" \
  -f summary="$BODY"

echo "[weekly] $(date -u +%FT%TZ) done — H=$H_DONE J=$J_DONE D=$D_DONE"
